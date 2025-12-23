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
//                AXI-Lite GPIO (Bidirectional)
//  - 64 GPIOs (configurable via N_GPIO)
//  - Address Map (Assuming 32-bit Data Bus):
//      Offset 0x00: DATA[31:0]  (Read: Pin State, Write: Output Latch)
//      Offset 0x04: DATA[63:32]
//      Offset 0x08: DIR[31:0]   (0 = Input/High-Z, 1 = Output)
//      Offset 0x0C: DIR[63:32]
// **************************************************
`timescale 1ns / 1ps

module axil_gpio #
(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter STRB_WIDTH = (DATA_WIDTH/8),
    parameter N_GPIO     = 64
)
(
    input  wire                   clk,
    input  wire                   rst,

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

    // Bidirectional GPIOs
    inout  wire [N_GPIO-1:0]      gpio
);

    // =========================================================================
    // Registers & Wires
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

    // GPIO Internal Registers
    // Ensure registers are wide enough to prevent "Index out of range" errors
    // during elaboration even for unused byte lanes.
    localparam REG_WIDTH = 64;
    
    reg [REG_WIDTH-1:0] gpio_data_out; // Stores value to drive when DIR=1
    reg [REG_WIDTH-1:0] gpio_dir;      // 1 = Output, 0 = Input

    // Padded Input for safe reading
    wire [REG_WIDTH-1:0] gpio_in_padded;
    generate
        if (REG_WIDTH > N_GPIO) begin
            assign gpio_in_padded = {{REG_WIDTH-N_GPIO{1'b0}}, gpio};
        end else begin
            assign gpio_in_padded = gpio;
        end
    endgenerate

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
    // IO Logic (Tri-state)
    // =========================================================================
    // If bit i is Output (dir=1), drive switch. If Input (dir=0), Float (z).
    genvar i;
    generate
        for (i = 0; i < N_GPIO; i = i + 1) begin : gpio_io_buffers
            assign gpio[i] = gpio_dir[i] ? gpio_data_out[i] : 1'bz;
        end
    endgenerate

    // =========================================================================
    // Write Channel Logic
    // =========================================================================
    always @(posedge clk) begin
        if (rst) begin
            s_axil_awready_reg <= 1'b0;
            s_axil_wready_reg  <= 1'b0;
            s_axil_bvalid_reg  <= 1'b0;
            axi_awready_flag   <= 1'b0;
            axi_wready_flag    <= 1'b0;
            axi_awaddr         <= {ADDR_WIDTH{1'b0}};
            axi_wdata          <= {DATA_WIDTH{1'b0}};
            axi_wstrb          <= {STRB_WIDTH{1'b0}};
            
            // Defaut: All Inputs (Safe state), Data Out 0
            gpio_data_out      <= {REG_WIDTH{1'b0}};
            gpio_dir           <= {REG_WIDTH{1'b0}}; 
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

                // Decode Address
                // 0x00: Data Low, 0x04: Data High
                // 0x08: Dir Low,  0x0C: Dir High
                
                case (axi_awaddr[3:2])
                    2'b00: begin // 0x00: DATA[31:0]
                        if (axi_wstrb[0]) gpio_data_out[7:0]   <= axi_wdata[7:0];
                        if (axi_wstrb[1]) gpio_data_out[15:8]  <= axi_wdata[15:8];
                        if (axi_wstrb[2]) gpio_data_out[23:16] <= axi_wdata[23:16];
                        if (axi_wstrb[3]) gpio_data_out[31:24] <= axi_wdata[31:24];
                    end
                    2'b01: begin // 0x04: DATA[63:32]
                         if (N_GPIO > 32) begin
                            if (axi_wstrb[0]) if(N_GPIO>32) gpio_data_out[39:32] <= axi_wdata[7:0];
                            if (axi_wstrb[1]) if(N_GPIO>40) gpio_data_out[47:40] <= axi_wdata[15:8];
                            if (axi_wstrb[2]) if(N_GPIO>48) gpio_data_out[55:48] <= axi_wdata[23:16];
                            if (axi_wstrb[3]) if(N_GPIO>56) gpio_data_out[63:56] <= axi_wdata[31:24];
                        end
                    end
                    2'b10: begin // 0x08: DIR[31:0]
                        if (axi_wstrb[0]) gpio_dir[7:0]   <= axi_wdata[7:0];
                        if (axi_wstrb[1]) gpio_dir[15:8]  <= axi_wdata[15:8];
                        if (axi_wstrb[2]) gpio_dir[23:16] <= axi_wdata[23:16];
                        if (axi_wstrb[3]) gpio_dir[31:24] <= axi_wdata[31:24];
                    end
                    2'b11: begin // 0x0C: DIR[63:32]
                        if (N_GPIO > 32) begin
                            if (axi_wstrb[0]) if(N_GPIO>32) gpio_dir[39:32] <= axi_wdata[7:0];
                            if (axi_wstrb[1]) if(N_GPIO>40) gpio_dir[47:40] <= axi_wdata[15:8];
                            if (axi_wstrb[2]) if(N_GPIO>48) gpio_dir[55:48] <= axi_wdata[23:16];
                            if (axi_wstrb[3]) if(N_GPIO>56) gpio_dir[63:56] <= axi_wdata[31:24];
                        end
                    end
                endcase

            end else if (s_axil_bvalid_reg && s_axil_bready) begin
                s_axil_bvalid_reg <= 1'b0;
            end
        end
    end

    // =========================================================================
    // Read Channel Logic
    // =========================================================================
    always @(posedge clk) begin
        if (rst) begin
            s_axil_arready_reg <= 1'b0;
            s_axil_rvalid_reg  <= 1'b0;
            s_axil_rdata_reg   <= {DATA_WIDTH{1'b0}};
        end else begin
            if (~s_axil_arready_reg && s_axil_arvalid && ~s_axil_rvalid_reg) begin
                s_axil_arready_reg <= 1'b1;
                
                // Read Logic
                case (s_axil_araddr[3:2])
                    2'b00: begin // 0x00: Read DATA[31:0] (Sampled from IO Pins)
                        s_axil_rdata_reg <= gpio_in_padded[31:0];
                    end
                    2'b01: begin // 0x04: Read DATA[63:32]
                        s_axil_rdata_reg <= 32'b0;
                        if (N_GPIO > 32) begin
                            // Safe read using padded wire
                             s_axil_rdata_reg <= gpio_in_padded[63:32];
                        end
                    end
                    2'b10: begin // 0x08: Read DIR[31:0]
                        s_axil_rdata_reg <= gpio_dir[31:0];
                    end
                    2'b11: begin // 0x0C: Read DIR[63:32]
                        // Note: gpio_dir is REG_WIDTH=64 bits wide, so [63:32] always valid
                        s_axil_rdata_reg <= gpio_dir[63:32];
                    end
                    default: s_axil_rdata_reg <= 32'b0;
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

endmodule
