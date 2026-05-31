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
//                    Z-Core Top Model
//
// DE10-Lite board wrapper for the Z-Core RV32IM SoC.
// Peripherals on AXI-Lite bus:
//   M0: Block RAM 16 KB  @ 0x0000_0000
//   M1: UART       4 KB  @ 0x0400_0000
//   M2: GPIO       4 KB  @ 0x0400_1000
//   M3: Timer      4 KB  @ 0x0400_2000
//   M4: VGA        4 KB  @ 0x0400_3000
//   M5: SDRAM     64 MB  @ 0x1000_0000
// **************************************************

module z_core_top #(
    parameter DATA_WIDTH    = 32,
    parameter ADDR_WIDTH    = 32,
    parameter STRB_WIDTH    = (DATA_WIDTH/8),
    parameter N_GPIO        = 16,
    parameter PIPELINE_OUTPUT = 0,
    parameter INST_CACHE_DEPTH = 8192,
    parameter DATA_CACHE_DEPTH = 4096,
    // Bootloader MIF files (byte lanes 0-3)
    parameter INIT_FILE_0   = "software/bootloader_byte0.mif",
    parameter INIT_FILE_1   = "software/bootloader_byte1.mif",
    parameter INIT_FILE_2   = "software/bootloader_byte2.mif",
    parameter INIT_FILE_3   = "software/bootloader_byte3.mif"
)(
    // Board clock and reset
    input  wire        MAX10_CLK1_50,
    input  wire [1:0]  KEY,            // KEY[0] = active-low reset

    // LEDs
    output wire [9:0]  LEDR,

    // UART
    input  wire        uart_rx,
    output wire        uart_tx,

    // GPIO
    inout  wire [N_GPIO-1:0] gpio_pins,

    // VGA
    output wire [3:0]  VGA_R,
    output wire [3:0]  VGA_G,
    output wire [3:0]  VGA_B,
    output wire        VGA_HS,
    output wire        VGA_VS,

    // SDRAM (IS42S16320D on DE10-Lite)
    output wire [12:0] DRAM_ADDR,
    output wire [1:0]  DRAM_BA,
    output wire        DRAM_CAS_N,
    output wire        DRAM_CKE,
    output wire        DRAM_CLK,
    output wire        DRAM_CS_N,
    inout  wire [15:0] DRAM_DQ,
    output wire        DRAM_LDQM,
    output wire        DRAM_RAS_N,
    output wire        DRAM_UDQM,
    output wire        DRAM_WE_N
);

wire clk  = MAX10_CLK1_50;
wire rstn = KEY[0];

// Heartbeat: bit 25 of a 26-bit counter at 50 MHz ≈ 0.74 Hz
reg [25:0] heartbeat;
always @(posedge clk) heartbeat <= heartbeat + 1;

assign LEDR[0] = rstn;               // System Active
assign LEDR[1] = 1'b0;
assign LEDR[2] = s_axil_arvalid;  // Instr Fetch Active
assign LEDR[3] = s_axil_arready;  // Data Write Active
assign LEDR[6:4] = 3'b0;
assign LEDR[7] = gpio_pins[0];
assign LEDR[8] = KEY[1];
assign LEDR[9] = heartbeat[25];      // Heartbeat

// **************************************************
//              AXI-Lite Interconnect
// **************************************************

localparam S_COUNT  = 1;
localparam M_COUNT  = 6;
localparam M_REGIONS = 1;

localparam [M_COUNT*ADDR_WIDTH-1:0] M_BASE_ADDR = {
    32'h1000_0000, // M5: SDRAM  (64 MB)
    32'h0400_3000, // M4: VGA    (4 KB)
    32'h0400_2000, // M3: Timer  (4 KB)
    32'h0400_1000, // M2: GPIO   (4 KB)
    32'h0400_0000, // M1: UART   (4 KB)
    32'h0000_0000  // M0: BRAM   (16 KB)
};

localparam [M_COUNT*32-1:0] M_ADDR_WIDTH_CONF = {
    32'd26, // M5: SDRAM  (64 MB = 2^26)
    32'd12, // M4: VGA    (4 KB  = 2^12)
    32'd12, // M3: Timer  (4 KB  = 2^12)
    32'd12, // M2: GPIO   (4 KB  = 2^12)
    32'd12, // M1: UART   (4 KB  = 2^12)
    32'd14  // M0: BRAM   (16 KB = 2^14)
};

// Slave-side wires (CPU → interconnect)
wire [S_COUNT*ADDR_WIDTH-1:0]  s_axil_awaddr;
wire [S_COUNT*3-1:0]           s_axil_awprot;
wire [S_COUNT-1:0]             s_axil_awvalid;
wire [S_COUNT-1:0]             s_axil_awready;
wire [S_COUNT*DATA_WIDTH-1:0]  s_axil_wdata;
wire [S_COUNT*STRB_WIDTH-1:0]  s_axil_wstrb;
wire [S_COUNT-1:0]             s_axil_wvalid;
wire [S_COUNT-1:0]             s_axil_wready;
wire [S_COUNT*2-1:0]           s_axil_bresp;
wire [S_COUNT-1:0]             s_axil_bvalid;
wire [S_COUNT-1:0]             s_axil_bready;
wire [S_COUNT*ADDR_WIDTH-1:0]  s_axil_araddr;
wire [S_COUNT*3-1:0]           s_axil_arprot;
wire [S_COUNT-1:0]             s_axil_arvalid;
wire [S_COUNT-1:0]             s_axil_arready;
wire [S_COUNT*DATA_WIDTH-1:0]  s_axil_rdata;
wire [S_COUNT*2-1:0]           s_axil_rresp;
wire [S_COUNT-1:0]             s_axil_rvalid;
wire [S_COUNT-1:0]             s_axil_rready;

// Master-side wires (interconnect → peripherals)
wire [M_COUNT*ADDR_WIDTH-1:0]  m_axil_awaddr;
wire [M_COUNT*3-1:0]           m_axil_awprot;
wire [M_COUNT-1:0]             m_axil_awvalid;
wire [M_COUNT-1:0]             m_axil_awready;
wire [M_COUNT*DATA_WIDTH-1:0]  m_axil_wdata;
wire [M_COUNT*STRB_WIDTH-1:0]  m_axil_wstrb;
wire [M_COUNT-1:0]             m_axil_wvalid;
wire [M_COUNT-1:0]             m_axil_wready;
wire [M_COUNT*2-1:0]           m_axil_bresp;
wire [M_COUNT-1:0]             m_axil_bvalid;
wire [M_COUNT-1:0]             m_axil_bready;
wire [M_COUNT*ADDR_WIDTH-1:0]  m_axil_araddr;
wire [M_COUNT*3-1:0]           m_axil_arprot;
wire [M_COUNT-1:0]             m_axil_arvalid;
wire [M_COUNT-1:0]             m_axil_arready;
wire [M_COUNT*DATA_WIDTH-1:0]  m_axil_rdata;
wire [M_COUNT*2-1:0]           m_axil_rresp;
wire [M_COUNT-1:0]             m_axil_rvalid;
wire [M_COUNT-1:0]             m_axil_rready;

axil_interconnect #(
    .S_COUNT    (S_COUNT),
    .M_COUNT    (M_COUNT),
    .DATA_WIDTH (DATA_WIDTH),
    .ADDR_WIDTH (ADDR_WIDTH),
    .STRB_WIDTH (STRB_WIDTH),
    .M_REGIONS  (M_REGIONS),
    .M_BASE_ADDR(M_BASE_ADDR),
    .M_ADDR_WIDTH(M_ADDR_WIDTH_CONF)
) u_interconnect (
    .clk  (clk),
    .rst  (~rstn),
    .s_axil_awaddr (s_axil_awaddr),
    .s_axil_awprot (s_axil_awprot),
    .s_axil_awvalid(s_axil_awvalid),
    .s_axil_awready(s_axil_awready),
    .s_axil_wdata  (s_axil_wdata),
    .s_axil_wstrb  (s_axil_wstrb),
    .s_axil_wvalid (s_axil_wvalid),
    .s_axil_wready (s_axil_wready),
    .s_axil_bresp  (s_axil_bresp),
    .s_axil_bvalid (s_axil_bvalid),
    .s_axil_bready (s_axil_bready),
    .s_axil_araddr (s_axil_araddr),
    .s_axil_arprot (s_axil_arprot),
    .s_axil_arvalid(s_axil_arvalid),
    .s_axil_arready(s_axil_arready),
    .s_axil_rdata  (s_axil_rdata),
    .s_axil_rresp  (s_axil_rresp),
    .s_axil_rvalid (s_axil_rvalid),
    .s_axil_rready (s_axil_rready),
    .m_axil_awaddr (m_axil_awaddr),
    .m_axil_awprot (m_axil_awprot),
    .m_axil_awvalid(m_axil_awvalid),
    .m_axil_awready(m_axil_awready),
    .m_axil_wdata  (m_axil_wdata),
    .m_axil_wstrb  (m_axil_wstrb),
    .m_axil_wvalid (m_axil_wvalid),
    .m_axil_wready (m_axil_wready),
    .m_axil_bresp  (m_axil_bresp),
    .m_axil_bvalid (m_axil_bvalid),
    .m_axil_bready (m_axil_bready),
    .m_axil_araddr (m_axil_araddr),
    .m_axil_arprot (m_axil_arprot),
    .m_axil_arvalid(m_axil_arvalid),
    .m_axil_arready(m_axil_arready),
    .m_axil_rdata  (m_axil_rdata),
    .m_axil_rresp  (m_axil_rresp),
    .m_axil_rvalid (m_axil_rvalid),
    .m_axil_rready (m_axil_rready)
);

// **************************************************
//              Control Unit  (AXI-Lite Master)
// **************************************************

wire timer_irq;

z_core_control_u #(
    .DATA_WIDTH       (DATA_WIDTH),
    .ADDR_WIDTH       (ADDR_WIDTH),
    .STRB_WIDTH       (STRB_WIDTH),
    .INST_CACHE_DEPTH (INST_CACHE_DEPTH),
    .DATA_CACHE_DEPTH (DATA_CACHE_DEPTH)
) u_control_unit (
    .clk  (clk),
    .rstn (rstn),
    .m_axil_awaddr (s_axil_awaddr),
    .m_axil_awprot (s_axil_awprot),
    .m_axil_awvalid(s_axil_awvalid),
    .m_axil_awready(s_axil_awready),
    .m_axil_wdata  (s_axil_wdata),
    .m_axil_wstrb  (s_axil_wstrb),
    .m_axil_wvalid (s_axil_wvalid),
    .m_axil_wready (s_axil_wready),
    .m_axil_bresp  (s_axil_bresp),
    .m_axil_bvalid (s_axil_bvalid),
    .m_axil_bready (s_axil_bready),
    .m_axil_araddr (s_axil_araddr),
    .m_axil_arprot (s_axil_arprot),
    .m_axil_arvalid(s_axil_arvalid),
    .m_axil_arready(s_axil_arready),
    .m_axil_rdata  (s_axil_rdata),
    .m_axil_rresp  (s_axil_rresp),
    .m_axil_rvalid (s_axil_rvalid),
    .m_axil_rready (s_axil_rready),
    .meip(1'b0),
    .mtip(timer_irq),
    .msip(1'b0)
);

// **************************************************
//              M0: Block RAM  16 KB  @ 0x0000_0000
// **************************************************

axil_ram #(
    .DATA_WIDTH    (DATA_WIDTH),
    .ADDR_WIDTH    (14),
    .STRB_WIDTH    (STRB_WIDTH),
    .PIPELINE_OUTPUT(PIPELINE_OUTPUT),
    .INIT_FILE_0   (INIT_FILE_0),
    .INIT_FILE_1   (INIT_FILE_1),
    .INIT_FILE_2   (INIT_FILE_2),
    .INIT_FILE_3   (INIT_FILE_3)
) u_memory (
    .clk  (clk),
    .rstn (rstn),
    .s_axil_awaddr (m_axil_awaddr [0*ADDR_WIDTH +: 14]),
    .s_axil_awprot (m_axil_awprot [0*3 +: 3]),
    .s_axil_awvalid(m_axil_awvalid[0]),
    .s_axil_awready(m_axil_awready[0]),
    .s_axil_wdata  (m_axil_wdata  [0*DATA_WIDTH +: DATA_WIDTH]),
    .s_axil_wstrb  (m_axil_wstrb  [0*STRB_WIDTH +: STRB_WIDTH]),
    .s_axil_wvalid (m_axil_wvalid [0]),
    .s_axil_wready (m_axil_wready [0]),
    .s_axil_bresp  (m_axil_bresp  [0*2 +: 2]),
    .s_axil_bvalid (m_axil_bvalid [0]),
    .s_axil_bready (m_axil_bready [0]),
    .s_axil_araddr (m_axil_araddr [0*ADDR_WIDTH +: 14]),
    .s_axil_arprot (m_axil_arprot [0*3 +: 3]),
    .s_axil_arvalid(m_axil_arvalid[0]),
    .s_axil_arready(m_axil_arready[0]),
    .s_axil_rdata  (m_axil_rdata  [0*DATA_WIDTH +: DATA_WIDTH]),
    .s_axil_rresp  (m_axil_rresp  [0*2 +: 2]),
    .s_axil_rvalid (m_axil_rvalid [0]),
    .s_axil_rready (m_axil_rready [0])
);

// **************************************************
//              M1: UART  4 KB  @ 0x0400_0000
// **************************************************

axil_uart #(
    .DATA_WIDTH(DATA_WIDTH),
    .ADDR_WIDTH(12),
    .STRB_WIDTH(STRB_WIDTH)
) u_uart (
    .clk(clk),
    .rst(~rstn),
    .s_axil_awaddr (m_axil_awaddr [1*ADDR_WIDTH +: 12]),
    .s_axil_awprot (m_axil_awprot [1*3 +: 3]),
    .s_axil_awvalid(m_axil_awvalid[1]),
    .s_axil_awready(m_axil_awready[1]),
    .s_axil_wdata  (m_axil_wdata  [1*DATA_WIDTH +: DATA_WIDTH]),
    .s_axil_wstrb  (m_axil_wstrb  [1*STRB_WIDTH +: STRB_WIDTH]),
    .s_axil_wvalid (m_axil_wvalid [1]),
    .s_axil_wready (m_axil_wready [1]),
    .s_axil_bresp  (m_axil_bresp  [1*2 +: 2]),
    .s_axil_bvalid (m_axil_bvalid [1]),
    .s_axil_bready (m_axil_bready [1]),
    .s_axil_araddr (m_axil_araddr [1*ADDR_WIDTH +: 12]),
    .s_axil_arprot (m_axil_arprot [1*3 +: 3]),
    .s_axil_arvalid(m_axil_arvalid[1]),
    .s_axil_arready(m_axil_arready[1]),
    .s_axil_rdata  (m_axil_rdata  [1*DATA_WIDTH +: DATA_WIDTH]),
    .s_axil_rresp  (m_axil_rresp  [1*2 +: 2]),
    .s_axil_rvalid (m_axil_rvalid [1]),
    .s_axil_rready (m_axil_rready [1]),
    .uart_tx(uart_tx),
    .uart_rx(uart_rx)
);

// **************************************************
//              M2: GPIO  4 KB  @ 0x0400_1000
// **************************************************

axil_gpio #(
    .DATA_WIDTH(DATA_WIDTH),
    .ADDR_WIDTH(12),
    .STRB_WIDTH(STRB_WIDTH),
    .N_GPIO    (N_GPIO)
) u_gpio (
    .clk(clk),
    .rst(~rstn),
    .s_axil_awaddr (m_axil_awaddr [2*ADDR_WIDTH +: 12]),
    .s_axil_awprot (m_axil_awprot [2*3 +: 3]),
    .s_axil_awvalid(m_axil_awvalid[2]),
    .s_axil_awready(m_axil_awready[2]),
    .s_axil_wdata  (m_axil_wdata  [2*DATA_WIDTH +: DATA_WIDTH]),
    .s_axil_wstrb  (m_axil_wstrb  [2*STRB_WIDTH +: STRB_WIDTH]),
    .s_axil_wvalid (m_axil_wvalid [2]),
    .s_axil_wready (m_axil_wready [2]),
    .s_axil_bresp  (m_axil_bresp  [2*2 +: 2]),
    .s_axil_bvalid (m_axil_bvalid [2]),
    .s_axil_bready (m_axil_bready [2]),
    .s_axil_araddr (m_axil_araddr [2*ADDR_WIDTH +: 12]),
    .s_axil_arprot (m_axil_arprot [2*3 +: 3]),
    .s_axil_arvalid(m_axil_arvalid[2]),
    .s_axil_arready(m_axil_arready[2]),
    .s_axil_rdata  (m_axil_rdata  [2*DATA_WIDTH +: DATA_WIDTH]),
    .s_axil_rresp  (m_axil_rresp  [2*2 +: 2]),
    .s_axil_rvalid (m_axil_rvalid [2]),
    .s_axil_rready (m_axil_rready [2]),
    .gpio(gpio_pins)
);

// **************************************************
//              M3: Timer  4 KB  @ 0x0400_2000
// **************************************************

axil_timer #(
    .DATA_WIDTH(DATA_WIDTH),
    .ADDR_WIDTH(12),
    .STRB_WIDTH(STRB_WIDTH)
) u_timer (
    .clk (clk),
    .rstn(rstn),
    .ext_event_i(1'b0),
    .s_axil_awaddr (m_axil_awaddr [3*ADDR_WIDTH +: 12]),
    .s_axil_awprot (m_axil_awprot [3*3 +: 3]),
    .s_axil_awvalid(m_axil_awvalid[3]),
    .s_axil_awready(m_axil_awready[3]),
    .s_axil_wdata  (m_axil_wdata  [3*DATA_WIDTH +: DATA_WIDTH]),
    .s_axil_wstrb  (m_axil_wstrb  [3*STRB_WIDTH +: STRB_WIDTH]),
    .s_axil_wvalid (m_axil_wvalid [3]),
    .s_axil_wready (m_axil_wready [3]),
    .s_axil_bresp  (m_axil_bresp  [3*2 +: 2]),
    .s_axil_bvalid (m_axil_bvalid [3]),
    .s_axil_bready (m_axil_bready [3]),
    .s_axil_araddr (m_axil_araddr [3*ADDR_WIDTH +: 12]),
    .s_axil_arprot (m_axil_arprot [3*3 +: 3]),
    .s_axil_arvalid(m_axil_arvalid[3]),
    .s_axil_arready(m_axil_arready[3]),
    .s_axil_rdata  (m_axil_rdata  [3*DATA_WIDTH +: DATA_WIDTH]),
    .s_axil_rresp  (m_axil_rresp  [3*2 +: 2]),
    .s_axil_rvalid (m_axil_rvalid [3]),
    .s_axil_rready (m_axil_rready [3]),
    .timer_irq_o(timer_irq)
);

// **************************************************
//              M4: VGA  4 KB  @ 0x0400_3000
// **************************************************

axil_vga #(
    .DATA_WIDTH(DATA_WIDTH),
    .ADDR_WIDTH(12),
    .STRB_WIDTH(STRB_WIDTH),
    .FB_WIDTH (320),
    .FB_HEIGHT(200)
) u_vga (
    .clk(clk),
    .rst(~rstn),
    .vga_r (VGA_R),
    .vga_g (VGA_G),
    .vga_b (VGA_B),
    .vga_hs(VGA_HS),
    .vga_vs(VGA_VS),
    .s_axil_awaddr (m_axil_awaddr [4*ADDR_WIDTH +: 12]),
    .s_axil_awprot (m_axil_awprot [4*3 +: 3]),
    .s_axil_awvalid(m_axil_awvalid[4]),
    .s_axil_awready(m_axil_awready[4]),
    .s_axil_wdata  (m_axil_wdata  [4*DATA_WIDTH +: DATA_WIDTH]),
    .s_axil_wstrb  (m_axil_wstrb  [4*STRB_WIDTH +: STRB_WIDTH]),
    .s_axil_wvalid (m_axil_wvalid [4]),
    .s_axil_wready (m_axil_wready [4]),
    .s_axil_bresp  (m_axil_bresp  [4*2 +: 2]),
    .s_axil_bvalid (m_axil_bvalid [4]),
    .s_axil_bready (m_axil_bready [4]),
    .s_axil_araddr (m_axil_araddr [4*ADDR_WIDTH +: 12]),
    .s_axil_arprot (m_axil_arprot [4*3 +: 3]),
    .s_axil_arvalid(m_axil_arvalid[4]),
    .s_axil_arready(m_axil_arready[4]),
    .s_axil_rdata  (m_axil_rdata  [4*DATA_WIDTH +: DATA_WIDTH]),
    .s_axil_rresp  (m_axil_rresp  [4*2 +: 2]),
    .s_axil_rvalid (m_axil_rvalid [4]),
    .s_axil_rready (m_axil_rready [4])
);

// **************************************************
//              M5: SDRAM  64 MB  @ 0x1000_0000
// **************************************************

wire sdram_clk_out; // 100 MHz from internal PLL — unused externally

sdram_axi_top #(
    .AXI_ADDR_WIDTH(26),
    .AXI_DATA_WIDTH(DATA_WIDTH)
) u_sdram (
    .clk_50mhz(clk),
    .rst_n    (rstn),
    // Write address
    .s_axi_awaddr (m_axil_awaddr [5*ADDR_WIDTH +: 26]),
    .s_axi_awvalid(m_axil_awvalid[5]),
    .s_axi_awready(m_axil_awready[5]),
    // Write data
    .s_axi_wdata  (m_axil_wdata  [5*DATA_WIDTH +: DATA_WIDTH]),
    .s_axi_wstrb  (m_axil_wstrb  [5*STRB_WIDTH +: STRB_WIDTH]),
    .s_axi_wvalid (m_axil_wvalid [5]),
    .s_axi_wready (m_axil_wready [5]),
    // Write response
    .s_axi_bresp  (m_axil_bresp  [5*2 +: 2]),
    .s_axi_bvalid (m_axil_bvalid [5]),
    .s_axi_bready (m_axil_bready [5]),
    // Read address
    .s_axi_araddr (m_axil_araddr [5*ADDR_WIDTH +: 26]),
    .s_axi_arvalid(m_axil_arvalid[5]),
    .s_axi_arready(m_axil_arready[5]),
    // Read data
    .s_axi_rdata  (m_axil_rdata  [5*DATA_WIDTH +: DATA_WIDTH]),
    .s_axi_rresp  (m_axil_rresp  [5*2 +: 2]),
    .s_axi_rvalid (m_axil_rvalid [5]),
    .s_axi_rready (m_axil_rready [5]),
    // SDRAM pins
    .DRAM_ADDR(DRAM_ADDR),
    .DRAM_BA  (DRAM_BA),
    .DRAM_CAS_N(DRAM_CAS_N),
    .DRAM_CKE (DRAM_CKE),
    .DRAM_CLK (DRAM_CLK),
    .DRAM_CS_N(DRAM_CS_N),
    .DRAM_DQ  (DRAM_DQ),
    .DRAM_LDQM(DRAM_LDQM),
    .DRAM_RAS_N(DRAM_RAS_N),
    .DRAM_UDQM(DRAM_UDQM),
    .DRAM_WE_N(DRAM_WE_N),
    // Debug outputs (unused)
    .debug_state   (),
    .debug_busy    (),
    .debug_wr_full (),
    .debug_rd_empty(),
    .debug_wr_count(),
    .debug_rd_count(),
    .debug_error   (),
    .clk_sdram     (sdram_clk_out)
);

endmodule
