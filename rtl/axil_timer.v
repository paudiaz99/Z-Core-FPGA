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


module axil_timer #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter STRB_WIDTH = (DATA_WIDTH/8)
) 
(
    input  wire                   clk,
    input  wire                   rstn,

    // AXI-Lite Slave Interface
    input  wire [ADDR_WIDTH-1:0]  s_axil_awaddr,
    input  wire [2:0]             s_axil_awprot,
    input  wire                   s_axil_awvalid,
    output wire                   s_axil_awready,
    input  wire [DATA_WIDTH-1:0]  s_axil_wdata,
    input  wire [STRB_WIDTH-1:0]  s_axil_wstrb,
    input  wire                   s_axil_wvalid,
    output wire                   s_axil_wready,
    output wire [1:0]             s_axil_bresp,
    output wire                   s_axil_bvalid,
    input  wire                   s_axil_bready,
    input  wire [ADDR_WIDTH-1:0]  s_axil_araddr,
    input  wire [2:0]             s_axil_arprot,
    input  wire                   s_axil_arvalid,
    output wire                   s_axil_arready,
    output wire [DATA_WIDTH-1:0]  s_axil_rdata,
    output wire [1:0]             s_axil_rresp,
    output wire                   s_axil_rvalid,
    input  wire                   s_axil_rready,

    input wire ext_event_i,

    output wire timer_irq_o

);

    
    // =========================================================================
    // Memory Mapped Registers
    // =========================================================================
    
    wire [DATA_WIDTH-1:0] timer_lo;       // 0x00 -> Timer Low  (counter inside the timer)
    wire [DATA_WIDTH-1:0] timer_hi;       // 0x04 -> Timer High (counter inside the timer)
    reg  [DATA_WIDTH-1:0] timer_ctrl;     // 0x08 -> Timer Control
    reg  [DATA_WIDTH-1:0] timecmp_lo_r;   // 0x0C -> Compare Low  (new)
    reg  [DATA_WIDTH-1:0] timecmp_hi_r;   // 0x10 -> Compare High (new)

    // =========================================================================
    // Internal Wires and Registers
    // =========================================================================
    
    wire timer_lo_overflow;
    wire timer_hi_overflow;

    wire timer_lo_enable;
    wire timer_hi_enable;

    reg [DATA_WIDTH-1:0] timer_lo_load_val;
    reg [DATA_WIDTH-1:0] timer_hi_load_val;

    reg load_lo;
    reg load_hi;
    

    // =========================================================================
    // AXI-Lite Registers & Wires
    // =========================================================================

    // AXI-Lite Status
    reg s_axil_awready_reg;
    reg s_axil_wready_reg;
    reg s_axil_bvalid_reg;
    reg s_axil_arready_reg;
    reg [DATA_WIDTH-1:0] s_axil_rdata_reg;
    reg s_axil_rvalid_reg;

    // Latched Write Request
    reg [ADDR_WIDTH-1:0] axi_awaddr;
    reg axi_awready_flag;
    reg [DATA_WIDTH-1:0] axi_wdata;
    reg [STRB_WIDTH-1:0] axi_wstrb;
    reg axi_wready_flag;

    // Assignments
    assign s_axil_awready = s_axil_awready_reg;
    assign s_axil_wready  = s_axil_wready_reg;
    assign s_axil_bresp   = 2'b00; // OKAY
    assign s_axil_bvalid  = s_axil_bvalid_reg;
    assign s_axil_arready = s_axil_arready_reg;
    assign s_axil_rdata   = s_axil_rdata_reg;
    assign s_axil_rresp   = 2'b00; // OKAY
    assign s_axil_rvalid  = s_axil_rvalid_reg;


    // =========================================================================
    // Write Channel Logic
    // =========================================================================
    always @(posedge clk) begin
        if (~rstn) begin
            s_axil_awready_reg <= 1'b0;
            s_axil_wready_reg  <= 1'b0;
            s_axil_bvalid_reg  <= 1'b0;
            axi_awready_flag   <= 1'b0;
            axi_wready_flag    <= 1'b0;
            axi_awaddr         <= {ADDR_WIDTH{1'b0}};
            axi_wdata          <= {DATA_WIDTH{1'b0}};
            axi_wstrb          <= {STRB_WIDTH{1'b0}};
            timer_ctrl         <= {DATA_WIDTH{1'b0}};
            timecmp_lo_r       <= {DATA_WIDTH{1'b1}}; // Max value so IRQ is not immediately asserted
            timecmp_hi_r       <= {DATA_WIDTH{1'b1}};
        end else begin
            // Address Handshake
            if (~s_axil_awready_reg && s_axil_awvalid && ~axi_awready_flag && ~s_axil_bvalid_reg) begin
                s_axil_awready_reg <= 1'b1;
                axi_awaddr         <= s_axil_awaddr;
                axi_awready_flag   <= 1'b1;
            end else begin
                s_axil_awready_reg <= 1'b0;
            end

            // Data Handshake
            if (~s_axil_wready_reg && s_axil_wvalid && ~axi_wready_flag && ~s_axil_bvalid_reg) begin
                s_axil_wready_reg <= 1'b1;
                axi_wdata         <= s_axil_wdata;
                axi_wstrb         <= s_axil_wstrb;
                axi_wready_flag   <= 1'b1;
            end else begin
                s_axil_wready_reg <= 1'b0;
            end

            // Execution
            if (axi_awready_flag && axi_wready_flag && ~s_axil_bvalid_reg) begin
                s_axil_bvalid_reg <= 1'b1;
                axi_awready_flag  <= 1'b0;
                axi_wready_flag   <= 1'b0;

                case (axi_awaddr[4:2])
                    3'b000: begin // 0x00: timer_lo[31:0]
                        timer_lo_load_val <= axi_wdata;
                        load_lo <= 1'b1;
                    end
                    3'b001: begin // 0x04: timer_hi[63:32]
                        timer_hi_load_val <= axi_wdata;
                        load_hi <= 1'b1;
                    end
                    3'b010: begin // 0x08: timer_ctrl[31:0]
                        timer_ctrl <= axi_wdata;
                    end
                    3'b011: begin // 0x0C: timecmp_lo[31:0]
                        timecmp_lo_r <= axi_wdata;
                    end
                    3'b100: begin // 0x10: timecmp_hi[63:32]
                        timecmp_hi_r <= axi_wdata;
                    end
                endcase
            end else begin
                // Auto-clear load flags
                load_lo <= 1'b0;
                load_hi <= 1'b0;
            end 

            if (s_axil_bvalid_reg && s_axil_bready) begin
                s_axil_bvalid_reg <= 1'b0;
            end
        end
    end

    // =========================================================================
    // Read Channel Logic
    // =========================================================================
    always @(posedge clk) begin
        if (~rstn) begin
            s_axil_arready_reg <= 1'b0;
            s_axil_rvalid_reg  <= 1'b0;
            s_axil_rdata_reg   <= {DATA_WIDTH{1'b0}};
        end else begin
            if (~s_axil_arready_reg && s_axil_arvalid && ~s_axil_rvalid_reg) begin
                s_axil_arready_reg <= 1'b1;
                
                case (s_axil_araddr[4:2])
                    3'b000: s_axil_rdata_reg <= timer_lo;       // 0x00
                    3'b001: s_axil_rdata_reg <= timer_hi;       // 0x04
                    3'b010: s_axil_rdata_reg <= timer_ctrl;     // 0x08
                    3'b011: s_axil_rdata_reg <= timecmp_lo_r;   // 0x0C
                    3'b100: s_axil_rdata_reg <= timecmp_hi_r;   // 0x10
                    default: s_axil_rdata_reg <= {DATA_WIDTH{1'b0}};
                endcase
            end else begin
                s_axil_arready_reg <= 1'b0;
            end

            if (s_axil_arready_reg) begin
                s_axil_rvalid_reg <= 1'b1;
            end else if (s_axil_rvalid_reg && s_axil_rready) begin
                s_axil_rvalid_reg <= 1'b0;
            end
        end
    end

    // Timer Logic
    
    // Timer Control Register (Bits):
    // 0 -> Enable/Disable Timer
    // 1 -> Count Up / Count Down
    // 2 -> Timer / Counter Mode (0: Timer, 1: Counter)
    // 3 -> Interrupt Enable (gates timer_irq_o)

    assign timer_irq_o = timer_ctrl[3] & ({timer_hi, timer_lo} >= {timecmp_hi_r, timecmp_lo_r});

    // External Signal Edge Detection
    reg ext_event_r;
    always @(posedge clk) begin
        ext_event_r <= ext_event_i;
    end

    wire ext_event_edge = ~ext_event_r & ext_event_i;

    wire count_pulse = timer_ctrl[2] ? ext_event_edge : 1'b1;

    assign timer_lo_enable = timer_ctrl[0] & count_pulse;
    assign timer_hi_enable = timer_ctrl[0] & (timer_lo_overflow) & count_pulse;

    z_core_32b_timer timer_lo_inst (
        .clk(clk),
        .rstn(rstn),
        .enable(timer_lo_enable),
        .load(load_lo),
        .load_val(timer_lo_load_val),
        .count_up(timer_ctrl[1]),
        .overflow(timer_lo_overflow),
        .timer(timer_lo)
    );

    z_core_32b_timer timer_hi_inst (
        .clk(clk),
        .rstn(rstn),
        .enable(timer_hi_enable),
        .load(load_hi),
        .load_val(timer_hi_load_val),
        .count_up(timer_ctrl[1]),
        .overflow(timer_hi_overflow),
        .timer(timer_hi)
    );



endmodule