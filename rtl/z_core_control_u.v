/*

Copyright (c) 2025 Pau Díaz Cuesta

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

// ****************************************************
//                 Z-Core Control Unit
//     5-Stage Pipelined RISC-V RV32IMZicsr Processor
// ****************************************************

module z_core_control_u #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter STRB_WIDTH = (DATA_WIDTH/8),
    parameter INST_CACHE_DEPTH = 4096,
    parameter DATA_CACHE_DEPTH = 4096
)(
    input  wire                   clk,
    input  wire                   rstn,

    // AXI-Lite Master Interface
    output wire [ADDR_WIDTH-1:0]  m_axil_awaddr,
    output wire [2:0]             m_axil_awprot,
    output wire                   m_axil_awvalid,
    input  wire                   m_axil_awready,
    output wire [DATA_WIDTH-1:0]  m_axil_wdata,
    output wire [STRB_WIDTH-1:0]  m_axil_wstrb,
    output wire                   m_axil_wvalid,
    input  wire                   m_axil_wready,
    input  wire [1:0]             m_axil_bresp,
    input  wire                   m_axil_bvalid,
    output wire                   m_axil_bready,
    output wire [ADDR_WIDTH-1:0]  m_axil_araddr,
    output wire [2:0]             m_axil_arprot,
    output wire                   m_axil_arvalid,
    input  wire                   m_axil_arready,
    input  wire [DATA_WIDTH-1:0]  m_axil_rdata,
    input  wire [1:0]             m_axil_rresp,
    input  wire                   m_axil_rvalid,
    output wire                   m_axil_rready,

    // External Interrupt Inputs
    input  wire                   meip,    // Machine External Interrupt Pending
    input  wire                   mtip,    // Machine Timer Interrupt Pending
    input  wire                   msip     // Machine Software Interrupt Pending
);

// **************************************************
//                Instructions OP
// **************************************************

localparam R_INST      = 7'b0110011;
localparam I_INST      = 7'b0010011;
localparam I_LOAD_INST = 7'b0000011;
localparam JALR_INST   = 7'b1100111;
localparam S_INST      = 7'b0100011;
localparam B_INST      = 7'b1100011;
localparam JAL_INST    = 7'b1101111;
localparam LUI_INST    = 7'b0110111;
localparam AUIPC_INST  = 7'b0010111;

// System Instructions
localparam SYSTEM_INST = 7'b1110011;  // ECALL, EBREAK
localparam FENCE_INST  = 7'b0001111;  // FENCE

// **************************************************
//              AXI-Lite Master Interface
// **************************************************

reg  [ADDR_WIDTH-1:0] mem_addr;
wire [DATA_WIDTH-1:0] mem_rdata;
wire                  mem_ready;
wire                  mem_busy;

// mem_addr is reg (defined at top), driven by arbiter
reg                   mem_wen_comb;
reg                   mem_req_comb;

reg  [31:0]           mem_data_out_r;
reg  [STRB_WIDTH-1:0] mem_wstrb_r;

wire mem_req = mem_req_comb;
wire mem_wen = mem_wen_comb;

axil_master #(
    .DATA_WIDTH(DATA_WIDTH),
    .ADDR_WIDTH(ADDR_WIDTH),
    .STRB_WIDTH(STRB_WIDTH)
) u_axil_master (
    .clk(clk),
    .rstn(rstn),
    .mem_req(mem_req),
    .mem_wen(mem_wen),
    .mem_addr(mem_addr),
    .mem_wdata(mem_data_out_r),
    .mem_wstrb(mem_wstrb_r),
    .mem_rdata(mem_rdata),
    .mem_ready(mem_ready),
    .mem_busy(mem_busy),
    .m_axil_awaddr(m_axil_awaddr),
    .m_axil_awprot(m_axil_awprot),
    .m_axil_awvalid(m_axil_awvalid),
    .m_axil_awready(m_axil_awready),
    .m_axil_wdata(m_axil_wdata),
    .m_axil_wstrb(m_axil_wstrb),
    .m_axil_wvalid(m_axil_wvalid),
    .m_axil_wready(m_axil_wready),
    .m_axil_bresp(m_axil_bresp),
    .m_axil_bvalid(m_axil_bvalid),
    .m_axil_bready(m_axil_bready),
    .m_axil_araddr(m_axil_araddr),
    .m_axil_arprot(m_axil_arprot),
    .m_axil_arvalid(m_axil_arvalid),
    .m_axil_arready(m_axil_arready),
    .m_axil_rdata(m_axil_rdata),
    .m_axil_rresp(m_axil_rresp),
    .m_axil_rvalid(m_axil_rvalid),
    .m_axil_rready(m_axil_rready)
);

// **************************************************
//                 Program Counter
// **************************************************

localparam PC_INIT = 32'd0;


reg [31:0] PC;
reg        pc_q_valid;

reg fetch_wait;
reg [31:0] fetch_pc;  // PC of the cache fill in progress
reg mem_op_pending;
reg mem_op_is_uncached_r;

// ##################################################
//              PIPELINE REGISTERS
// ##################################################

// --- IF/ID Pipeline Register ---
reg [31:0] if_id_pc;
reg [31:0] if_id_ir;
reg        if_id_valid;
reg        if_id_branch_taken_pred;
reg [31:0] if_id_branch_target_pred;

// --- ID/EX Pipeline Register ---
reg [31:0] id_ex_pc;
reg [31:0] id_ex_rs1_data;
reg [31:0] id_ex_rs2_data;
reg [31:0] id_ex_imm;
reg [4:0]  id_ex_rd;
reg [4:0]  id_ex_rs1_addr;
reg [4:0]  id_ex_rs2_addr;
reg [4:0]  id_ex_alu_op;
reg [2:0]  id_ex_funct3;
reg        id_ex_is_load, id_ex_is_store, id_ex_is_branch;
reg        id_ex_is_jal, id_ex_is_jalr, id_ex_is_lui, id_ex_is_auipc, id_ex_is_div;
reg        id_ex_is_i_alu;
reg        id_ex_reg_write;
reg        id_ex_valid;
reg        id_ex_branch_taken_pred;
reg [31:0] id_ex_branch_target_pred;

// --- ID/EX CSR Pipeline Fields (Zicsr) ---
reg        id_ex_is_csr;
reg        id_ex_is_mret;
reg [11:0] id_ex_csr_addr;
reg [4:0]  id_ex_csr_zimm;

// --- ID/EX Exception Pipeline Fields ---
reg        id_ex_is_ecall;
reg        id_ex_is_ebreak;
reg        id_ex_is_illegal;
reg        id_ex_is_iaf;     // PMA instruction access fault (cause 1)
reg [31:0] id_ex_ir;         // Raw instruction (for mtval on illegal insn)

// --- EX/MEM Pipeline Register ---
reg [31:0] ex_mem_alu_result;
reg [31:0] ex_mem_rs2_data;
reg [4:0]  ex_mem_rd;
reg [2:0]  ex_mem_funct3;
reg        ex_mem_is_load, ex_mem_is_store;
reg        ex_mem_reg_write;
reg        ex_mem_valid;

// --- MEM/WB Pipeline Register ---
reg [31:0] mem_wb_result;
reg [4:0]  mem_wb_rd;
reg        mem_wb_reg_write;
reg        mem_wb_valid;
reg [2:0]  mem_wb_funct3;
reg [1:0]  mem_wb_alu_result_lo;

// ##################################################
//       INSTRUCTION CACHE (uses z_core_instr_cache)
// ##################################################

wire [31:0] instr_cache_address;
wire [31:0] instr_cache_data_in;
wire instr_cache_wen;

wire [31:0] instr_cache_address_d;    // registered: the address presented LAST cycle
wire [31:0] instr_cache_data_out;
wire instr_cache_valid;
wire instr_cache_cache_hit;
wire instr_cache_cache_miss;

z_core_instr_cache#(
    .DATA_WIDTH(DATA_WIDTH),
    .ADDR_WIDTH(ADDR_WIDTH),
    .CACHE_DEPTH(INST_CACHE_DEPTH)
) instr_cache (
    .clk(clk),
    .rstn(rstn),
    .wen(instr_cache_wen), 
    .addr_rd(instr_cache_address),
    .addr_rd_d(instr_cache_address_d),
    .addr_wr(fetch_pc),
    .data_in(instr_cache_data_in),
    .data_out(instr_cache_data_out),
    .valid(instr_cache_valid),
    .cache_hit(instr_cache_cache_hit),
    .cache_miss(instr_cache_cache_miss)
);

// ##################################################
//     DATA CACHE (uses z_core_data_cache.v)
// ##################################################

wire data_cache_wen;
wire data_cache_cs;
wire [ADDR_WIDTH-1:0] data_cache_addr;
reg [DATA_WIDTH-1:0] data_cache_data_in;
reg [3:0] data_cache_strb;
wire data_cache_refill_complete;
reg data_cache_writeback_pending_r;

wire [DATA_WIDTH-1:0] data_cache_data_out;
wire data_cache_cache_hit;
wire data_cache_dirty_writeback_enabled;
wire [ADDR_WIDTH-1:0] data_cache_dirty_writeback_addr;
wire [DATA_WIDTH-1:0] data_cache_dirty_writeback_data;
wire [3:0] data_cache_dirty_writeback_strb;
wire data_cache_request_refill;
wire data_cache_flush_done;
assign data_cache_wen = ex_mem_is_store;
assign data_cache_cs = ex_mem_valid && (ex_mem_is_load || ex_mem_is_store)
                       && !mem_op_pending && !data_uncacheable_mem;
assign data_cache_addr = ex_result & ~32'b11;
assign data_cache_refill_complete = mem_op_pending && mem_ready && data_cache_request_refill
                                    && !data_cache_writeback_pending_r && !mem_op_is_uncached_r;

z_core_data_cache#(
    .ADDR_WIDTH(ADDR_WIDTH),
    .DATA_WIDTH(DATA_WIDTH),
    .CACHE_ENTRIES(DATA_CACHE_DEPTH*2)
) data_cache (
    .clk(clk),
    .rstn(rstn),
    .wen(data_cache_wen),
    .cs(data_cache_cs),
    .refill_complete(data_cache_refill_complete),
    .addr(data_cache_addr),
    .data_in(data_cache_data_in),
    .strb(data_cache_strb),
    .pipeline_enable((id_ex_valid && (id_ex_is_load || id_ex_is_store)) && !ex_stall),
    .data_out(data_cache_data_out),
    .cache_hit_comb(data_cache_cache_hit),
    .dirty_writeback_enabled(data_cache_dirty_writeback_enabled),
    .dirty_writeback_addr(data_cache_dirty_writeback_addr),
    .dirty_writeback_data(data_cache_dirty_writeback_data),
    .dirty_writeback_strb(data_cache_dirty_writeback_strb),
    .request_refill(data_cache_request_refill),
    .cache_flush_done(data_cache_flush_done)
);

// ##################################################
//      PMA CHECKERS (uses z_core_pma)
// ##################################################
//
// Three combinational instances:
//   - pma_fetch_dec : PMA on if_id_pc, type=fetch. Drives dec_is_iaf
//                     (instruction access fault) and instr_cache_wen gate
//                     (suppress fill of non-cacheable I-fetches).
//   - pma_data_ex  : PMA on alu_out (data address being formed in EX),
//                    type=load|store. Drives load/store access faults
//                    that fire in the same EX trap cycle as misalign.
//   - pma_data_mem : PMA on ex_mem_alu_result (MEM stage address).
//                    Drives data_uncacheable_mem, which gates
//                    data_cache_cs and triggers the uncached AXI path.

localparam [1:0] PMA_ACCESS_FETCH = 2'b00;
localparam [1:0] PMA_ACCESS_LOAD  = 2'b01;
localparam [1:0] PMA_ACCESS_STORE = 2'b10;

// ---- Fetch PMA: at decode (against if_id_pc) ----
wire pma_fetch_dec_executable;
wire pma_fetch_dec_access_fault;
wire pma_fetch_dec_unused_c, pma_fetch_dec_unused_r, pma_fetch_dec_unused_w, pma_fetch_dec_unused_io;

z_core_pma #(.ADDR_WIDTH(ADDR_WIDTH)) pma_fetch_dec (
    .addr        (if_id_pc),
    .access_type (PMA_ACCESS_FETCH),
    .cacheable   (pma_fetch_dec_unused_c),
    .readable    (pma_fetch_dec_unused_r),
    .writable    (pma_fetch_dec_unused_w),
    .executable  (pma_fetch_dec_executable),
    .is_io       (pma_fetch_dec_unused_io),
    .access_fault(pma_fetch_dec_access_fault)
);

// ---- Fetch PMA: at fill (against fetch_pc) ----
wire pma_fetch_pc_cacheable;
wire pma_fetch_pc_unused_r, pma_fetch_pc_unused_w, pma_fetch_pc_unused_x, pma_fetch_pc_unused_io, pma_fetch_pc_unused_af;

z_core_pma #(.ADDR_WIDTH(ADDR_WIDTH)) pma_fetch_fill (
    .addr        (fetch_pc),
    .access_type (PMA_ACCESS_FETCH),
    .cacheable   (pma_fetch_pc_cacheable),
    .readable    (pma_fetch_pc_unused_r),
    .writable    (pma_fetch_pc_unused_w),
    .executable  (pma_fetch_pc_unused_x),
    .is_io       (pma_fetch_pc_unused_io),
    .access_fault(pma_fetch_pc_unused_af)
);

// ---- Data PMA at EX stage (against alu_out) ----
wire [1:0] pma_data_ex_acctype = id_ex_is_store ? PMA_ACCESS_STORE : PMA_ACCESS_LOAD;
wire pma_data_ex_access_fault;
wire pma_data_ex_unused_c, pma_data_ex_unused_r, pma_data_ex_unused_w, pma_data_ex_unused_x, pma_data_ex_unused_io;

z_core_pma #(.ADDR_WIDTH(ADDR_WIDTH)) pma_data_ex (
    .addr        (alu_out),
    .access_type (pma_data_ex_acctype),
    .cacheable   (pma_data_ex_unused_c),
    .readable    (pma_data_ex_unused_r),
    .writable    (pma_data_ex_unused_w),
    .executable  (pma_data_ex_unused_x),
    .is_io       (pma_data_ex_unused_io),
    .access_fault(pma_data_ex_access_fault)
);

// ---- Data PMA at MEM stage (against ex_mem_alu_result) ----
wire pma_data_mem_cacheable;
wire pma_data_mem_unused_r, pma_data_mem_unused_w, pma_data_mem_unused_x, pma_data_mem_unused_io, pma_data_mem_unused_af;

z_core_pma #(.ADDR_WIDTH(ADDR_WIDTH)) pma_data_mem (
    .addr        (ex_mem_alu_result),
    .access_type (PMA_ACCESS_LOAD), // access_type irrelevant for cacheable bit
    .cacheable   (pma_data_mem_cacheable),
    .readable    (pma_data_mem_unused_r),
    .writable    (pma_data_mem_unused_w),
    .executable  (pma_data_mem_unused_x),
    .is_io       (pma_data_mem_unused_io),
    .access_fault(pma_data_mem_unused_af)
);

wire data_uncacheable_mem = ~pma_data_mem_cacheable;


// ##################################################
//      INSTRUCTION DECODER (uses z_core_decoder)
// ##################################################

wire [6:0]  dec_op;
wire [4:0]  dec_rs1, dec_rs2, dec_rd;
wire [31:0] dec_Iimm, dec_Simm, dec_Uimm, dec_Bimm, dec_Jimm;
wire [2:0]  dec_funct3;
wire [6:0]  dec_funct7;
wire [11:0] dec_csr_addr;
wire [4:0]  dec_csr_zimm;

z_core_decoder decoder (
    .inst(if_id_ir),
    .op(dec_op),
    .rs1(dec_rs1),
    .rs2(dec_rs2),
    .rd(dec_rd),
    .Iimm(dec_Iimm),
    .Simm(dec_Simm),
    .Uimm(dec_Uimm),
    .Bimm(dec_Bimm),
    .Jimm(dec_Jimm),
    .funct3(dec_funct3),
    .funct7(dec_funct7),
    .csr_addr(dec_csr_addr),
    .csr_zimm(dec_csr_zimm)
);

// ##################################################
//              ALU CONTROL (uses z_core_alu_ctrl)
// ##################################################

wire [4:0] dec_alu_op;

z_core_alu_ctrl alu_ctrl (
    .alu_op(dec_op),
    .alu_funct3(dec_funct3),
    .alu_funct7(dec_funct7),
    .alu_inst_type(dec_alu_op)
);


// Control signal decode (from current IF/ID instruction)
wire dec_is_load   = (dec_op == I_LOAD_INST);
wire dec_is_store  = (dec_op == S_INST);
wire dec_is_branch = (dec_op == B_INST);
wire dec_is_jal    = (dec_op == JAL_INST);
wire dec_is_jalr   = (dec_op == JALR_INST);
wire dec_is_lui    = (dec_op == LUI_INST);
wire dec_is_auipc  = (dec_op == AUIPC_INST);
wire dec_is_r_type = (dec_op == R_INST);
wire dec_is_i_alu  = (dec_op == I_INST);
wire dec_is_div    = (dec_op == R_INST) & (dec_alu_op >= 5'd20) & (dec_alu_op <= 5'd23);

// Zicsr / System instruction detection
wire dec_is_csr    = (dec_op == SYSTEM_INST) && (dec_funct3 != 3'b000);
wire dec_is_mret   = (dec_op == SYSTEM_INST) && (dec_funct3 == 3'b000) && (if_id_ir[31:20] == 12'h302);

// Illegal: opcode doesn't match any known type (0x00000000 is treated as NOP)
wire dec_is_fence  = (dec_op == FENCE_INST);
wire dec_opcode_valid = dec_is_load | dec_is_store | dec_is_branch |
                        dec_is_jal | dec_is_jalr | dec_is_lui | dec_is_auipc |
                        dec_is_r_type | dec_is_i_alu | dec_is_csr |
                        dec_is_mret | dec_is_ecall | dec_is_ebreak | dec_is_fence;
wire dec_is_illegal = if_id_valid && !dec_opcode_valid && (if_id_ir != 32'h0);

wire dec_is_iaf = if_id_valid && pma_fetch_dec_access_fault;

wire dec_reg_write = dec_is_r_type | dec_is_i_alu | dec_is_load | 
                     dec_is_jal | dec_is_jalr | dec_is_lui | dec_is_auipc |
                     dec_is_csr;

// Immediate mux
wire [31:0] dec_imm = dec_is_i_alu | dec_is_load | dec_is_jalr ? dec_Iimm :
                      dec_is_store  ? dec_Simm :
                      dec_is_branch ? dec_Bimm :
                      dec_is_jal    ? dec_Jimm :
                      dec_Uimm;

// ##################################################
//              REGISTER FILE (uses z_core_reg_file)
// ##################################################

wire [31:0] rf_rs1_data, rf_rs2_data;

z_core_reg_file reg_file (
    .clk(clk),
    .reset(~rstn),
    .rd(mem_wb_rd),
    .rd_in(mem_wb_fwd_result),
    .write_enable(mem_wb_valid && mem_wb_reg_write && mem_wb_rd != 5'b0),
    .rs1(dec_rs1),
    .rs2(dec_rs2),
    .rs1_out(rf_rs1_data),
    .rs2_out(rf_rs2_data)
);

wire [31:0] fwd_rs1_data;
wire [31:0] fwd_rs2_data;
// ##################################################
//                   ALU (uses z_core_alu)
// ##################################################

// ALU inputs from ID/EX registers with forwarding
wire [31:0] alu_in1 = id_ex_is_auipc ? id_ex_pc : 
                      id_ex_is_lui   ? 32'b0 : 
                      fwd_rs1_data;

wire [31:0] alu_in2 = (id_ex_is_load | id_ex_is_store | id_ex_is_lui | 
                       id_ex_is_auipc | id_ex_is_jal | id_ex_is_jalr | id_ex_is_i_alu) ? id_ex_imm :
                      id_ex_is_branch ? fwd_rs2_data :
                      fwd_rs2_data;  // R-type

wire [31:0] alu_out;
wire        alu_branch;

z_core_alu alu (
    .alu_in1(alu_in1),
    .alu_in2(alu_in2),
    .alu_inst_type(id_ex_alu_op),
    .alu_out(alu_out),
    .alu_branch(alu_branch)
);

// ##################################################
//          DIV Unit (uses z_core_div_unit)
// ##################################################

wire div_running; // When division is running, we have to stall the pipeline
wire div_done;
wire [31:0] div_result;

// Division by zero detection (RISC-V spec):
// - DIV/DIVU by 0:  result = -1 (0xFFFFFFFF)
// - REM/REMU by 0:  result = dividend
wire div_by_zero = (alu_in2 == 32'b0);
wire [31:0] div_by_zero_result = id_ex_funct3[1] ? alu_in1 : 32'hFFFFFFFF;

// Only start division unit if divisor is non-zero
// Forwarding from ex_mem should work since values update before being sampled
wire div_start = !div_running && !div_done && id_ex_is_div && id_ex_valid && !div_by_zero;

// Division complete: either div_done from unit OR div_by_zero (instant)
wire div_complete = div_done || (id_ex_is_div && div_by_zero);

// Final division result: use bypass result for div-by-zero, otherwise unit result
wire [31:0] div_final_result = div_by_zero ? div_by_zero_result : div_result;

z_core_div_unit div_unit (
    .clk(clk),
    .rstn(rstn),
    .dividend(alu_in1),
    .divisor(alu_in2),
    .is_signed(~id_ex_funct3[0]),
    .quotient_or_rem(~id_ex_funct3[1]),
    .div_start(div_start),
    .div_running(div_running),
    .div_done(div_done),
    .div_result(div_result)
);

// ##################################################
//              DATA FORWARDING
// ##################################################

wire ex_mem_load_completing = ex_mem_is_load && mem_op_pending && mem_ready;
wire ex_mem_load_in_flight  = ex_mem_is_load && !ex_mem_load_completing;
wire [31:0] ex_mem_fwd_value = ex_mem_load_completing ? mem_load_data : ex_mem_alu_result;
wire ex_mem_fwd_ok = ex_mem_valid && ex_mem_reg_write && ex_mem_rd != 5'b0
                  && !ex_mem_load_in_flight;

reg [31:0] cache_load_result;
always @* begin
    case (mem_wb_funct3)
        3'b000: case (mem_wb_alu_result_lo)  // LB (signed)
            2'b00: cache_load_result = {{24{data_cache_data_out[7]}},  data_cache_data_out[7:0]};
            2'b01: cache_load_result = {{24{data_cache_data_out[15]}}, data_cache_data_out[15:8]};
            2'b10: cache_load_result = {{24{data_cache_data_out[23]}}, data_cache_data_out[23:16]};
            2'b11: cache_load_result = {{24{data_cache_data_out[31]}}, data_cache_data_out[31:24]};
        endcase
        3'b001: case (mem_wb_alu_result_lo[1])  // LH (signed)
            1'b0: cache_load_result = {{16{data_cache_data_out[15]}}, data_cache_data_out[15:0]};
            1'b1: cache_load_result = {{16{data_cache_data_out[31]}}, data_cache_data_out[31:16]};
        endcase
        3'b010: cache_load_result = data_cache_data_out;  // LW
        3'b100: case (mem_wb_alu_result_lo)  // LBU (unsigned)
            2'b00: cache_load_result = {24'b0, data_cache_data_out[7:0]};
            2'b01: cache_load_result = {24'b0, data_cache_data_out[15:8]};
            2'b10: cache_load_result = {24'b0, data_cache_data_out[23:16]};
            2'b11: cache_load_result = {24'b0, data_cache_data_out[31:24]};
        endcase
        3'b101: case (mem_wb_alu_result_lo[1])  // LHU (unsigned)
            1'b0: cache_load_result = {16'b0, data_cache_data_out[15:0]};
            1'b1: cache_load_result = {16'b0, data_cache_data_out[31:16]};
        endcase
        default: cache_load_result = data_cache_data_out;
    endcase
end

wire [31:0] mem_wb_fwd_result = data_cache_hit_q ? cache_load_result : mem_wb_result;

// Forward from EX/MEM or MEM/WB to resolve RAW hazards
assign fwd_rs1_data =
    (ex_mem_fwd_ok && ex_mem_rd == id_ex_rs1_addr) ? ex_mem_fwd_value :
    (mem_wb_valid && mem_wb_reg_write && mem_wb_rd == id_ex_rs1_addr && mem_wb_rd != 5'b0) ? mem_wb_fwd_result :
    id_ex_rs1_data;

assign fwd_rs2_data =
    (ex_mem_fwd_ok && ex_mem_rd == id_ex_rs2_addr) ? ex_mem_fwd_value :
    (mem_wb_valid && mem_wb_reg_write && mem_wb_rd == id_ex_rs2_addr && mem_wb_rd != 5'b0) ? mem_wb_fwd_result :
    id_ex_rs2_data;

// ##################################################
//              HAZARD DETECTION
// ##################################################

// Load-use hazard: need to stall one cycle
wire load_use_hazard = id_ex_valid && id_ex_is_load && if_id_valid &&
    ((id_ex_rd == dec_rs1 && dec_rs1 != 5'b0) ||
     (id_ex_rd == dec_rs2 && dec_rs2 != 5'b0 && (dec_is_r_type || dec_is_store || dec_is_branch)));

// Memory operation in progress - stall whole pipeline  
wire mem_stall = (mem_op_pending && !mem_ready);

// System Instruction Detection
wire dec_is_ecall  = (dec_op == SYSTEM_INST) && (dec_funct3 == 3'b000) && (if_id_ir[31:20] == 12'h000);
wire dec_is_ebreak = (dec_op == SYSTEM_INST) && (dec_funct3 == 3'b000) && (if_id_ir[31:20] == 12'h001);

// ##################################################
//       MISALIGNMENT EXCEPTION DETECTION
// ##################################################

// Misaligned instruction fetch (cause 0): branch/jump to non-4B-aligned target (no C extension)
wire misalign_branch = id_ex_valid && id_ex_is_branch && alu_branch && (branch_target[1:0] != 2'b00);
wire misalign_jump   = id_ex_valid && (id_ex_is_jal || id_ex_is_jalr) && (jump_target[1:0] != 2'b00);

// Misaligned load (cause 4): LH/LHU at odd addr, LW at non-4B-aligned addr
wire misalign_load = id_ex_valid && id_ex_is_load &&
    ((id_ex_funct3[1:0] == 2'b01 && alu_out[0]  != 1'b0) ||      // LH/LHU
     (id_ex_funct3[1:0] == 2'b10 && alu_out[1:0] != 2'b00));     // LW

// Misaligned store (cause 6): SH at odd addr, SW at non-4B-aligned addr
wire misalign_store = id_ex_valid && id_ex_is_store &&
    ((id_ex_funct3[1:0] == 2'b01 && alu_out[0]  != 1'b0) ||      // SH
     (id_ex_funct3[1:0] == 2'b10 && alu_out[1:0] != 2'b00));     // SW

wire load_access_fault  = id_ex_valid && id_ex_is_load  && !misalign_load  && pma_data_ex_access_fault;
wire store_access_fault = id_ex_valid && id_ex_is_store && !misalign_store && pma_data_ex_access_fault;

// ##################################################
//     CSR FILE INSTANTIATION (Zicsr Extension)
// ##################################################

wire [31:0] csr_read_data;
reg  [31:0] csr_write_data;
wire        csr_wen;

// CSR source: rs1 for register variants, zero-extended zimm for immediate
wire [31:0] csr_src = id_ex_funct3[2] ? {27'b0, id_ex_csr_zimm} : fwd_rs1_data;

// Write suppression per §2.8: CSRRS/CSRRC with rs1=x0 or zimm=0 → no write
wire csr_src_is_zero = id_ex_funct3[2] ? (id_ex_csr_zimm == 5'b0) : (id_ex_rs1_addr == 5'b0);
assign csr_wen = id_ex_valid && id_ex_is_csr && !trap_enter_r && !ex_stall &&
                 (id_ex_funct3[1:0] == 2'b01 || !csr_src_is_zero);

always @(*) begin
    case (id_ex_funct3[1:0])
        2'b01:   csr_write_data = csr_src;                     // CSRRW / CSRRWI
        2'b10:   csr_write_data = csr_read_data | csr_src;     // CSRRS / CSRRSI
        2'b11:   csr_write_data = csr_read_data & ~csr_src;    // CSRRC / CSRRCI
        default: csr_write_data = csr_src;                     // Fallback
    endcase
end

reg  trap_enter_r;
reg  [31:0] trap_mepc_r;
reg  [31:0] trap_mcause_r;
reg  [31:0] trap_mtval_r;
wire mret_in_ex = id_ex_valid && id_ex_is_mret;

wire        csr_mstatus_mie;
wire [31:0] csr_mtvec;
wire [31:0] csr_mepc;
wire        csr_irq_pending;
wire        csr_mie_meie;
wire        csr_mie_mtie;
wire        csr_mie_msie;

// Pulse generators for performance counters
wire load_pulse;
wire write_pulse;
wire inst_fetch_pulse;
wire inst_cache_miss_pulse;

// AXI bus is granted to the EX/MEM load/store this cycle.
wire perf_axi_grant_dmem = ex_mem_valid
                        && (ex_mem_is_load || ex_mem_is_store)
                        && !mem_op_pending
                        && !mem_busy
                        && !fetch_wait;
assign load_pulse  = (perf_axi_grant_dmem && ex_mem_is_load)
                   || (data_cache_hit_comb && ex_mem_is_load);
assign write_pulse = (perf_axi_grant_dmem && ex_mem_is_store)
                   || (data_cache_hit_comb && ex_mem_is_store);


assign inst_fetch_pulse = fetch_wait && mem_ready && !flush;


assign inst_cache_miss_pulse = !flush
                            && !fetch_wait
                            && !stall
                            && cache_miss_q
                            && !mem_busy
                            && !mem_op_pending
                            && !(ex_mem_valid &&
                                 (ex_mem_is_load || ex_mem_is_store));

z_core_csr_file #(
    .DATA_WIDTH(DATA_WIDTH)
) u_csr_file (
    .clk(clk),
    .rstn(rstn),
    .csr_addr(id_ex_csr_addr),
    .csr_write_data(csr_write_data),
    .csr_wen(csr_wen),
    .csr_read_data(csr_read_data),
    .trap_enter(trap_enter_r),
    .trap_mepc(trap_mepc_r),
    .trap_mcause(trap_mcause_r),
    .trap_mtval(trap_mtval_r),
    .mret_exec(mret_in_ex),
    .meip(meip),
    .mtip(mtip),
    .msip(msip),
    .instret_pulse(mem_wb_valid),
    .inst_cache_hit_pulse(consume_inst),  // 1-cycle pulse per consumed I-cache hit
    .data_cache_hit_pulse(data_cache_hit_comb),
    .mem_read_pulse(load_pulse),
    .mem_write_pulse(write_pulse),
    .mem_inst_fetch_pulse(inst_fetch_pulse),
    .branch_misprediction_pulse(prediction_flush), // No need pulse: Always single cycle
    .pipeline_flush_pulse(flush),
    .inst_cache_miss_pulse(inst_cache_miss_pulse),
    .mstatus_mie(csr_mstatus_mie),
    .mtvec_out(csr_mtvec),
    .mepc_out(csr_mepc),
    .irq_pending(csr_irq_pending),
    .mie_meie_out(csr_mie_meie),
    .mie_mtie_out(csr_mie_mtie),
    .mie_msie_out(csr_mie_msie)
);


// Need to stall EX stage if:
// 1. MEM stage has pending operation waiting for completion (mem_stall)
// 2. EX/MEM has load/store but can't start yet (waiting for AXI bus to be free)
// 3. Data Cache Writeback Completed but not yet refilled
// 4. Division instruction in EX stage and division not complete yet
wire div_stall = id_ex_valid && id_ex_is_div && !div_complete;

wire ex_stall = mem_stall || 
                (ex_mem_valid && (ex_mem_is_load || ex_mem_is_store) && 
                (!data_cache_hit_comb) &&
                (!mem_op_pending || mem_busy)) ||
                ((mem_op_pending && mem_ready && data_cache_writeback_pending_r)) || 
                div_stall;

// Stall the pipeline (note: fetch_wait does NOT stall EX/MEM/WB stages)
// cache_flush_stall holds the whole pipeline until the data cache has
// finished its one-time post-reset invalidation (~4096 cycles).
wire cache_flush_stall = !data_cache_flush_done;
wire stall = load_use_hazard || ex_stall || cache_flush_stall;

// ##################################################
//              BRANCH/JUMP CONTROL
// ##################################################

wire branch_taken = id_ex_valid && id_ex_is_branch && alu_branch;
wire is_jump   = id_ex_valid && (id_ex_is_jal || id_ex_is_jalr);

wire branch_taken_pred;
wire id_ex_branch_taken_pred_valid = id_ex_branch_taken_pred & id_ex_valid;
wire [31:0] branch_target_pred;
wire is_branch = id_ex_is_branch & id_ex_valid;
wire branch_target_misspredict;

wire [31:0] branch_predictor_target = is_branch ? branch_target : (is_jump ? jump_target : 32'b0);

z_core_branch_pred branch_predictor(
    .clk(clk),
    .rstn(rstn),
    .branch_taken(branch_taken || is_jump),
    .is_branch(is_branch || is_jump),
    .inst_addr_wr(id_ex_pc),
    .branch_target_wr(branch_predictor_target),
    .inst_addr_rd(PC),
    .branch_taken_pred(branch_taken_pred),
    .branch_target_pred(branch_target_pred)
);


assign branch_target_misspredict = is_branch ? (id_ex_branch_target_pred != branch_target) : (is_jump ? (id_ex_branch_target_pred != jump_target) : 1'b0);


wire jump_misspredict = (id_ex_branch_taken_pred_valid ^ is_jump);
wire branch_misspredict = (branch_taken ^ id_ex_branch_taken_pred_valid);
wire prediction_flush = (branch_taken & branch_target_misspredict) || (is_jump ? (jump_misspredict || branch_target_misspredict) : branch_misspredict);
wire flush = prediction_flush || trap_enter_r || mret_in_ex;

// Track if we need to squash the NEXT instruction entering id_ex
wire if_id_is_jump = if_id_valid && (dec_is_jal || dec_is_jalr);
wire if_id_is_branch = if_id_valid && dec_is_branch;

wire [31:0] branch_target = id_ex_pc + id_ex_imm;
wire [31:0] jalr_target   = (fwd_rs1_data + id_ex_imm) & ~32'b1;
wire [31:0] jump_target   = id_ex_is_jalr ? jalr_target : branch_target;

// ##################################################
//              PIPELINE STAGE: FETCH
// ##################################################

wire [31:0] flush_target = trap_enter_r           ? csr_mtvec :
                           mret_in_ex             ? csr_mepc :
                           is_jump                ? jump_target :
                           id_ex_branch_taken_pred ? (id_ex_pc + 4) :
                                                    branch_target;

wire cache_hit_q  = pc_q_valid && instr_cache_cache_hit;
wire cache_miss_q = pc_q_valid && instr_cache_cache_miss;


wire [31:0] next_target = branch_taken_pred ? branch_target_pred : (PC + 32'd4);

// Direct delivery: AXI fill done and pipeline is ready to accept.
wire deliver_direct     = fetch_wait && mem_ready && !stall && !flush;

wire deliver_from_cache = cache_hit_q && !stall && !fetch_wait && !flush;
wire consume_inst       = deliver_direct || deliver_from_cache;

assign instr_cache_wen     = fetch_wait && mem_ready && !flush && pma_fetch_pc_cacheable;
assign instr_cache_data_in = mem_rdata;

// Next address presented at the cache.
//   Priority: flush > deliver_direct (pre-fetch next_target during the fill)
//           > fetch_wait (hold at fetch_pc) > stall (hold) > consume (advance)
//           > default (hold)
assign instr_cache_address =
    flush          ? flush_target :
    deliver_direct ? next_target  :
    fetch_wait     ? fetch_pc     :
    stall          ? PC           :
    consume_inst   ? next_target  :
                     PC;

always @(posedge clk) begin
    if (~rstn) begin
        PC                       <= PC_INIT;
        pc_q_valid               <= 1'b0;
        fetch_wait               <= 1'b0;
        fetch_pc                 <= PC_INIT;
        if_id_ir                 <= 32'h00000013;  // NOP
        if_id_pc                 <= 32'b0;
        if_id_valid              <= 1'b0;
        if_id_branch_taken_pred  <= 1'b0;
        if_id_branch_target_pred <= 32'b0;
    end else begin
        // Invalidate consumed IF/ID
        if (!stall && if_id_valid && !consume_inst)
            if_id_valid <= 1'b0;

        if (flush) begin
            PC          <= flush_target;
            pc_q_valid  <= 1'b1;
            fetch_wait  <= 1'b0;
            if_id_valid <= 1'b0;          // 1-cycle bubble (sync-cache penalty)
        end else if (fetch_wait) begin
            if (mem_ready) begin
                // AXI fill done.
                fetch_wait       <= 1'b0;
                if (!stall) begin
                    if_id_ir                 <= mem_rdata;
                    if_id_pc                 <= fetch_pc;
                    if_id_valid              <= 1'b1;
                    if_id_branch_taken_pred  <= branch_taken_pred;
                    if_id_branch_target_pred <= branch_target_pred;
                    PC                       <= next_target;
                    pc_q_valid               <= 1'b1;
                end else begin
                    PC         <= fetch_pc;
                    pc_q_valid <= 1'b1;
                end
            end else begin
                // Still waiting for AXI, hold at fetch_pc.
                PC         <= fetch_pc;
                pc_q_valid <= 1'b0;
            end
        end else if (stall) begin
            // Pipeline stall -> No delays when resumed
        end else if (cache_hit_q) begin
            // Hit
            if_id_ir                 <= instr_cache_data_out;
            if_id_pc                 <= PC;
            if_id_valid              <= 1'b1;
            if_id_branch_taken_pred  <= branch_taken_pred;
            if_id_branch_target_pred <= branch_target_pred;
            PC                       <= next_target;
            pc_q_valid               <= 1'b1;
        end else if (cache_miss_q) begin
            // Miss
            if (!mem_busy && !mem_op_pending &&
                !(ex_mem_valid && (ex_mem_is_load || ex_mem_is_store))) begin
                fetch_wait            <= 1'b1;
                fetch_pc              <= PC;
                pc_q_valid            <= 1'b0;
            end
        end else begin
            // addr_rd this cycle = PC, so next cycle the cache will have a valid instr
            pc_q_valid <= 1'b1;
        end
    end
end

// ##################################################
//              PIPELINE STAGE: DECODE
// ##################################################

// Forwarding for decode stage (into ID/EX)
wire [31:0] dec_fwd_rs1 =
    (ex_mem_fwd_ok && ex_mem_rd == dec_rs1 && dec_rs1 != 5'b0) ? ex_mem_fwd_value :
    (mem_wb_valid && mem_wb_reg_write && mem_wb_rd == dec_rs1 && mem_wb_rd != 5'b0) ? mem_wb_fwd_result :
    rf_rs1_data;

wire [31:0] dec_fwd_rs2 =
    (ex_mem_fwd_ok && ex_mem_rd == dec_rs2 && dec_rs2 != 5'b0) ? ex_mem_fwd_value :
    (mem_wb_valid && mem_wb_reg_write && mem_wb_rd == dec_rs2 && mem_wb_rd != 5'b0) ? mem_wb_fwd_result :
    rf_rs2_data;

always @(posedge clk) begin
    if (~rstn) begin
        id_ex_valid <= 1'b0;
        id_ex_pc <= 32'b0;
        id_ex_rs1_data <= 32'b0;
        id_ex_rs2_data <= 32'b0;
        id_ex_imm <= 32'b0;
        id_ex_rd <= 5'b0;
        id_ex_rs1_addr <= 5'b0;
        id_ex_rs2_addr <= 5'b0;
        id_ex_alu_op <= 5'b0;
        id_ex_funct3 <= 3'b0;
        id_ex_is_load <= 1'b0;
        id_ex_is_store <= 1'b0;
        id_ex_is_branch <= 1'b0;
        id_ex_is_jal <= 1'b0;
        id_ex_is_jalr <= 1'b0;
        id_ex_is_lui <= 1'b0;
        id_ex_is_auipc <= 1'b0;
        id_ex_is_i_alu <= 1'b0;
        id_ex_is_div <= 1'b0;
        id_ex_reg_write <= 1'b0;
        id_ex_branch_taken_pred <= 1'b0;
        id_ex_branch_target_pred <= 32'b0;
        id_ex_is_csr <= 1'b0;
        id_ex_is_mret <= 1'b0;
        id_ex_csr_addr <= 12'b0;
        id_ex_csr_zimm <= 5'b0;
        id_ex_is_ecall <= 1'b0;
        id_ex_is_ebreak <= 1'b0;
        id_ex_is_illegal <= 1'b0;
        id_ex_is_iaf <= 1'b0;
        id_ex_ir <= 32'b0;
    end else if (trap_enter_r || mret_in_ex || ((prediction_flush || load_use_hazard) && !ex_stall)) begin
        // Insert bubble on flush or load-use hazard.
        // prediction_flush is gated by !ex_stall: if the EX stage is stalled,
        // the jump/branch result hasn't been latched into EX/MEM yet, so we
        // must keep id_ex_valid until the stall clears to avoid losing rd writes.
        // trap_enter_r and mret_in_ex always clear id_ex (trap gates ex_mem_valid
        // independently via !trap_enter_r).
        id_ex_valid <= 1'b0;
        id_ex_reg_write <= 1'b0;
        id_ex_is_load <= 1'b0;
        id_ex_is_store <= 1'b0;
        id_ex_is_branch <= 1'b0;
        id_ex_is_jal <= 1'b0;
        id_ex_is_jalr <= 1'b0;
        id_ex_is_lui <= 1'b0;
        id_ex_is_auipc <= 1'b0;
        id_ex_is_div <= 1'b0;
        id_ex_branch_taken_pred <= 1'b0;
        id_ex_branch_target_pred <= 32'b0;
        id_ex_is_csr <= 1'b0;
        id_ex_is_mret <= 1'b0;
        id_ex_is_ecall <= 1'b0;
        id_ex_is_ebreak <= 1'b0;
        id_ex_is_illegal <= 1'b0;
        id_ex_is_iaf <= 1'b0;
    end else if (!stall && if_id_valid) begin
        id_ex_pc <= if_id_pc;
        id_ex_rs1_data <= dec_fwd_rs1;
        id_ex_rs2_data <= dec_fwd_rs2;
        id_ex_imm <= dec_imm;
        id_ex_rd <= dec_rd;
        id_ex_rs1_addr <= dec_rs1;
        id_ex_rs2_addr <= dec_rs2;
        id_ex_alu_op <= dec_alu_op;
        id_ex_funct3 <= dec_funct3;
        id_ex_is_load <= dec_is_load;
        id_ex_is_store <= dec_is_store;
        id_ex_is_branch <= dec_is_branch;
        id_ex_is_jal <= dec_is_jal;
        id_ex_is_jalr <= dec_is_jalr;
        id_ex_is_lui <= dec_is_lui;
        id_ex_is_auipc <= dec_is_auipc;
        id_ex_is_i_alu <= dec_is_i_alu;
        id_ex_is_div <= dec_is_div;
        id_ex_is_csr <= dec_is_csr;
        id_ex_is_mret <= dec_is_mret;
        id_ex_csr_addr <= dec_csr_addr;
        id_ex_csr_zimm <= dec_csr_zimm;
        id_ex_is_ecall <= dec_is_ecall;
        id_ex_is_ebreak <= dec_is_ebreak;
        id_ex_is_illegal <= dec_is_illegal;
        id_ex_is_iaf <= dec_is_iaf;
        id_ex_ir <= if_id_ir;
        id_ex_reg_write <= dec_reg_write;
        id_ex_branch_taken_pred <= if_id_branch_taken_pred;
        id_ex_branch_target_pred <= if_id_branch_target_pred;
        id_ex_valid <= 1'b1;
    end else if (!stall) begin
        id_ex_valid <= 1'b0;
    end else if (ex_stall && id_ex_valid) begin
        id_ex_rs1_data <= fwd_rs1_data;
        id_ex_rs2_data <= fwd_rs2_data;
    end
end

// ##################################################
//              PIPELINE STAGE: EXECUTE
// ##################################################

wire [31:0] ex_result = id_ex_is_csr   ? csr_read_data :    // CSR read (old value -> rd)
                        id_ex_is_lui   ? id_ex_imm :
                        id_ex_is_auipc ? (id_ex_pc + id_ex_imm) :
                        (id_ex_is_jal || id_ex_is_jalr) ? (id_ex_pc + 4) :
                        id_ex_is_div ? div_final_result :
                        alu_out;

always @(posedge clk) begin
    if (~rstn) begin
        ex_mem_valid <= 1'b0;
        ex_mem_alu_result <= 32'b0;
        ex_mem_rs2_data <= 32'b0;
        ex_mem_rd <= 5'b0;
        ex_mem_funct3 <= 3'b0;
        ex_mem_is_load <= 1'b0;
        ex_mem_is_store <= 1'b0;
        ex_mem_reg_write <= 1'b0;
    end else if (!mem_stall && !ex_stall) begin
        ex_mem_alu_result <= ex_result;
        ex_mem_rs2_data <= fwd_rs2_data;
        ex_mem_rd <= id_ex_rd;
        ex_mem_funct3 <= id_ex_funct3;
        ex_mem_is_load <= id_ex_is_load;
        ex_mem_is_store <= id_ex_is_store;
        ex_mem_reg_write <= id_ex_reg_write && !id_ex_is_branch && !id_ex_is_store && !id_ex_is_mret
                           && !id_ex_is_ecall && !id_ex_is_ebreak && !id_ex_is_illegal && !id_ex_is_iaf && !trap_enter_r
                           && !misalign_load && !misalign_store && !misalign_branch && !misalign_jump
                           && !load_access_fault && !store_access_fault;
        ex_mem_valid <= id_ex_valid && !id_ex_is_branch && !id_ex_is_mret
                        && !id_ex_is_ecall && !id_ex_is_ebreak && !id_ex_is_illegal && !id_ex_is_iaf && !trap_enter_r
                        && !misalign_load && !misalign_store && !misalign_branch && !misalign_jump
                        && !load_access_fault && !store_access_fault;
    end
end

// ##################################################
//              PIPELINE STAGE: MEMORY
// ##################################################

// Acts as a LSU (Load Store Unit)
reg [31:0] mem_load_data;
always @* begin
    case (ex_mem_funct3)
        3'b000: case (ex_mem_alu_result[1:0])  // LB (signed)
            2'b00: mem_load_data = {{24{mem_rdata[7]}},  mem_rdata[7:0]};
            2'b01: mem_load_data = {{24{mem_rdata[15]}}, mem_rdata[15:8]};
            2'b10: mem_load_data = {{24{mem_rdata[23]}}, mem_rdata[23:16]};
            2'b11: mem_load_data = {{24{mem_rdata[31]}}, mem_rdata[31:24]};
        endcase
        3'b001: case (ex_mem_alu_result[1])  // LH (signed)
            1'b0: mem_load_data = {{16{mem_rdata[15]}}, mem_rdata[15:0]};
            1'b1: mem_load_data = {{16{mem_rdata[31]}}, mem_rdata[31:16]};
        endcase
        3'b010: mem_load_data = mem_rdata;  // LW
        3'b100: case (ex_mem_alu_result[1:0])  // LBU (unsigned)
            2'b00: mem_load_data = {24'b0, mem_rdata[7:0]};
            2'b01: mem_load_data = {24'b0, mem_rdata[15:8]};
            2'b10: mem_load_data = {24'b0, mem_rdata[23:16]};
            2'b11: mem_load_data = {24'b0, mem_rdata[31:24]};
        endcase
        3'b101: case (ex_mem_alu_result[1])  // LHU (unsigned)
            1'b0: mem_load_data = {16'b0, mem_rdata[15:0]};
            1'b1: mem_load_data = {16'b0, mem_rdata[31:16]};
        endcase
        default: mem_load_data = mem_rdata;
    endcase
end

always @* begin
    if (data_cache_refill_complete) begin
        data_cache_data_in = mem_rdata;
    end else begin
        case (ex_mem_funct3[1:0])
            2'b00: data_cache_data_in = {4{ex_mem_rs2_data[7:0]}};
            2'b01: data_cache_data_in = {2{ex_mem_rs2_data[15:0]}};
            2'b10: data_cache_data_in = ex_mem_rs2_data;
            2'b11: data_cache_data_in = {2{ex_mem_rs2_data[15:0]}};
            default: data_cache_data_in = ex_mem_rs2_data;
        endcase
    end
end

always @* begin
    case (ex_mem_funct3[1:0])
        2'b00: data_cache_strb = 4'b0001 << ex_mem_alu_result[1:0];
        2'b01: data_cache_strb = 4'b0011 << ex_mem_alu_result[1:0];
        2'b10: data_cache_strb = 4'b1111;
        2'b11: data_cache_strb = 4'b0011 << ex_mem_alu_result[1:0];
    endcase
end

wire data_cache_hit_comb = data_cache_cs && data_cache_cache_hit;
wire data_cache_refill_comb = data_cache_cs && !data_cache_cache_hit && data_cache_request_refill;
reg data_cache_hit_q;

// PMA uncached path: an EX/MEM load/store to a non-cacheable address
// requests a direct AXI transaction. Mirrors the cache-miss trigger
// conditions (no concurrent fetch / no AXI bus busy / no other op
// already pending) so the cycle budget matches a current cache miss.
wire uncached_req = ex_mem_valid && (ex_mem_is_load || ex_mem_is_store)
                    && data_uncacheable_mem
                    && !mem_op_pending && !mem_busy && !fetch_wait;


reg [31:0] uncached_store_data;
always @* begin
    case (ex_mem_funct3[1:0])
        2'b00: uncached_store_data = {4{ex_mem_rs2_data[7:0]}};
        2'b01: uncached_store_data = {2{ex_mem_rs2_data[15:0]}};
        2'b10: uncached_store_data = ex_mem_rs2_data;
        2'b11: uncached_store_data = {2{ex_mem_rs2_data[15:0]}};
        default: uncached_store_data = ex_mem_rs2_data;
    endcase
end

always @(posedge clk) begin
    if (~rstn) begin
        mem_op_pending <= 1'b0;
        mem_op_is_uncached_r <= 1'b0;
        mem_data_out_r <= 32'b0;
        mem_wstrb_r <= 4'b1111;
        data_cache_hit_q <= 1'b0;
    end else begin
        if (data_cache_hit_comb) begin
            data_cache_hit_q <= 1'b1;
        end else if(data_cache_refill_comb && data_cache_dirty_writeback_enabled && !fetch_wait && !mem_busy) begin // Dirty Writeback Missed -> Writeback
            // For now, writeback will be done sequentially before refill,
            // Future work: Build Store Buffer to handle multiple writebacks in parallel.
            data_cache_hit_q <= 1'b0;
            mem_op_pending <= 1'b1;
            mem_data_out_r <= data_cache_dirty_writeback_data;
            mem_wstrb_r <= data_cache_dirty_writeback_strb;
            data_cache_writeback_pending_r <= 1'b1;
         end else if(((data_cache_refill_comb && !data_cache_dirty_writeback_enabled) || (data_cache_writeback_pending_r && mem_op_pending && mem_ready)) && !fetch_wait && !mem_busy) begin // Cache Missed / Writback Complete -> Refill
            data_cache_hit_q <= 1'b0;
            mem_op_pending <= 1'b1;
            data_cache_writeback_pending_r <= 1'b0;
        end else if (uncached_req) begin
            // PMA uncached: launch a direct AXI load/store. No cache fill on completion.
            data_cache_hit_q <= 1'b0;
            mem_op_pending <= 1'b1;
            mem_op_is_uncached_r <= 1'b1;
            mem_data_out_r <= uncached_store_data;
            mem_wstrb_r <= ex_mem_is_store ? data_cache_strb : 4'b1111;
        end else if (mem_op_pending && mem_ready) begin
            data_cache_hit_q <= 1'b0;
            mem_op_pending <= 1'b0;
            mem_op_is_uncached_r <= 1'b0;
        end else begin
            data_cache_hit_q <= 1'b0;
        end
    end
end

// ##################################################
//              PIPELINE STAGE: WRITEBACK
// ##################################################

always @(posedge clk) begin
    if (~rstn) begin
        mem_wb_valid <= 1'b0;
        mem_wb_result <= 32'b0;
        mem_wb_rd <= 5'b0;
        mem_wb_reg_write <= 1'b0;
        mem_wb_funct3 <= 3'b0;
        mem_wb_alu_result_lo <= 2'b0;
    end else if ((!mem_stall && !ex_stall) || (mem_op_pending && mem_ready) || data_cache_hit_comb) begin
        // Advance MEM/WB pipeline register when:
        // 1. No stalls (neither memory nor EX stage stalled), OR
        // 2. A memory operation just completed (even if stalled, we take the result)
        mem_wb_rd <= ex_mem_rd;
        mem_wb_reg_write <= ex_mem_reg_write && !ex_mem_is_store;
        mem_wb_valid <= ex_mem_valid && !ex_mem_is_store;
        mem_wb_funct3 <= ex_mem_funct3;
        mem_wb_alu_result_lo <= ex_mem_alu_result[1:0];

        if ((ex_mem_is_load && mem_op_pending && mem_ready)) begin
            mem_wb_result <= mem_load_data;
        end else begin
            mem_wb_result <= ex_mem_alu_result;
        end
    end else begin
        // Stalled - insert bubble (don't retire any instruction)
        mem_wb_valid <= 1'b0;
        mem_wb_reg_write <= 1'b0;
        mem_wb_rd <= 5'b0;
    end
end

// ##################################################
//        INTERRUPT DETECTION & TRAP ENTRY
// ##################################################
//
// Interrupt priority (§3.1.9): MEI (11) > MSI (3) > MTI (7)
// Taken only when pipeline is not stalled and no flush in progress.

reg [31:0] irq_cause;
always @(*) begin
    if (meip && csr_mie_meie)
        irq_cause = {1'b1, 31'd11};  // Machine External Interrupt
    else if (msip && csr_mie_msie)
        irq_cause = {1'b1, 31'd3};   // Machine Software Interrupt
    else
        irq_cause = {1'b1, 31'd7};   // Machine Timer Interrupt
end

// mepc for interrupts: earliest valid instruction in the pipeline
wire [31:0] trap_mepc_next = if_id_valid ? if_id_pc : PC;

always @(posedge clk) begin
    if (~rstn) begin
        trap_enter_r  <= 1'b0;
        trap_mepc_r   <= 32'b0;
        trap_mcause_r <= 32'b0;
        trap_mtval_r  <= 32'b0;
    end else begin
        trap_enter_r <= 1'b0;

        // Synchronous exceptions (highest priority)
        if (id_ex_valid && !trap_enter_r && !stall) begin
            if (misalign_branch || misalign_jump) begin
                trap_enter_r  <= 1'b1;
                trap_mepc_r   <= id_ex_pc;
                trap_mcause_r <= {1'b0, 31'd0};   // Instruction address misaligned
                trap_mtval_r  <= misalign_branch ? branch_target : jump_target;
            end else if (misalign_load) begin
                trap_enter_r  <= 1'b1;
                trap_mepc_r   <= id_ex_pc;
                trap_mcause_r <= {1'b0, 31'd4};   // Load address misaligned
                trap_mtval_r  <= alu_out;
            end else if (misalign_store) begin
                trap_enter_r  <= 1'b1;
                trap_mepc_r   <= id_ex_pc;
                trap_mcause_r <= {1'b0, 31'd6};   // Store/AMO address misaligned
                trap_mtval_r  <= alu_out;
            end else if (load_access_fault) begin
                trap_enter_r  <= 1'b1;
                trap_mepc_r   <= id_ex_pc;
                trap_mcause_r <= {1'b0, 31'd5};   // Load access fault (PMA)
                trap_mtval_r  <= alu_out;
            end else if (store_access_fault) begin
                trap_enter_r  <= 1'b1;
                trap_mepc_r   <= id_ex_pc;
                trap_mcause_r <= {1'b0, 31'd7};   // Store/AMO access fault (PMA)
                trap_mtval_r  <= alu_out;
            end else if (id_ex_is_iaf) begin
                trap_enter_r  <= 1'b1;
                trap_mepc_r   <= id_ex_pc;
                trap_mcause_r <= {1'b0, 31'd1};   // Instruction access fault (PMA)
                trap_mtval_r  <= id_ex_pc;
            end else if (id_ex_is_illegal) begin
                trap_enter_r  <= 1'b1;
                trap_mepc_r   <= id_ex_pc;
                trap_mcause_r <= {1'b0, 31'd2};   // Illegal instruction
                trap_mtval_r  <= id_ex_ir;
            end else if (id_ex_is_ecall) begin
                trap_enter_r  <= 1'b1;
                trap_mepc_r   <= id_ex_pc;
                trap_mcause_r <= {1'b0, 31'd11};  // Environment call from M-mode
                trap_mtval_r  <= 32'b0;
            end else if (id_ex_is_ebreak) begin
                trap_enter_r  <= 1'b1;
                trap_mepc_r   <= id_ex_pc;
                trap_mcause_r <= {1'b0, 31'd3};   // Breakpoint
                trap_mtval_r  <= id_ex_pc;         // Spec: mtval = PC of ebreak
            end
        end

        // Asynchronous interrupts (lower priority than exceptions)
        if (csr_irq_pending && !flush && !stall && !trap_enter_r &&
            !(id_ex_valid && (id_ex_is_illegal || id_ex_is_ecall || id_ex_is_ebreak || id_ex_is_iaf ||
                             misalign_branch || misalign_jump || misalign_load || misalign_store ||
                             load_access_fault || store_access_fault))) begin
            trap_enter_r  <= 1'b1;
            trap_mepc_r   <= trap_mepc_next;
            trap_mcause_r <= irq_cause;
            trap_mtval_r  <= 32'b0;
        end
    end
end

// ##################################################
//             PIPELINE STATE TRACKING
// ##################################################

localparam N_STATES = 5;
localparam STATE_FETCH_b = 0;
localparam STATE_DECODE_b = 1;
localparam STATE_EXECUTE_b = 2;
localparam STATE_MEM_b = 3;
localparam STATE_WRITE_b = 4;

wire [N_STATES-1:0] state;

assign state = {mem_wb_valid, ex_mem_valid, id_ex_valid, if_id_valid, fetch_wait | consume_inst};

// Unified Memory Request Logic (Arbiter)
// IMPORTANT: Don't assert mem_req when mem_ready is high to avoid race condition
// where the AXI master starts a new transaction while we're processing the old one.
always @* begin
    if (mem_op_pending && !mem_ready) begin
        mem_req_comb = 1'b1;
        if (mem_op_is_uncached_r) begin
            // PMA uncached load/store: route directly to AXI.
            mem_wen_comb = ex_mem_is_store;
            mem_addr = ex_mem_alu_result & ~32'b11;
        end else begin
            mem_wen_comb = data_cache_dirty_writeback_enabled;
            mem_addr = data_cache_dirty_writeback_enabled ? data_cache_dirty_writeback_addr : ex_mem_alu_result;
        end
    end else if (fetch_wait && !mem_ready) begin
        mem_req_comb = 1'b1;
        mem_wen_comb = 1'b0;
        mem_addr = fetch_pc;  // Use captured fetch_pc, not current PC
    end else begin
        mem_req_comb = 1'b0;
        mem_wen_comb = 1'b0;
        mem_addr = 32'b0;
    end
end


endmodule
