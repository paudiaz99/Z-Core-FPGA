/*

Copyright (c) 2018 Alex Forencich

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

// Language: Verilog 2001

/*
 * AXI4-Lite RAM — 4 byte-lane M9K architecture
 *
 * Memory is split into 4 separate 8-bit wide arrays so Quartus
 * can map each directly to M9K blocks with no bit-select writes.
 * Each byte lane has its own .mif file for initialization.
 */
module axil_ram #
(
    // Width of data bus in bits
    parameter DATA_WIDTH = 32,
    // Width of address bus in bits
    parameter ADDR_WIDTH = 32,
    // Width of wstrb (width of data bus in words)
    parameter STRB_WIDTH = (DATA_WIDTH/8),
    // Extra pipeline register on output
    parameter PIPELINE_OUTPUT = 0,
    // Per-byte-lane MIF files for M9K initialization
    parameter INIT_FILE_0 = "",
    parameter INIT_FILE_1 = "",
    parameter INIT_FILE_2 = "",
    parameter INIT_FILE_3 = ""
)
(
    input  wire                   clk,
    input  wire                   rstn,

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

localparam VALID_ADDR_WIDTH = ADDR_WIDTH - $clog2(STRB_WIDTH);

reg mem_wr_en;
reg mem_rd_en;

reg s_axil_awready_reg = 1'b0, s_axil_awready_next;
reg s_axil_wready_reg = 1'b0, s_axil_wready_next;
reg s_axil_bvalid_reg = 1'b0, s_axil_bvalid_next;
reg s_axil_arready_reg = 1'b0, s_axil_arready_next;
reg [DATA_WIDTH-1:0] s_axil_rdata_reg = {DATA_WIDTH{1'b0}};
reg s_axil_rvalid_reg = 1'b0, s_axil_rvalid_next;
reg [DATA_WIDTH-1:0] s_axil_rdata_pipe_reg = {DATA_WIDTH{1'b0}};
reg s_axil_rvalid_pipe_reg = 1'b0;

// =========================================================================
// Address decoding
// =========================================================================

wire [VALID_ADDR_WIDTH-1:0] s_axil_awaddr_valid = s_axil_awaddr >> (ADDR_WIDTH - VALID_ADDR_WIDTH);
wire [VALID_ADDR_WIDTH-1:0] s_axil_araddr_valid = s_axil_araddr >> (ADDR_WIDTH - VALID_ADDR_WIDTH);

// =========================================================================
// Byte-lane memories — each 8-bit wide for clean M9K mapping
// =========================================================================

(* ramstyle = "M9K", ram_init_file = INIT_FILE_0 *) reg [7:0] mem0 [(2**VALID_ADDR_WIDTH)-1:0];
(* ramstyle = "M9K", ram_init_file = INIT_FILE_1 *) reg [7:0] mem1 [(2**VALID_ADDR_WIDTH)-1:0];
(* ramstyle = "M9K", ram_init_file = INIT_FILE_2 *) reg [7:0] mem2 [(2**VALID_ADDR_WIDTH)-1:0];
(* ramstyle = "M9K", ram_init_file = INIT_FILE_3 *) reg [7:0] mem3 [(2**VALID_ADDR_WIDTH)-1:0];

// =========================================================================
// AXI-Lite output assignments
// =========================================================================

assign s_axil_awready = s_axil_awready_reg;
assign s_axil_wready = s_axil_wready_reg;
assign s_axil_bresp = 2'b00;
assign s_axil_bvalid = s_axil_bvalid_reg;
assign s_axil_arready = s_axil_arready_reg;
assign s_axil_rresp = 2'b00;

assign s_axil_rdata = PIPELINE_OUTPUT ? s_axil_rdata_pipe_reg : s_axil_rdata_reg;
assign s_axil_rvalid = PIPELINE_OUTPUT ? s_axil_rvalid_pipe_reg : s_axil_rvalid_reg;

// =========================================================================
// Write channel control
// =========================================================================

always @* begin
    mem_wr_en = 1'b0;

    s_axil_awready_next = 1'b0;
    s_axil_wready_next = 1'b0;
    s_axil_bvalid_next = s_axil_bvalid_reg && !s_axil_bready;

    if (s_axil_awvalid && s_axil_wvalid && (!s_axil_bvalid || s_axil_bready) && (!s_axil_awready && !s_axil_wready)) begin
        s_axil_awready_next = 1'b1;
        s_axil_wready_next = 1'b1;
        s_axil_bvalid_next = 1'b1;

        mem_wr_en = 1'b1;
    end
end

// =========================================================================
// Byte-lane 0: bits [7:0]
// =========================================================================
always @(posedge clk) begin
    if (mem_wr_en && s_axil_wstrb[0])
        mem0[s_axil_awaddr_valid] <= s_axil_wdata[7:0];
    s_axil_rdata_reg[7:0] <= mem0[s_axil_araddr_valid];
end

// =========================================================================
// Byte-lane 1: bits [15:8]
// =========================================================================
always @(posedge clk) begin
    if (mem_wr_en && s_axil_wstrb[1])
        mem1[s_axil_awaddr_valid] <= s_axil_wdata[15:8];
    s_axil_rdata_reg[15:8] <= mem1[s_axil_araddr_valid];
end

// =========================================================================
// Byte-lane 2: bits [23:16]
// =========================================================================
always @(posedge clk) begin
    if (mem_wr_en && s_axil_wstrb[2])
        mem2[s_axil_awaddr_valid] <= s_axil_wdata[23:16];
    s_axil_rdata_reg[23:16] <= mem2[s_axil_araddr_valid];
end

// =========================================================================
// Byte-lane 3: bits [31:24]
// =========================================================================
always @(posedge clk) begin
    if (mem_wr_en && s_axil_wstrb[3])
        mem3[s_axil_awaddr_valid] <= s_axil_wdata[31:24];
    s_axil_rdata_reg[31:24] <= mem3[s_axil_araddr_valid];
end

// =========================================================================
// AXI write-channel control registers
// =========================================================================
always @(posedge clk) begin
    s_axil_awready_reg <= s_axil_awready_next;
    s_axil_wready_reg <= s_axil_wready_next;
    s_axil_bvalid_reg <= s_axil_bvalid_next;

    if (~rstn) begin
        s_axil_awready_reg <= 1'b0;
        s_axil_wready_reg <= 1'b0;
        s_axil_bvalid_reg <= 1'b0;
    end
end

// =========================================================================
// Read channel control
// =========================================================================

always @* begin
    mem_rd_en = 1'b0;

    s_axil_arready_next = 1'b0;
    s_axil_rvalid_next = s_axil_rvalid_reg && !(s_axil_rready || (PIPELINE_OUTPUT && !s_axil_rvalid_pipe_reg));

    if (s_axil_arvalid && (!s_axil_rvalid || s_axil_rready || (PIPELINE_OUTPUT && !s_axil_rvalid_pipe_reg)) && (!s_axil_arready)) begin
        s_axil_arready_next = 1'b1;
        s_axil_rvalid_next = 1'b1;

        mem_rd_en = 1'b1;
    end
end

always @(posedge clk) begin
    s_axil_arready_reg <= s_axil_arready_next;
    s_axil_rvalid_reg <= s_axil_rvalid_next;

    if (!s_axil_rvalid_pipe_reg || s_axil_rready) begin
        s_axil_rdata_pipe_reg <= s_axil_rdata_reg;
        s_axil_rvalid_pipe_reg <= s_axil_rvalid_reg;
    end

    if (~rstn) begin
        s_axil_arready_reg <= 1'b0;
        s_axil_rvalid_reg <= 1'b0;
        s_axil_rvalid_pipe_reg <= 1'b0;
    end
end

endmodule
