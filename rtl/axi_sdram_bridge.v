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
module axi_sdram_bridge #(
    parameter AXI_ADDR_WIDTH    = 26,
    parameter AXI_DATA_WIDTH    = 32,
    parameter SDRAM_DATA_WIDTH  = 16,
    parameter SDRAM_ADDR_WIDTH  = 25
)(
    input  wire                         clk,
    input  wire                         rst_n,

    // ---- AXI4-Lite Slave ----
    input  wire [AXI_ADDR_WIDTH-1:0]    s_axi_awaddr,
    input  wire                         s_axi_awvalid,
    output reg                          s_axi_awready,
    input  wire [AXI_DATA_WIDTH-1:0]    s_axi_wdata,
    input  wire [3:0]                   s_axi_wstrb,
    input  wire                         s_axi_wvalid,
    output reg                          s_axi_wready,
    output reg  [1:0]                   s_axi_bresp,
    output reg                          s_axi_bvalid,
    input  wire                         s_axi_bready,
    input  wire [AXI_ADDR_WIDTH-1:0]    s_axi_araddr,
    input  wire                         s_axi_arvalid,
    output reg                          s_axi_arready,
    output reg  [AXI_DATA_WIDTH-1:0]    s_axi_rdata,
    output reg  [1:0]                   s_axi_rresp,
    output reg                          s_axi_rvalid,
    input  wire                         s_axi_rready,

    // ---- SDRAM Controller FIFO Interface ----
    output reg  [SDRAM_DATA_WIDTH-1:0]  sdram_wr_data,
    output reg                          sdram_wr,
    output reg  [SDRAM_ADDR_WIDTH-1:0]  sdram_wr_addr,
    output wire [SDRAM_ADDR_WIDTH-1:0]  sdram_wr_max_addr,
    output reg  [8:0]                   sdram_wr_length,
    output reg                          sdram_wr_load,
    input  wire                         sdram_wr_full,
    input  wire [15:0]                  sdram_wr_use,
    input  wire [15:0]                  sdram_wr_fifo_rdusedw,
    input  wire [SDRAM_DATA_WIDTH-1:0]  sdram_rd_data,
    output reg                          sdram_rd,
    output reg  [SDRAM_ADDR_WIDTH-1:0]  sdram_rd_addr,
    output wire [SDRAM_ADDR_WIDTH-1:0]  sdram_rd_max_addr,
    output reg  [8:0]                   sdram_rd_length,
    output reg                          sdram_rd_load,
    input  wire                         sdram_rd_empty,
    input  wire [15:0]                  sdram_rd_use,

    // ---- Debug ----
    output wire [4:0]                   debug_state,
    output wire                         debug_busy,
    output wire                         debug_wr_full,
    output wire                         debug_rd_empty,
    output reg  [31:0]                  debug_wr_count,
    output reg  [31:0]                  debug_rd_count,
    output reg                          debug_error
);

    // ----------------------------------------------------------------
    // Fixed controller parameters
    // ----------------------------------------------------------------
    assign sdram_wr_max_addr = {SDRAM_ADDR_WIDTH{1'b1}};
    assign sdram_rd_max_addr = {SDRAM_ADDR_WIDTH{1'b1}};

    assign debug_wr_full  = sdram_wr_full;
    assign debug_rd_empty = sdram_rd_empty;

    // ----------------------------------------------------------------
    // Timing parameters
    // ----------------------------------------------------------------
    localparam [3:0]  FIFO_RECOVERY = 4'd8;
    localparam [15:0] INIT_WAIT     = 16'd30000;
    localparam [15:0] DRAIN_TIMEOUT = 16'd5000;

    // ----------------------------------------------------------------
    // State encoding (5-bit, 29 states)
    // ----------------------------------------------------------------
    // Init
    localparam [4:0] ST_RESET         = 5'd0;
    localparam [4:0] ST_INIT_LOAD     = 5'd1;
    localparam [4:0] ST_INIT_FIFO_REC = 5'd2;
    localparam [4:0] ST_INIT_PUSH_LO  = 5'd3;
    localparam [4:0] ST_INIT_PUSH_HI  = 5'd4;
    localparam [4:0] ST_INIT_DRAIN    = 5'd5;
    localparam [4:0] ST_IDLE          = 5'd6;
    // Write path (6 states)
    localparam [4:0] ST_WR_LOAD       = 5'd7;
    localparam [4:0] ST_WR_FIFO_REC   = 5'd8;
    localparam [4:0] ST_WR_LO         = 5'd9;
    localparam [4:0] ST_WR_HI         = 5'd10;
    localparam [4:0] ST_WR_DRAIN      = 5'd11;
    localparam [4:0] ST_WR_RESP       = 5'd12;
    // Read path (8 states — 4-stage pop pipeline)
    localparam [4:0] ST_RD_LOAD       = 5'd13;
    localparam [4:0] ST_RD_FIFO_REC   = 5'd14;
    localparam [4:0] ST_RD_WAIT       = 5'd15;
    localparam [4:0] ST_RD_POP_LO     = 5'd16;  // assert rdreq (word 0)
    localparam [4:0] ST_RD_POP_HI     = 5'd17;  // assert rdreq (word 1); FIFO sees req 0
    localparam [4:0] ST_RD_LATCH_LO   = 5'd18;  // word 0 valid → latch; FIFO sees req 1
    localparam [4:0] ST_RD_LATCH_HI   = 5'd19;  // word 1 valid → assemble
    localparam [4:0] ST_RD_RESP       = 5'd20;
    // RMW path (8 states — same 4-stage pop pipeline)
    localparam [4:0] ST_RMW_RD_LOAD   = 5'd21;
    localparam [4:0] ST_RMW_FIFO_REC  = 5'd22;
    localparam [4:0] ST_RMW_RD_WAIT   = 5'd23;
    localparam [4:0] ST_RMW_POP_LO    = 5'd24;
    localparam [4:0] ST_RMW_POP_HI    = 5'd25;
    localparam [4:0] ST_RMW_LATCH_LO  = 5'd26;
    localparam [4:0] ST_RMW_LATCH_HI  = 5'd27;
    localparam [4:0] ST_RMW_MERGE     = 5'd28;

    reg [4:0] state, state_next;

    // ----------------------------------------------------------------
    // Latches & counters
    // ----------------------------------------------------------------
    reg [AXI_DATA_WIDTH-1:0]    latched_wdata;
    reg [3:0]                   latched_wstrb;
    reg [SDRAM_ADDR_WIDTH-1:0]  latched_sdram_addr;
    reg [SDRAM_DATA_WIDTH-1:0]  read_lo;
    reg [15:0]                  init_cnt;
    reg [3:0]                   fifo_rec_cnt;
    reg [15:0]                  drain_cnt;

    // ----------------------------------------------------------------
    // Debug counters
    // ----------------------------------------------------------------
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            debug_wr_count <= 32'd0;
            debug_rd_count <= 32'd0;
            debug_error    <= 1'b0;
        end else begin
            if (state == ST_WR_RESP && s_axi_bready)
                debug_wr_count <= debug_wr_count + 1;
            if (state == ST_RD_RESP && s_axi_rready)
                debug_rd_count <= debug_rd_count + 1;
            if (drain_cnt >= DRAIN_TIMEOUT)
                debug_error <= 1'b1;
        end
    end

    assign debug_state = state;
    assign debug_busy  = (state != ST_IDLE);

    // ----------------------------------------------------------------
    // State register
    // ----------------------------------------------------------------
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) state <= ST_RESET;
        else        state <= state_next;
    end

    // ----------------------------------------------------------------
    // Init counter
    // ----------------------------------------------------------------
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            init_cnt <= 16'd0;
        else if (state == ST_RESET && init_cnt < INIT_WAIT)
            init_cnt <= init_cnt + 1'b1;
    end

    // ----------------------------------------------------------------
    // FIFO recovery counter
    // ----------------------------------------------------------------
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            fifo_rec_cnt <= 4'd0;
        else if (state == ST_INIT_FIFO_REC || state == ST_WR_FIFO_REC ||
                 state == ST_RD_FIFO_REC   || state == ST_RMW_FIFO_REC)
            fifo_rec_cnt <= fifo_rec_cnt + 1'b1;
        else
            fifo_rec_cnt <= 4'd0;
    end

    // ----------------------------------------------------------------
    // Drain / wait counter
    // ----------------------------------------------------------------
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n)
            drain_cnt <= 16'd0;
        else if (state == ST_INIT_DRAIN || state == ST_WR_DRAIN ||
                 state == ST_RD_WAIT    || state == ST_RMW_RD_WAIT)
            drain_cnt <= drain_cnt + 1'b1;
        else
            drain_cnt <= 16'd0;
    end

    // ----------------------------------------------------------------
    // Next-state logic (combinational)
    // ----------------------------------------------------------------
    always @(*) begin
        state_next = state;
        case (state)
            // === Init ===
            ST_RESET:
                if (init_cnt >= INIT_WAIT)
                    state_next = ST_INIT_LOAD;

            ST_INIT_LOAD:
                state_next = ST_INIT_FIFO_REC;

            ST_INIT_FIFO_REC:
                if (fifo_rec_cnt >= FIFO_RECOVERY)
                    state_next = ST_INIT_PUSH_LO;

            ST_INIT_PUSH_LO:
                if (!sdram_wr_full)
                    state_next = ST_INIT_PUSH_HI;

            ST_INIT_PUSH_HI:
                if (!sdram_wr_full)
                    state_next = ST_INIT_DRAIN;

            ST_INIT_DRAIN:
                if (sdram_wr_fifo_rdusedw == 0 && drain_cnt > 16'd10)
                    state_next = ST_IDLE;
                else if (drain_cnt >= DRAIN_TIMEOUT)
                    state_next = ST_IDLE;

            // === Idle ===
            ST_IDLE: begin
                if (s_axi_awvalid && s_axi_wvalid) begin
                    if (s_axi_wstrb == 4'hF)
                        state_next = ST_WR_LOAD;
                    else
                        state_next = ST_RMW_RD_LOAD;
                end
                else if (s_axi_arvalid)
                    state_next = ST_RD_LOAD;
            end

            // === Full write ===
            ST_WR_LOAD:      state_next = ST_WR_FIFO_REC;
            ST_WR_FIFO_REC:
                if (fifo_rec_cnt >= FIFO_RECOVERY)
                    state_next = ST_WR_LO;
            ST_WR_LO:
                if (!sdram_wr_full) state_next = ST_WR_HI;
            ST_WR_HI:
                if (!sdram_wr_full) state_next = ST_WR_DRAIN;
            ST_WR_DRAIN:
                if (sdram_wr_fifo_rdusedw == 0 && drain_cnt > 16'd4)
                    state_next = ST_WR_RESP;
                else if (drain_cnt >= DRAIN_TIMEOUT)
                    state_next = ST_WR_RESP;
            ST_WR_RESP:
                if (s_axi_bready) state_next = ST_IDLE;

            // === Read (4-stage pop pipeline) ===
            ST_RD_LOAD:      state_next = ST_RD_FIFO_REC;
            ST_RD_FIFO_REC:
                if (fifo_rec_cnt >= FIFO_RECOVERY)
                    state_next = ST_RD_WAIT;
            ST_RD_WAIT:
                if (sdram_rd_use >= 16'd2)
                    state_next = ST_RD_POP_LO;
                else if (drain_cnt >= DRAIN_TIMEOUT)
                    state_next = ST_RD_RESP;
            ST_RD_POP_LO:   state_next = ST_RD_POP_HI;
            ST_RD_POP_HI:   state_next = ST_RD_LATCH_LO;
            ST_RD_LATCH_LO: state_next = ST_RD_LATCH_HI;
            ST_RD_LATCH_HI: state_next = ST_RD_RESP;
            ST_RD_RESP:
                if (s_axi_rready) state_next = ST_IDLE;

            // === RMW read phase (same 4-stage pipeline) ===
            ST_RMW_RD_LOAD:  state_next = ST_RMW_FIFO_REC;
            ST_RMW_FIFO_REC:
                if (fifo_rec_cnt >= FIFO_RECOVERY)
                    state_next = ST_RMW_RD_WAIT;
            ST_RMW_RD_WAIT:
                if (sdram_rd_use >= 16'd2)
                    state_next = ST_RMW_POP_LO;
                else if (drain_cnt >= DRAIN_TIMEOUT)
                    state_next = ST_RMW_MERGE;
            ST_RMW_POP_LO:  state_next = ST_RMW_POP_HI;
            ST_RMW_POP_HI:  state_next = ST_RMW_LATCH_LO;
            ST_RMW_LATCH_LO: state_next = ST_RMW_LATCH_HI;
            ST_RMW_LATCH_HI: state_next = ST_RMW_MERGE;
            ST_RMW_MERGE:    state_next = ST_WR_LOAD;

            default: state_next = ST_RESET;
        endcase
    end

    // ----------------------------------------------------------------
    // Datapath / outputs (registered)
    // ----------------------------------------------------------------
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            s_axi_awready      <= 0;
            s_axi_wready       <= 0;
            s_axi_arready      <= 0;
            s_axi_bvalid       <= 0;
            s_axi_rvalid       <= 0;
            s_axi_bresp        <= 2'b00;
            s_axi_rresp        <= 2'b00;
            s_axi_rdata        <= 0;
            sdram_wr           <= 0;
            sdram_rd           <= 0;
            sdram_wr_load      <= 0;
            sdram_rd_load      <= 0;
            sdram_wr_data      <= 0;
            sdram_wr_addr      <= 0;
            sdram_rd_addr      <= 0;
            sdram_wr_length    <= 9'd2;   // STATIC: always 2
            sdram_rd_length    <= 9'd0;
            latched_wdata      <= 0;
            latched_wstrb      <= 0;
            latched_sdram_addr <= 0;
            read_lo            <= 0;
        end else begin
            // Default: de-assert one-cycle pulses
            sdram_wr      <= 0;
            sdram_rd      <= 0;
            sdram_wr_load <= 0;
            sdram_rd_load <= 0;
            s_axi_awready <= 0;
            s_axi_wready  <= 0;
            s_axi_arready <= 0;

            case (state)
                // ====================================================
                // INIT
                // ====================================================
                ST_INIT_LOAD: begin
                    sdram_wr_addr <= 0;
                    sdram_wr_load <= 1;
                    sdram_rd_addr <= 0;
                    sdram_rd_load <= 1;
                end

                ST_INIT_PUSH_LO: begin
                    if (!sdram_wr_full) begin
                        sdram_wr_data <= 16'h0000;
                        sdram_wr      <= 1;
                    end
                end

                ST_INIT_PUSH_HI: begin
                    if (!sdram_wr_full) begin
                        sdram_wr_data <= 16'h0000;
                        sdram_wr      <= 1;
                    end
                end

                // ====================================================
                // IDLE
                // ====================================================
                ST_IDLE: begin
                    s_axi_bvalid    <= 0;
                    s_axi_rvalid    <= 0;
                    sdram_rd_length <= 9'd0;  // prevent spurious reads

                    if (s_axi_awvalid && s_axi_wvalid) begin
                        latched_wdata      <= s_axi_wdata;
                        latched_wstrb      <= s_axi_wstrb;
                        latched_sdram_addr <= {s_axi_awaddr[SDRAM_ADDR_WIDTH:2], 1'b0};
                        s_axi_awready      <= 1;
                        s_axi_wready       <= 1;
                    end else if (s_axi_arvalid) begin
                        latched_sdram_addr <= {s_axi_araddr[SDRAM_ADDR_WIDTH:2], 1'b0};
                        s_axi_arready      <= 1;
                    end
                end

                // ====================================================
                // FULL WRITE
                // ====================================================
                ST_WR_LOAD: begin
                    sdram_wr_addr <= latched_sdram_addr;
                    sdram_wr_load <= 1;
                end

                ST_WR_LO: begin
                    if (!sdram_wr_full) begin
                        sdram_wr_data <= latched_wdata[15:0];
                        sdram_wr      <= 1;
                    end
                end

                ST_WR_HI: begin
                    if (!sdram_wr_full) begin
                        sdram_wr_data <= latched_wdata[31:16];
                        sdram_wr      <= 1;
                    end
                end

                ST_WR_RESP: begin
                    s_axi_bvalid <= 1;
                    s_axi_bresp  <= 2'b00;
                end

                // ====================================================
                // READ — 4-stage FIFO pop pipeline
                //
                // Timing (all on same 100 MHz clock):
                //   Cycle T+0 [POP_LO]  : sdram_rd<=1 (registered)
                //   Cycle T+1 [POP_HI]  : sdram_rd<=1; FIFO sees 1st rdreq
                //   Cycle T+2 [LATCH_LO]: q[] has word 0 → latch read_lo
                //                         FIFO sees 2nd rdreq
                //   Cycle T+3 [LATCH_HI]: q[] has word 1 → assemble rdata
                // ====================================================
                ST_RD_LOAD: begin
                    sdram_rd_addr   <= latched_sdram_addr;
                    sdram_rd_load   <= 1;
                    sdram_rd_length <= 9'd2;
                end

                ST_RD_POP_LO: begin
                    sdram_rd        <= 1;     // 1st rdreq (registered)
                    sdram_rd_length <= 9'd0;  // prevent spurious re-reads
                end

                ST_RD_POP_HI: begin
                    sdram_rd <= 1;            // 2nd rdreq (registered)
                    // FIFO now sees 1st rdreq — word 0 appears next cycle
                end

                ST_RD_LATCH_LO: begin
                    // Word 0 is NOW valid on q[] — latch it
                    read_lo <= sdram_rd_data;
                    // FIFO now sees 2nd rdreq — word 1 appears next cycle
                end

                ST_RD_LATCH_HI: begin
                    // Word 1 is NOW valid on q[] — assemble 32-bit result
                    s_axi_rdata <= {sdram_rd_data, read_lo};
                end

                ST_RD_RESP: begin
                    s_axi_rvalid <= 1;
                    s_axi_rresp  <= 2'b00;
                end

                // ====================================================
                // RMW — read phase uses same 4-stage pipeline
                // ====================================================
                ST_RMW_RD_LOAD: begin
                    sdram_rd_addr   <= latched_sdram_addr;
                    sdram_rd_load   <= 1;
                    sdram_rd_length <= 9'd2;
                end

                ST_RMW_POP_LO: begin
                    sdram_rd        <= 1;
                    sdram_rd_length <= 9'd0;
                end

                ST_RMW_POP_HI: begin
                    sdram_rd <= 1;
                end

                ST_RMW_LATCH_LO: begin
                    read_lo <= sdram_rd_data;
                end

                ST_RMW_LATCH_HI: begin
                    s_axi_rdata <= {sdram_rd_data, read_lo};
                end

                ST_RMW_MERGE: begin
                    if (latched_wstrb[0]) latched_wdata[7:0]   <= latched_wdata[7:0];
                    else                  latched_wdata[7:0]   <= s_axi_rdata[7:0];
                    if (latched_wstrb[1]) latched_wdata[15:8]  <= latched_wdata[15:8];
                    else                  latched_wdata[15:8]  <= s_axi_rdata[15:8];
                    if (latched_wstrb[2]) latched_wdata[23:16] <= latched_wdata[23:16];
                    else                  latched_wdata[23:16] <= s_axi_rdata[23:16];
                    if (latched_wstrb[3]) latched_wdata[31:24] <= latched_wdata[31:24];
                    else                  latched_wdata[31:24] <= s_axi_rdata[31:24];
                end
            endcase
        end
    end

endmodule
