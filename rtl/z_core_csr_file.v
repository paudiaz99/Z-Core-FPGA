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
//              Z-Core CSR Register File
//       RISC-V Privileged Architecture v1.12
//              Machine Mode (M-mode) Only
// **************************************************
//
// Implements the Zicsr extension CSR read/write logic.
// The control unit performs the read-modify-write
// operation and drives csr_write_data with the final
// value to write.
//
// Trap entry/exit (mepc, mcause, mstatus updates on
// trap) is handled by the control unit via dedicated
// trap_enter / mret ports.
//

module z_core_csr_file #(
    parameter DATA_WIDTH = 32
) (
    input  wire clk,
    input  wire rstn,

    // ============================================
    // CSR Read/Write Interface (from pipeline EX stage)
    // ============================================
    input  wire [11:0]          csr_addr,        // CSR address (inst[31:20])
    input  wire [DATA_WIDTH-1:0] csr_write_data,  // Data to write (after RMW in control unit)
    input  wire                 csr_wen,          // Write enable
    output reg  [DATA_WIDTH-1:0] csr_read_data,   // Combinational read output

    // ============================================
    // Trap Interface (from control unit)
    // ============================================
    input  wire                 trap_enter,       // Trap entry pulse
    input  wire [DATA_WIDTH-1:0] trap_mepc,       // PC to save on trap entry
    input  wire [DATA_WIDTH-1:0] trap_mcause,     // Cause code on trap entry
    input  wire [DATA_WIDTH-1:0] trap_mtval,      // Trap value (faulting addr/insn)

    input  wire                 mret_exec,        // MRET execution pulse

    // ============================================
    // Interrupt Pending Inputs (directly wired)
    // ============================================
    input  wire                 meip,             // Machine External Interrupt Pending
    input  wire                 mtip,             // Machine Timer Interrupt Pending
    input  wire                 msip,             // Machine Software Interrupt Pending

    // ============================================
    // Instruction Retired Pulse (from pipeline WB)
    // ============================================
    input  wire                 instret_pulse,    // Pulse when instruction retires

    // ============================================
    //           Cache Hit/Miss Counters
    // ============================================
    input  wire                 inst_cache_hit_pulse,    // Pulse when cache hit
    input  wire                 inst_cache_miss_pulse,    // Pulse when cache miss
    input  wire                 data_cache_hit_pulse,    // Pulse when cache hit

    // ============================================
    //           Memory Access Counters
    // ============================================
    input  wire                 mem_read_pulse,   // Pulse when memory read
    input  wire                 mem_write_pulse,  // Pulse when memory write
    input  wire                 mem_inst_fetch_pulse,  // Pulse when memory instruction fetch

    // ============================================
    //           Branch Misprediction Counters
    // ============================================
    input  wire                 branch_misprediction_pulse,  // Pulse when branch misprediction

    // ============================================
    //           Pipeline Flush Counters
    // ============================================
    input  wire                 pipeline_flush_pulse,  // Pulse when pipeline flush

    // ============================================
    // CSR Outputs (directly used by control unit)
    // ============================================
    output wire                 mstatus_mie,      // Global interrupt enable (mstatus.MIE)
    output wire [DATA_WIDTH-1:0] mtvec_out,       // Trap vector base address
    output wire [DATA_WIDTH-1:0] mepc_out,        // Exception PC
    output wire                 irq_pending,       // Any enabled interrupt is pending
    output wire                 mie_meie_out,      // Machine External Interrupt Enable
    output wire                 mie_mtie_out,      // Machine Timer Interrupt Enable
    output wire                 mie_msie_out       // Machine Software Interrupt Enable
);

    // =========================================================================
    //  CSR Address Map (RISC-V Privileged Spec v1.12, Table 2.2 - 2.6)
    // =========================================================================
    localparam ADDR_MSTATUS    = 12'h300;
    localparam ADDR_MISA       = 12'h301;
    localparam ADDR_MIE        = 12'h304;
    localparam ADDR_MTVEC      = 12'h305;
    localparam ADDR_MSCRATCH   = 12'h340;
    localparam ADDR_MEPC       = 12'h341;
    localparam ADDR_MCAUSE     = 12'h342;
    localparam ADDR_MTVAL      = 12'h343;
    localparam ADDR_MIP        = 12'h344;

    // Machine Information Registers (Read-Only)
    localparam ADDR_MVENDORID  = 12'hF11;
    localparam ADDR_MARCHID    = 12'hF12;
    localparam ADDR_MIMPID     = 12'hF13;
    localparam ADDR_MHARTID    = 12'hF14;

    // Machine Counter/Timer (Read-Only from M-mode for now)
    localparam ADDR_MCYCLE     = 12'hB00;
    localparam ADDR_MCYCLEH    = 12'hB80;
    localparam ADDR_MINSTRET   = 12'hB02;
    localparam ADDR_MINSTRETH  = 12'hB82;

    // User-visible counter aliases (Read-Only)
    localparam ADDR_CYCLE      = 12'hC00;
    localparam ADDR_CYCLEH     = 12'hC80;
    localparam ADDR_INSTRET    = 12'hC02;
    localparam ADDR_INSTRETH   = 12'hC82;
    localparam ADDR_MHPMCOUNTER3 = 12'hC03;
    localparam ADDR_MHPMCOUNTER3H = 12'hC83;
    localparam ADDR_MHPMCOUNTER4 = 12'hC04;
    localparam ADDR_MHPMCOUNTER4H = 12'hC84;
    localparam ADDR_MHPMCOUNTER5 = 12'hC05;
    localparam ADDR_MHPMCOUNTER5H = 12'hC85;
    localparam ADDR_MHPMCOUNTER6 = 12'hC06;
    localparam ADDR_MHPMCOUNTER6H = 12'hC86;
    localparam ADDR_MHPMCOUNTER7 = 12'hC07;
    localparam ADDR_MHPMCOUNTER7H = 12'hC87;
    localparam ADDR_MHPMCOUNTER8 = 12'hC08;
    localparam ADDR_MHPMCOUNTER8H = 12'hC88;
    localparam ADDR_MHPMCOUNTER9 = 12'hC09;
    localparam ADDR_MHPMCOUNTER9H = 12'hC89;
    localparam ADDR_MHPMCOUNTER10 = 12'hC0A;
    localparam ADDR_MHPMCOUNTER10H = 12'hC8A;

    // =========================================================================
    //  CSR Registers
    // =========================================================================

    // --- mstatus (Machine Status) ---
    // RV32 mstatus layout (Privileged Spec v1.12 §3.1.6):
    //   Bit  3: MIE  (Machine Interrupt Enable)
    //   Bit  7: MPIE (Machine Previous Interrupt Enable)
    //   Bits 12:11: MPP (Machine Previous Privilege) - hardwired 2'b11 (M-mode only)
    //
    // All other bits are hardwired to 0 for M-mode-only implementation.
    reg        mstatus_mie_r;   // Bit 3
    reg        mstatus_mpie_r;  // Bit 7

    // Compose full mstatus read value
    wire [DATA_WIDTH-1:0] mstatus_val = {
        19'b0,                  // Bits 31:13 = 0
        2'b11,                  // Bits 12:11 = MPP (hardwired M-mode)
        3'b0,                   // Bits 10:8 = 0
        mstatus_mpie_r,         // Bit 7 = MPIE
        3'b0,                   // Bits 6:4 = 0
        mstatus_mie_r,          // Bit 3 = MIE
        3'b0                    // Bits 2:0 = 0
    };

    // --- misa (Machine ISA) ---
    // RV32IM + Zicsr: MXL=1 (32-bit), Extensions: I(bit 8) + M(bit 12)
    wire [DATA_WIDTH-1:0] misa_val = {
        2'b01,                  // MXL = 1 (XLEN=32)
        4'b0,                   // Bits 29:26 = 0
        26'b00_0000_0000_0001_0001_0000_0000  // I(bit 8) + M(bit 12)
    };

    // --- mie (Machine Interrupt Enable) ---
    // Bit 3:  MSIE (Machine Software Interrupt Enable)
    // Bit 7:  MTIE (Machine Timer Interrupt Enable)
    // Bit 11: MEIE (Machine External Interrupt Enable)
    reg mie_msie;
    reg mie_mtie;
    reg mie_meie;

    wire [DATA_WIDTH-1:0] mie_val = {
        20'b0,                  // Bits 31:12 = 0
        mie_meie,               // Bit 11 = MEIE
        3'b0,                   // Bits 10:8 = 0
        mie_mtie,               // Bit 7 = MTIE
        3'b0,                   // Bits 6:4 = 0
        mie_msie,               // Bit 3 = MSIE
        3'b0                    // Bits 2:0 = 0
    };

    // --- mip (Machine Interrupt Pending) ---
    // These are directly wired from external sources (read-only in this implementation)
    // Bit 3:  MSIP
    // Bit 7:  MTIP
    // Bit 11: MEIP
    wire [DATA_WIDTH-1:0] mip_val = {
        20'b0,
        meip,                   // Bit 11 = MEIP
        3'b0,
        mtip,                   // Bit 7 = MTIP
        3'b0,
        msip,                   // Bit 3 = MSIP
        3'b0
    };

    // --- mtvec (Machine Trap-Vector Base-Address) ---
    // Bits [31:2]: BASE (4-byte aligned trap handler address)
    // Bits [1:0]:  MODE (0 = Direct, 1 = Vectored)
    reg [DATA_WIDTH-1:0] mtvec_r;

    // --- mscratch (Machine Scratch Register) ---
    reg [DATA_WIDTH-1:0] mscratch_r;

    // --- mepc (Machine Exception Program Counter) ---
    // Always 4-byte aligned for non-C extension: bit 1:0 are 0
    reg [DATA_WIDTH-1:0] mepc_r;

    // --- mcause (Machine Cause Register) ---
    // Bit 31: Interrupt (1) or Exception (0)
    // Bits 30:0: Exception/Interrupt code
    reg [DATA_WIDTH-1:0] mcause_r;

    // --- mtval (Machine Trap Value) ---
    reg [DATA_WIDTH-1:0] mtval_r;

    // --- Performance Counters (Write Any, Read Legal (WARL) Registers) ---
    reg [63:0] mcycle_r;
    reg [63:0] minstret_r;
    reg [63:0] mhpmcounter3_r; // I-Cache Hits
    reg [63:0] mhpmcounter4_r; // D-Cache Hits
    reg [63:0] mhpmcounter5_r; // Load Requests 
    reg [63:0] mhpmcounter6_r; // Store Requests
    reg [63:0] mhpmcounter7_r; // Branch Mispredictions
    reg [63:0] mhpmcounter8_r; // Pipeline Flushes
    reg [63:0] mhpmcounter9_r; // I-Cache Misses 
    reg [63:0] mhpmcounter10_r; // Memory Instruction Fetches

    // =========================================================================
    //  Output Assignments
    // =========================================================================

    assign mstatus_mie = mstatus_mie_r;
    assign mtvec_out   = mtvec_r;
    assign mepc_out    = mepc_r;
    assign mie_meie_out = mie_meie;
    assign mie_mtie_out = mie_mtie;
    assign mie_msie_out = mie_msie;

    // Interrupt pending: any enabled interrupt that is pending, gated by global MIE
    assign irq_pending = mstatus_mie_r & (
        (mie_meie & meip) |   // External interrupt
        (mie_mtie & mtip) |   // Timer interrupt
        (mie_msie & msip)     // Software interrupt
    );

    // =========================================================================
    //  Combinational Read Logic
    // =========================================================================

    always @(*) begin
        case (csr_addr)
            ADDR_MSTATUS:   csr_read_data = mstatus_val;
            ADDR_MISA:      csr_read_data = misa_val;
            ADDR_MIE:       csr_read_data = mie_val;
            ADDR_MTVEC:     csr_read_data = mtvec_r;
            ADDR_MSCRATCH:  csr_read_data = mscratch_r;
            ADDR_MEPC:      csr_read_data = mepc_r;
            ADDR_MCAUSE:    csr_read_data = mcause_r;
            ADDR_MTVAL:     csr_read_data = mtval_r;
            ADDR_MIP:       csr_read_data = mip_val;

            // Read-Only Machine Information
            ADDR_MVENDORID: csr_read_data = 32'h0;
            ADDR_MARCHID:   csr_read_data = 32'h0;
            ADDR_MIMPID:    csr_read_data = 32'h0;
            ADDR_MHARTID:   csr_read_data = 32'h0;

            // Performance Counters
            ADDR_MCYCLE,
            ADDR_CYCLE:     csr_read_data = mcycle_r[31:0];
            ADDR_MCYCLEH,
            ADDR_CYCLEH:    csr_read_data = mcycle_r[63:32];
            ADDR_MINSTRET,
            ADDR_INSTRET:   csr_read_data = minstret_r[31:0];
            ADDR_MINSTRETH,
            ADDR_INSTRETH:  csr_read_data = minstret_r[63:32];
            ADDR_MHPMCOUNTER3: csr_read_data = mhpmcounter3_r[31:0];
            ADDR_MHPMCOUNTER3H: csr_read_data = mhpmcounter3_r[63:32];
            ADDR_MHPMCOUNTER4: csr_read_data = mhpmcounter4_r[31:0];
            ADDR_MHPMCOUNTER4H: csr_read_data = mhpmcounter4_r[63:32];
            ADDR_MHPMCOUNTER5: csr_read_data = mhpmcounter5_r[31:0];
            ADDR_MHPMCOUNTER5H: csr_read_data = mhpmcounter5_r[63:32];
            ADDR_MHPMCOUNTER6: csr_read_data = mhpmcounter6_r[31:0];
            ADDR_MHPMCOUNTER6H: csr_read_data = mhpmcounter6_r[63:32];
            ADDR_MHPMCOUNTER7: csr_read_data = mhpmcounter7_r[31:0];
            ADDR_MHPMCOUNTER7H: csr_read_data = mhpmcounter7_r[63:32];
            ADDR_MHPMCOUNTER8: csr_read_data = mhpmcounter8_r[31:0];
            ADDR_MHPMCOUNTER8H: csr_read_data = mhpmcounter8_r[63:32];
            ADDR_MHPMCOUNTER9: csr_read_data = mhpmcounter9_r[31:0];
            ADDR_MHPMCOUNTER9H: csr_read_data = mhpmcounter9_r[63:32];
            ADDR_MHPMCOUNTER10: csr_read_data = mhpmcounter10_r[31:0];
            ADDR_MHPMCOUNTER10H: csr_read_data = mhpmcounter10_r[63:32];

            default:        csr_read_data = 32'h0;
        endcase
    end

    // =========================================================================
    //  Sequential Write Logic
    // =========================================================================

    always @(posedge clk) begin
        if (~rstn) begin
            // Reset values per RISC-V Privileged Spec
            mstatus_mie_r  <= 1'b0;
            mstatus_mpie_r <= 1'b0;
            mie_msie       <= 1'b0;
            mie_mtie       <= 1'b0;
            mie_meie       <= 1'b0;
            mtvec_r        <= 32'h0;
            mscratch_r     <= 32'h0;
            mepc_r         <= 32'h0;
            mcause_r       <= 32'h0;
            mtval_r        <= 32'h0;
            mcycle_r       <= 64'h0;
            minstret_r     <= 64'h0;
            mhpmcounter3_r <= 64'h0;
            mhpmcounter4_r <= 64'h0;
            mhpmcounter5_r <= 64'h0;
            mhpmcounter6_r <= 64'h0;
            mhpmcounter7_r <= 64'h0;
            mhpmcounter8_r <= 64'h0;
            mhpmcounter9_r <= 64'h0;
            mhpmcounter10_r <= 64'h0;
        end else begin

            // --- Always-running counters ---
            mcycle_r <= mcycle_r + 1;
            if (instret_pulse)
                minstret_r <= minstret_r + 1;
            if (inst_cache_hit_pulse)
                mhpmcounter3_r <= mhpmcounter3_r + 1;
            if(data_cache_hit_pulse)
                mhpmcounter4_r <= mhpmcounter4_r + 1;
            if (mem_read_pulse)
                mhpmcounter5_r <= mhpmcounter5_r + 1;
            if (mem_write_pulse)
                mhpmcounter6_r <= mhpmcounter6_r + 1;
            if (branch_misprediction_pulse)
                mhpmcounter7_r <= mhpmcounter7_r + 1;
            if (pipeline_flush_pulse)
                mhpmcounter8_r <= mhpmcounter8_r + 1;
            if (mem_inst_fetch_pulse)
                mhpmcounter10_r <= mhpmcounter10_r + 1;
            if (inst_cache_miss_pulse)
                mhpmcounter9_r <= mhpmcounter9_r + 1;

            // --- Trap Entry (highest priority over CSR writes) ---
            // Per Privileged Spec §3.1.6.1:
            //   mepc    <- trap_mepc (PC of interrupted/excepting instruction)
            //   mcause  <- trap_mcause
            //   MPIE    <- MIE
            //   MIE     <- 0 (disable interrupts)
            //   MPP     <- M (hardwired, no change needed)
            if (trap_enter) begin
                mepc_r         <= trap_mepc & 32'hFFFFFFFC; // Enforce alignment
                mcause_r       <= trap_mcause;
                mtval_r        <= trap_mtval;
                mstatus_mpie_r <= mstatus_mie_r;
                mstatus_mie_r  <= 1'b0;
            end

            // --- MRET Execution ---
            // Per Privileged Spec §3.1.6.1:
            //   MIE  <- MPIE
            //   MPIE <- 1
            //   MPP  <- M (hardwired, no change needed)
            else if (mret_exec) begin
                mstatus_mie_r  <= mstatus_mpie_r;
                mstatus_mpie_r <= 1'b1;
            end

            // --- Normal CSR Write (from CSRRW/CSRRS/CSRRC) ---
            else if (csr_wen) begin
                case (csr_addr)
                    ADDR_MSTATUS: begin
                        mstatus_mie_r  <= csr_write_data[3];
                        mstatus_mpie_r <= csr_write_data[7];
                        // MPP (bits 12:11) ignored - hardwired to M-mode
                    end
                    ADDR_MIE: begin
                        mie_msie <= csr_write_data[3];
                        mie_mtie <= csr_write_data[7];
                        mie_meie <= csr_write_data[11];
                    end
                    ADDR_MTVEC: begin
                        mtvec_r <= csr_write_data;
                    end
                    ADDR_MSCRATCH: begin
                        mscratch_r <= csr_write_data;
                    end
                    ADDR_MEPC: begin
                        mepc_r <= csr_write_data & 32'hFFFFFFFC; // Enforce alignment
                    end
                    ADDR_MCAUSE: begin
                        mcause_r <= csr_write_data;
                    end
                    ADDR_MTVAL: begin
                        mtval_r <= csr_write_data;
                    end
                    // ADDR_MIP: mip bits are read-only (externally driven)
                    // ADDR_MISA: read-only
                    // ADDR_MVENDORID, etc: read-only

                    ADDR_MCYCLE: begin
                        mcycle_r[31:0] <= csr_write_data;
                    end
                    ADDR_MCYCLEH: begin
                        mcycle_r[63:32] <= csr_write_data;
                    end
                    ADDR_MINSTRET: begin
                        minstret_r[31:0] <= csr_write_data;
                    end
                    ADDR_MINSTRETH: begin
                        minstret_r[63:32] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER3: begin
                        mhpmcounter3_r[31:0] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER3H: begin
                        mhpmcounter3_r[63:32] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER4: begin
                        mhpmcounter4_r[31:0] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER4H: begin
                        mhpmcounter4_r[63:32] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER5: begin
                        mhpmcounter5_r[31:0] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER5H: begin
                        mhpmcounter5_r[63:32] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER6: begin
                        mhpmcounter6_r[31:0] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER6H: begin
                        mhpmcounter6_r[63:32] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER7: begin
                        mhpmcounter7_r[31:0] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER7H: begin
                        mhpmcounter7_r[63:32] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER8: begin
                        mhpmcounter8_r[31:0] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER8H: begin
                        mhpmcounter8_r[63:32] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER9: begin
                        mhpmcounter9_r[31:0] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER9H: begin
                        mhpmcounter9_r[63:32] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER10: begin
                        mhpmcounter10_r[31:0] <= csr_write_data;
                    end
                    ADDR_MHPMCOUNTER10H: begin
                        mhpmcounter10_r[63:32] <= csr_write_data;
                    end
                    // default: ignore writes to unknown/read-only CSRs
                endcase
            end
        end
    end

endmodule