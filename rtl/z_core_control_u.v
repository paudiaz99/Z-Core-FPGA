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

// **************************************************
//                 Z-Core Control Unit
//        5-Stage Pipelined RISC-V RV32I Processor
// **************************************************

`timescale 1ns / 1ns


module z_core_control_u #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter STRB_WIDTH = (DATA_WIDTH/8)
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

    // Halt signal (ECALL/EBREAK detected - for RISCOF signature dump)
    output wire                   halt
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

assign mem_req = mem_req_comb;
assign mem_wen = mem_wen_comb;


reg  [31:0]           mem_data_out_r;
reg  [STRB_WIDTH-1:0] mem_wstrb_r;

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

// ##################################################
//              PIPELINE REGISTERS
// ##################################################

// --- IF/ID Pipeline Register ---
reg [31:0] if_id_pc;
reg [31:0] if_id_ir;
reg        if_id_valid;

// --- Skid Buffer for Fetch ---
reg [31:0] fetch_buffer_ir;
reg [31:0] fetch_buffer_pc;
reg        fetch_buffer_valid;

// --- ID/EX Pipeline Register ---
reg [31:0] id_ex_pc;
reg [31:0] id_ex_rs1_data;
reg [31:0] id_ex_rs2_data;
reg [31:0] id_ex_imm;
reg [4:0]  id_ex_rd;
reg [4:0]  id_ex_rs1_addr;
reg [4:0]  id_ex_rs2_addr;
reg [3:0]  id_ex_alu_op;
reg [2:0]  id_ex_funct3;
reg        id_ex_is_load, id_ex_is_store, id_ex_is_branch;
reg        id_ex_is_jal, id_ex_is_jalr, id_ex_is_lui, id_ex_is_auipc;
reg        id_ex_is_r_type, id_ex_is_i_alu;
reg        id_ex_reg_write;
reg        id_ex_valid;

// --- EX/MEM Pipeline Register ---
reg [31:0] ex_mem_pc;
reg [31:0] ex_mem_alu_result;
reg [31:0] ex_mem_rs2_data;
reg [4:0]  ex_mem_rd;
reg [2:0]  ex_mem_funct3;
reg        ex_mem_is_load, ex_mem_is_store;
reg        ex_mem_reg_write;
reg        ex_mem_valid;

// --- MEM/WB Pipeline Register ---
reg [31:0] mem_wb_result;
reg [31:0] mem_wb_pc;  // PC for debug tracing
reg [4:0]  mem_wb_rd;
reg        mem_wb_reg_write;
reg        mem_wb_valid;

// ##################################################
//             INSTRUCTION DECODER (uses z_core_decoder)
// ##################################################

wire [6:0]  dec_op;
wire [4:0]  dec_rs1, dec_rs2, dec_rd;
wire [31:0] dec_Iimm, dec_Simm, dec_Uimm, dec_Bimm, dec_Jimm;
wire [2:0]  dec_funct3;
wire [6:0]  dec_funct7;

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
    .funct7(dec_funct7)
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
wire dec_reg_write = dec_is_r_type | dec_is_i_alu | dec_is_load | 
                     dec_is_jal | dec_is_jalr | dec_is_lui | dec_is_auipc;

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
    .rd_in(mem_wb_result),
    .write_enable(mem_wb_valid && mem_wb_reg_write && mem_wb_rd != 5'b0),
    .rs1(dec_rs1),
    .rs2(dec_rs2),
    .rs1_out(rf_rs1_data),
    .rs2_out(rf_rs2_data)
);

// ##################################################
//              ALU CONTROL (uses z_core_alu_ctrl)
// ##################################################

wire [3:0] dec_alu_op;

z_core_alu_ctrl alu_ctrl (
    .alu_op(dec_op),
    .alu_funct3(dec_funct3),
    .alu_funct7(dec_funct7),
    .alu_inst_type(dec_alu_op)
);

// ##################################################
//                   ALU (uses z_core_alu)
// ##################################################

wire [31:0] fwd_rs1_data;
wire [31:0] fwd_rs2_data;

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
//              DATA FORWARDING
// ##################################################

// Forward from EX/MEM or MEM/WB to resolve RAW hazards
assign fwd_rs1_data = 
    (ex_mem_valid && ex_mem_reg_write && ex_mem_rd == id_ex_rs1_addr && ex_mem_rd != 5'b0) ? ex_mem_alu_result :
    (mem_wb_valid && mem_wb_reg_write && mem_wb_rd == id_ex_rs1_addr && mem_wb_rd != 5'b0) ? mem_wb_result :
    id_ex_rs1_data;

assign fwd_rs2_data = 
    (ex_mem_valid && ex_mem_reg_write && ex_mem_rd == id_ex_rs2_addr && ex_mem_rd != 5'b0) ? ex_mem_alu_result :
    (mem_wb_valid && mem_wb_reg_write && mem_wb_rd == id_ex_rs2_addr && mem_wb_rd != 5'b0) ? mem_wb_result :
    id_ex_rs2_data;

// ##################################################
//              HAZARD DETECTION
// ##################################################

reg mem_op_pending;

// Load-use hazard: need to stall one cycle
wire load_use_hazard = id_ex_valid && id_ex_is_load && if_id_valid &&
    ((id_ex_rd == dec_rs1 && dec_rs1 != 5'b0) ||
     (id_ex_rd == dec_rs2 && dec_rs2 != 5'b0 && (dec_is_r_type || dec_is_store || dec_is_branch)));

// Memory operation in progress - stall whole pipeline  
wire mem_stall = mem_op_pending && !mem_ready;

// System Instruction Detection (for RISCOF halt signal)
wire dec_is_ecall  = (dec_op == SYSTEM_INST) && (dec_funct3 == 3'b000) && (if_id_ir[31:20] == 12'h000);
wire dec_is_ebreak = (dec_op == SYSTEM_INST) && (dec_funct3 == 3'b000) && (if_id_ir[31:20] == 12'h001);

// Halt signal for RISCOF compliance testing (ECALL/EBREAK detection in ID stage)
assign halt = if_id_valid && (dec_is_ecall || dec_is_ebreak);


// Need to stall EX stage if:
// 1. MEM stage has pending operation waiting for completion (mem_stall)
// 2. EX/MEM has load/store but can't start yet (waiting for AXI bus to be free)
wire ex_stall = mem_stall || 
                (ex_mem_valid && (ex_mem_is_load || ex_mem_is_store) && 
                 (!mem_op_pending || mem_busy));

// Stall the pipeline (note: fetch_wait does NOT stall EX/MEM/WB stages)
wire stall = load_use_hazard || ex_stall;

// ##################################################
//              BRANCH/JUMP CONTROL
// ##################################################

wire branch_taken = id_ex_valid && id_ex_is_branch && alu_branch;
wire jump_taken   = id_ex_valid && (id_ex_is_jal || id_ex_is_jalr);

// Flush = control transfer detected in EX stage
// Need to squash the instruction that was in IF/ID when JAL entered ID/EX
wire flush        = branch_taken || jump_taken;

// Track if we need to squash the NEXT instruction entering id_ex
// This is set when the CURRENT if_id contains a jump being decoded into id_ex
wire if_id_is_jump = if_id_valid && (dec_is_jal || dec_is_jalr);
wire if_id_is_branch = if_id_valid && dec_is_branch;

wire [31:0] branch_target = id_ex_pc + id_ex_imm;
wire [31:0] jalr_target   = (fwd_rs1_data + id_ex_imm) & ~32'b1;
wire [31:0] next_pc       = flush ? (id_ex_is_jalr ? jalr_target : branch_target) :
                            stall ? PC : PC + 4;

// ##################################################
//              PIPELINE STAGE: FETCH
// ##################################################

reg fetch_wait;
reg [31:0] fetch_pc;  // Captures PC when fetch starts - used when fetch completes
reg squash_now;  // Set when JAL/JALR just entered id_ex, to squash instruction after
reg flush_r;     // Registered flush - set for one cycle after branch/jump detected

always @(posedge clk) begin
    if (~rstn) begin
        PC <= PC_INIT;
        fetch_wait <= 1'b0;
        fetch_pc <= PC_INIT;
        if_id_ir <= 32'h00000013;  // NOP
        if_id_pc <= 32'b0;
        if_id_valid <= 1'b0;
        flush_r <= 1'b0;
        fetch_buffer_valid <= 1'b0;
        fetch_buffer_ir <= 32'b0;
        fetch_buffer_pc <= 32'b0;
    end else begin
        if (flush) begin
            // Flush: invalidate IF/ID (delay slot) and redirect PC to target
            // The delay slot is invalidated by if_id_valid=0
            // Flush: invalidate IF/ID (delay slot) and redirect PC to target
            if_id_valid <= 1'b0;
            if_id_ir <= 32'h00000013;
            // Also invalidate the fetch buffer to prevent stale instructions from being loaded
            fetch_buffer_valid <= 1'b0;
            PC <= next_pc;
            fetch_wait <= 1'b0;
            // mem_req cleanup removed
            flush_r <= 1'b1;  // Register that flush happened
        end else begin
            // Clear if_id_valid when instruction is consumed by decode stage
            // (unless a new instruction is arriving to replace it)
            // new_instr_from_buffer: !stall && fetch_buffer_valid
            // new_instr_from_fetch: fetch_wait && mem_ready && !stall && !fetch_buffer_valid
            if (!stall && if_id_valid && 
                !(!stall && fetch_buffer_valid) && 
                !(fetch_wait && mem_ready && !stall && !fetch_buffer_valid)) begin
                // Instruction consumed by decode, no new instruction arriving
                if_id_valid <= 1'b0;
            end
            
            // Move buffer to IF/ID if not stalled and buffer valid
            if (!stall && fetch_buffer_valid) begin
                if_id_ir <= fetch_buffer_ir;
                if_id_pc <= fetch_buffer_pc;
                if_id_valid <= 1'b1;
                fetch_buffer_valid <= 1'b0;
            end
            
            // Check if we can start a new fetch:
            // 1. Not currently waiting for a fetch
            // 2. Memory bus not busy with data access (mem_op_pending)
            // 3. AXI master not busy (prevents overlap with cancelled fetch)
            // 4. Either buffer is empty OR (buffer will be emptied this cycle because !stall)
            // 5. EX stage does not need memory (prevent structural hazard)
            if (!fetch_wait && !mem_op_pending && !mem_busy &&
                !(ex_mem_valid && (ex_mem_is_load || ex_mem_is_store)) && 
                (!fetch_buffer_valid || !stall)) begin
                 // Start fetch - capture the PC we're fetching from
                 fetch_wait <= 1'b1;
                 fetch_pc <= PC;  // Capture PC for this fetch
            end else if (fetch_wait && mem_ready) begin
                // Fetch complete - use fetch_pc for the address, not current PC
                if (!stall && !fetch_buffer_valid) begin
                    // Pipeline active and buffer empty: load directly to IF/ID
                    if_id_ir <= mem_rdata;
                    if_id_pc <= fetch_pc;  // Use the PC that was captured when fetch started
                    if_id_valid <= 1'b1;
                end else begin
                    // Pipeline stalled or buffer full: load to buffer
                    fetch_buffer_ir <= mem_rdata;
                    fetch_buffer_pc <= fetch_pc;  // Use the PC that was captured when fetch started
                    fetch_buffer_valid <= 1'b1;
                end
                
                // Advance PC from the address we just fetched and clear flags
                PC <= fetch_pc + 4;
                fetch_wait <= 1'b0;
                // mem_req removal
            end else if (!fetch_wait) begin
                // mem_req removal
            end
            
            // Handle flush_r clearing (legacy from original code logic)
            flush_r <= 1'b0;
        end
    end
end

// ##################################################
//              PIPELINE STAGE: DECODE
// ##################################################

// Forwarding for decode stage (into ID/EX)
wire [31:0] dec_fwd_rs1 = 
    (ex_mem_valid && ex_mem_reg_write && ex_mem_rd == dec_rs1 && dec_rs1 != 5'b0) ? ex_mem_alu_result :
    (mem_wb_valid && mem_wb_reg_write && mem_wb_rd == dec_rs1 && mem_wb_rd != 5'b0) ? mem_wb_result :
    rf_rs1_data;

wire [31:0] dec_fwd_rs2 = 
    (ex_mem_valid && ex_mem_reg_write && ex_mem_rd == dec_rs2 && dec_rs2 != 5'b0) ? ex_mem_alu_result :
    (mem_wb_valid && mem_wb_reg_write && mem_wb_rd == dec_rs2 && mem_wb_rd != 5'b0) ? mem_wb_result :
    rf_rs2_data;

always @(posedge clk) begin
    if (~rstn) begin
        squash_now <= 1'b0;
        id_ex_valid <= 1'b0;
        id_ex_pc <= 32'b0;
        id_ex_rs1_data <= 32'b0;
        id_ex_rs2_data <= 32'b0;
        id_ex_imm <= 32'b0;
        id_ex_rd <= 5'b0;
        id_ex_rs1_addr <= 5'b0;
        id_ex_rs2_addr <= 5'b0;
        id_ex_alu_op <= 4'b0;
        id_ex_funct3 <= 3'b0;
        id_ex_is_load <= 1'b0;
        id_ex_is_store <= 1'b0;
        id_ex_is_branch <= 1'b0;
        id_ex_is_jal <= 1'b0;
        id_ex_is_jalr <= 1'b0;
        id_ex_is_lui <= 1'b0;
        id_ex_is_auipc <= 1'b0;
        id_ex_is_r_type <= 1'b0;
        id_ex_is_i_alu <= 1'b0;
        id_ex_reg_write <= 1'b0;
    end else if (flush || load_use_hazard) begin
        // Insert bubble - flush handles current cycle squash
        id_ex_valid <= 1'b0;
        id_ex_reg_write <= 1'b0;
        id_ex_is_load <= 1'b0;
        id_ex_is_store <= 1'b0;
        id_ex_is_branch <= 1'b0;
        id_ex_is_jal <= 1'b0;
        id_ex_is_jalr <= 1'b0;
        id_ex_is_lui <= 1'b0;
        id_ex_is_auipc <= 1'b0;
        // Set squash_now for delayed squash on next cycle
        if (flush) squash_now <= 1'b1;
    end else if (squash_now) begin
        // Squash instruction that was in IF/ID when jump entered ID/EX
        // Don't load if_id into id_ex, just invalidate
        id_ex_valid <= 1'b0;
        squash_now <= 1'b0;  // Clear after single use
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
        id_ex_is_r_type <= dec_is_r_type;
        id_ex_is_i_alu <= dec_is_i_alu;
        id_ex_reg_write <= dec_reg_write;
        id_ex_valid <= 1'b1;
        // Only set squash_now for JAL/JALR detected in if_id (unconditional jumps)
        // Branch squash is handled by flush -> squash_now path
        squash_now <= if_id_is_jump;
    end else if (!stall) begin
        id_ex_valid <= 1'b0;
        squash_now <= 1'b0;
    end else begin
        // On stall, clear squash_now since we already handled it
        squash_now <= 1'b0;
    end
end

// ##################################################
//              PIPELINE STAGE: EXECUTE
// ##################################################

wire [31:0] ex_result = id_ex_is_lui   ? id_ex_imm :
                        id_ex_is_auipc ? (id_ex_pc + id_ex_imm) :
                        (id_ex_is_jal || id_ex_is_jalr) ? (id_ex_pc + 4) :
                        alu_out;

always @(posedge clk) begin
    if (~rstn) begin
        ex_mem_valid <= 1'b0;
        ex_mem_pc <= 32'b0;
        ex_mem_alu_result <= 32'b0;
        ex_mem_rs2_data <= 32'b0;
        ex_mem_rd <= 5'b0;
        ex_mem_funct3 <= 3'b0;
        ex_mem_is_load <= 1'b0;
        ex_mem_is_store <= 1'b0;
        ex_mem_reg_write <= 1'b0;
    end else if (!mem_stall && !ex_stall) begin
        ex_mem_pc <= id_ex_pc;
        ex_mem_alu_result <= ex_result;
        ex_mem_rs2_data <= fwd_rs2_data;
        ex_mem_rd <= id_ex_rd;
        ex_mem_funct3 <= id_ex_funct3;
        ex_mem_is_load <= id_ex_is_load;
        ex_mem_is_store <= id_ex_is_store;
        ex_mem_reg_write <= id_ex_reg_write && !id_ex_is_branch && !id_ex_is_store;
        ex_mem_valid <= id_ex_valid && !id_ex_is_branch;
    end
end

// ##################################################
//              PIPELINE STAGE: MEMORY
// ##################################################

// Combinational load data extraction from mem_rdata
// This allows WB stage to use the correct data immediately
reg [31:0] mem_load_data;
always @* begin
    case (ex_mem_funct3)
        3'b000: case (ex_mem_alu_result[1:0])  // LB (signed)
            2'b00: mem_load_data = {{24{mem_rdata[7]}}, mem_rdata[7:0]};
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

always @(posedge clk) begin
    if (~rstn) begin
        mem_op_pending <= 1'b0;
        mem_data_out_r <= 32'b0;
        mem_wstrb_r <= 4'b1111;
    end else begin
        // Start mem_op_pending when:
        // - Not currently pending
        // - mem_busy is false (AXI bus available - either idle or just completed)
        // This allows stores to be queued while waiting for fetch to complete
        if (ex_mem_valid && (ex_mem_is_load || ex_mem_is_store) && !mem_op_pending && !mem_busy) begin
            // mem_addr assignment removed
            // mem_wen assignment removed
            // mem_req assignment removed
            mem_op_pending <= 1'b1;
            
            if (ex_mem_is_store) begin
                case (ex_mem_funct3[1:0])
                    2'b00: begin
                        mem_data_out_r <= {4{ex_mem_rs2_data[7:0]}};
                        mem_wstrb_r <= 4'b0001 << ex_mem_alu_result[1:0];
                    end
                    2'b01: begin
                        mem_data_out_r <= {2{ex_mem_rs2_data[15:0]}};
                        mem_wstrb_r <= 4'b0011 << ex_mem_alu_result[1:0];
                    end
                    default: begin
                        mem_data_out_r <= ex_mem_rs2_data;
                        mem_wstrb_r <= 4'b1111;
                    end
                endcase
            end
        end else if (mem_op_pending && mem_ready) begin
            mem_op_pending <= 1'b0;
            // mem_req removal
        end else if (!mem_op_pending) begin
            // mem_req removal
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
        mem_wb_pc <= 32'b0;
        mem_wb_rd <= 5'b0;
        mem_wb_reg_write <= 1'b0;
    end else if (!ex_stall || (mem_op_pending && mem_ready)) begin
        mem_wb_rd <= ex_mem_rd;
        mem_wb_pc <= ex_mem_pc;
        mem_wb_reg_write <= ex_mem_reg_write && !ex_mem_is_store;
        mem_wb_valid <= ex_mem_valid && !ex_mem_is_store;
        
        if (ex_mem_is_load && mem_op_pending && mem_ready) begin
            mem_wb_result <= mem_load_data;
            // synthesis translate_off
            // Debug print block removed
            // synthesis translate_on
        end else begin
            mem_wb_result <= ex_mem_alu_result;
        end
    end else begin
        mem_wb_valid <= 1'b0;
        mem_wb_reg_write <= 1'b0; // IMPORTANT: Must clear to prevent incorrect forwarding
        mem_wb_rd <= 5'b0;        // Safer to clear destination too
    end
end

// synthesis translate_off
// Debug blocks removed
// synthesis translate_on

// ##################################################
//           STATE FOR TESTBENCH COMPATIBILITY
// ##################################################

localparam N_STATES = 5;
localparam STATE_FETCH_b = 0;
localparam STATE_DECODE_b = 1;
localparam STATE_EXECUTE_b = 2;
localparam STATE_MEM_b = 3;
localparam STATE_WRITE_b = 4;

wire [N_STATES-1:0] state;

assign state = {mem_wb_valid, ex_mem_valid, id_ex_valid, if_id_valid, fetch_wait};

// Unified Memory Request Logic (Arbiter)
// mem_addr is defined as reg above but driven combinationally here.
// IMPORTANT: Don't assert mem_req when mem_ready is high to avoid race condition
// where the AXI master starts a new transaction while we're processing the old one.
always @* begin
    if (mem_op_pending && !mem_ready) begin
        mem_req_comb = 1'b1;
        mem_wen_comb = ex_mem_is_store;
        mem_addr = ex_mem_alu_result;
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
