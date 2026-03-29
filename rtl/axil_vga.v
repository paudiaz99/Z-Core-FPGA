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
//         AXI-Lite VGA Controller
//   160x120 framebuffer, 4x upscaled to 640x480
//   8-bit color (3-3-2 RGB)
//   DE10-Lite 4-bit resistor DAC
// **************************************************

module axil_vga #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 12,
    parameter STRB_WIDTH = (DATA_WIDTH/8),
    parameter FB_WIDTH   = 160,
    parameter FB_HEIGHT  = 120
)(
    input  wire                   clk,
    input  wire                   rst,

    // VGA output
    output reg  [3:0]             vga_r,
    output reg  [3:0]             vga_g,
    output reg  [3:0]             vga_b,
    output reg                    vga_hs,
    output reg                    vga_vs,

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
    input  wire                   s_axil_rready
);

// **************************************************
//           Register Map
// **************************************************
// 0x00: FB_ADDR   [R/W] - Framebuffer write address (0..19199)
// 0x04: FB_DATA   [W]   - Write pixel color, auto-increment addr
// 0x08: FB_STATUS [R]   - Bit 0: in vertical blanking

localparam REG_ADDR   = 3'b000;  // 0x00
localparam REG_DATA   = 3'b001;  // 0x04
localparam REG_STATUS = 3'b010;  // 0x08

// **************************************************
//    VGA Timing — 640x480 @ 60 Hz, 25 MHz pixel clk
// **************************************************

localparam H_SYNC  = 96;
localparam H_BACK  = 48;
localparam H_DISP  = 640;
localparam H_FRONT = 16;
localparam H_TOTAL = 800;

localparam V_SYNC  = 2;
localparam V_BACK  = 33;
localparam V_DISP  = 480;
localparam V_FRONT = 10;
localparam V_TOTAL = 525;

localparam H_START = H_SYNC + H_BACK;   // 144
localparam H_END   = H_START + H_DISP;  // 784
localparam V_START = V_SYNC + V_BACK;   // 35
localparam V_END   = V_START + V_DISP;  // 515

localparam FB_SIZE = FB_WIDTH * FB_HEIGHT;  // 19200

// **************************************************
//            Framebuffer (dual-port M9K)
// **************************************************
//  Port A — CPU write  (system clock)
//  Port B — VGA read   (system clock)

(* ramstyle = "M9K" *) reg [7:0] framebuffer [0:FB_SIZE-1];

// **************************************************
//        25 MHz pixel clock enable
// **************************************************

reg pixel_en;
always @(posedge clk) begin
    if (rst) pixel_en <= 1'b0;
    else     pixel_en <= ~pixel_en;
end

// **************************************************
//           VGA timing counters
// **************************************************

reg [9:0] h_count;
reg [9:0] v_count;

always @(posedge clk) begin
    if (rst) begin
        h_count <= 10'd0;
        v_count <= 10'd0;
    end else if (pixel_en) begin
        if (h_count == H_TOTAL - 1) begin
            h_count <= 10'd0;
            if (v_count == V_TOTAL - 1)
                v_count <= 10'd0;
            else
                v_count <= v_count + 1'd1;
        end else begin
            h_count <= h_count + 1'd1;
        end
    end
end

// **************************************************
//        Sync signals (active-low)
// **************************************************

always @(posedge clk) begin
    if (rst) begin
        vga_hs <= 1'b1;
        vga_vs <= 1'b1;
    end else if (pixel_en) begin
        vga_hs <= (h_count >= H_SYNC);
        vga_vs <= (v_count >= V_SYNC);
    end
end

// **************************************************
//    Active display region & framebuffer read
// **************************************************

wire h_active = (h_count >= H_START) && (h_count < H_END);
wire v_active = (v_count >= V_START) && (v_count < V_END);
wire active   = h_active && v_active;

wire in_vblank = !v_active;

// Framebuffer coordinates (4x upscale: divide by 4)
wire [7:0] fb_x = (h_count - H_START) >> 2;
wire [6:0] fb_y = (v_count - V_START) >> 2;

// y * 160 = y * 128 + y * 32 = (y << 7) + (y << 5)
wire [14:0] fb_rd_addr = active
    ? ({1'b0, fb_y, 7'd0} + {3'd0, fb_y, 5'd0} + {7'd0, fb_x})
    : 15'd0;

// Registered read — 1-cycle latency
reg [7:0] pixel_data;
always @(posedge clk) begin
    pixel_data <= framebuffer[fb_rd_addr];
end

// Delay active flag to match read latency
reg active_d;
always @(posedge clk) begin
    active_d <= active;
end

// **************************************************
//   RGB output — 8-bit (3-3-2) → 4-bit DAC
// **************************************************
// R[7:5] → 4-bit : {R[7:5], R[7]}
// G[4:2] → 4-bit : {G[4:2], G[4]}
// B[1:0] → 4-bit : {B[1:0], B[1:0]}

always @(posedge clk) begin
    if (active_d) begin
        vga_r <= {pixel_data[7:5], pixel_data[7]};
        vga_g <= {pixel_data[4:2], pixel_data[4]};
        vga_b <= {pixel_data[1:0], pixel_data[1:0]};
    end else begin
        vga_r <= 4'd0;
        vga_g <= 4'd0;
        vga_b <= 4'd0;
    end
end

// **************************************************
//       AXI-Lite Interface Logic
// **************************************************

reg s_axil_awready_reg = 0;
reg s_axil_wready_reg  = 0;
reg s_axil_bvalid_reg  = 0;
reg s_axil_arready_reg = 0;
reg s_axil_rvalid_reg  = 0;
reg [DATA_WIDTH-1:0] s_axil_rdata_reg = 0;

reg [ADDR_WIDTH-1:0] write_addr_reg;
reg [DATA_WIDTH-1:0] write_data_reg;

reg [ADDR_WIDTH-1:0] read_addr_reg;

// CPU-side framebuffer write address (auto-incrementing)
reg [14:0] fb_wr_addr;

assign s_axil_awready = s_axil_awready_reg;
assign s_axil_wready  = s_axil_wready_reg;
assign s_axil_bresp   = 2'b00;
assign s_axil_bvalid  = s_axil_bvalid_reg;
assign s_axil_arready = s_axil_arready_reg;
assign s_axil_rdata   = s_axil_rdata_reg;
assign s_axil_rresp   = 2'b00;
assign s_axil_rvalid  = s_axil_rvalid_reg;

// Write Channel
always @(posedge clk) begin
    if (rst) begin
        s_axil_awready_reg <= 0;
        s_axil_wready_reg  <= 0;
        s_axil_bvalid_reg  <= 0;
        write_addr_reg     <= 0;
        write_data_reg     <= 0;
        fb_wr_addr         <= 15'd0;
    end else begin
        // Address Handshake
        if (s_axil_awvalid && !s_axil_awready_reg && (!s_axil_bvalid_reg || s_axil_bready)) begin
            s_axil_awready_reg <= 1;
            write_addr_reg <= s_axil_awaddr;
        end else begin
            s_axil_awready_reg <= 0;
        end

        // Data Handshake
        if (s_axil_wvalid && !s_axil_wready_reg && (!s_axil_bvalid_reg || s_axil_bready)) begin
            s_axil_wready_reg <= 1;
            write_data_reg <= s_axil_wdata;
        end else begin
            s_axil_wready_reg <= 0;
        end

        // Write Response and Register Update
        if (s_axil_awready_reg && s_axil_wready_reg) begin
            s_axil_bvalid_reg <= 1;

            case (write_addr_reg[3:2])
                2'b00: begin // FB_ADDR (0x00)
                    fb_wr_addr <= write_data_reg[14:0];
                end
                2'b01: begin // FB_DATA (0x04)
                    framebuffer[fb_wr_addr] <= write_data_reg[7:0];
                    if (fb_wr_addr < FB_SIZE - 1)
                        fb_wr_addr <= fb_wr_addr + 1'd1;
                    else
                        fb_wr_addr <= 15'd0;
                end
            endcase
        end else if (s_axil_bready && s_axil_bvalid_reg) begin
            s_axil_bvalid_reg <= 0;
        end
    end
end

// Read Channel
always @(posedge clk) begin
    if (rst) begin
        s_axil_arready_reg <= 0;
        s_axil_rvalid_reg  <= 0;
        s_axil_rdata_reg   <= 0;
        read_addr_reg      <= 0;
    end else begin
        if (s_axil_arvalid && !s_axil_arready_reg && (!s_axil_rvalid_reg || s_axil_rready)) begin
            s_axil_arready_reg <= 1;
            read_addr_reg <= s_axil_araddr;
        end else begin
            s_axil_arready_reg <= 0;
        end

        if (s_axil_arready_reg) begin
            s_axil_rvalid_reg <= 1;

            case (read_addr_reg[3:2])
                2'b00: s_axil_rdata_reg <= {17'd0, fb_wr_addr};       // FB_ADDR
                2'b10: s_axil_rdata_reg <= {31'd0, in_vblank};        // FB_STATUS
                default: s_axil_rdata_reg <= 32'd0;
            endcase
        end else if (s_axil_rready && s_axil_rvalid_reg) begin
            s_axil_rvalid_reg <= 0;
        end
    end
end

endmodule
