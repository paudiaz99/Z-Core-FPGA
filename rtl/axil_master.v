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
//            AXI-Lite Master Interface
// Converts simple memory interface to AXI-Lite protocol
// **************************************************

`timescale 1ns / 1ns

module axil_master #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 32,
    parameter STRB_WIDTH = (DATA_WIDTH/8)
)(
    input  wire                   clk,
    input  wire                   rstn,

    // Simple Memory Interface (from Control Unit)
    // mem_req: pulsed high to start transaction
    // mem_wen: 1 for write, 0 for read
    input  wire                   mem_req,
    input  wire                   mem_wen,
    input  wire [ADDR_WIDTH-1:0]  mem_addr,
    input  wire [DATA_WIDTH-1:0]  mem_wdata,
    input  wire [STRB_WIDTH-1:0]  mem_wstrb,
    output reg  [DATA_WIDTH-1:0]  mem_rdata,
    output reg                    mem_ready,
    output wire                   mem_busy,

    // AXI-Lite Master Interface
    output reg  [ADDR_WIDTH-1:0]  m_axil_awaddr,
    output wire [2:0]             m_axil_awprot,
    output reg                    m_axil_awvalid,
    input  wire                   m_axil_awready,
    output reg  [DATA_WIDTH-1:0]  m_axil_wdata,
    output reg  [STRB_WIDTH-1:0]  m_axil_wstrb,
    output reg                    m_axil_wvalid,
    input  wire                   m_axil_wready,
    input  wire [1:0]             m_axil_bresp,
    input  wire                   m_axil_bvalid,
    output reg                    m_axil_bready,
    output reg  [ADDR_WIDTH-1:0]  m_axil_araddr,
    output wire [2:0]             m_axil_arprot,
    output reg                    m_axil_arvalid,
    input  wire                   m_axil_arready,
    input  wire [DATA_WIDTH-1:0]  m_axil_rdata,
    input  wire [1:0]             m_axil_rresp,
    input  wire                   m_axil_rvalid,
    output reg                    m_axil_rready
);

// Protection signals (unprivileged, secure, data access)
assign m_axil_awprot = 3'b000;
assign m_axil_arprot = 3'b000;

// FSM States
localparam STATE_IDLE       = 3'd0;
localparam STATE_READ_ADDR  = 3'd1;
localparam STATE_READ_DATA  = 3'd2;
localparam STATE_READ_DONE  = 3'd5;  // NEW: Data captured, signal ready
localparam STATE_WRITE_ADDR = 3'd3;
localparam STATE_WRITE_RESP = 3'd4;

reg [2:0] state;

assign mem_busy = (state != STATE_IDLE);

// Capture request parameters
reg [ADDR_WIDTH-1:0] addr_reg;
reg [DATA_WIDTH-1:0] wdata_reg;
reg [STRB_WIDTH-1:0] wstrb_reg;
reg wen_reg;

// Main state machine
always @(posedge clk) begin
    if (~rstn) begin
        state          <= STATE_IDLE;
        m_axil_araddr  <= {ADDR_WIDTH{1'b0}};
        m_axil_arvalid <= 1'b0;
        m_axil_rready  <= 1'b0;
        m_axil_awaddr  <= {ADDR_WIDTH{1'b0}};
        m_axil_awvalid <= 1'b0;
        m_axil_wdata   <= {DATA_WIDTH{1'b0}};
        m_axil_wstrb   <= {STRB_WIDTH{1'b0}};
        m_axil_wvalid  <= 1'b0;
        m_axil_bready  <= 1'b0;
        mem_rdata      <= {DATA_WIDTH{1'b0}};
        mem_ready      <= 1'b0;
        addr_reg       <= {ADDR_WIDTH{1'b0}};
        wdata_reg      <= {DATA_WIDTH{1'b0}};
        wstrb_reg      <= {STRB_WIDTH{1'b0}};
        wen_reg        <= 1'b0;
    end else begin
        // Default: mem_ready is a single-cycle pulse
        mem_ready <= 1'b0;
        
        case (state)
            STATE_IDLE: begin
                if (mem_req) begin
                    // Capture request parameters
                    addr_reg  <= mem_addr;
                    wdata_reg <= mem_wdata;
                    wstrb_reg <= mem_wstrb;
                    wen_reg   <= mem_wen;
                    
                    if (mem_wen) begin
                        // Start write transaction
                        m_axil_awaddr  <= mem_addr;
                        m_axil_awvalid <= 1'b1;
                        m_axil_wdata   <= mem_wdata;
                        m_axil_wstrb   <= mem_wstrb;
                        m_axil_wvalid  <= 1'b1;
                        state <= STATE_WRITE_ADDR;
                    end else begin
                        // Start read transaction
                        m_axil_araddr  <= mem_addr;
                        m_axil_arvalid <= 1'b1;
                        state <= STATE_READ_ADDR;
                    end
                end
            end
            
            STATE_READ_ADDR: begin
                // Wait for address to be accepted
                if (m_axil_arready) begin
                    m_axil_arvalid <= 1'b0;
                    m_axil_rready  <= 1'b1;
                    state <= STATE_READ_DATA;
                end
            end
            
            STATE_READ_DATA: begin
                // Wait for read data
                if (m_axil_rvalid) begin
                    // Capture data NOW - it will be stable next cycle
                    mem_rdata     <= m_axil_rdata;
                    m_axil_rready <= 1'b0;
                    // Go to DONE state to assert mem_ready AFTER data is registered
                    state <= STATE_READ_DONE;
                end
            end
            
            STATE_READ_DONE: begin
                // Data is now stable in mem_rdata, signal completion
                mem_ready <= 1'b1;
                state <= STATE_IDLE;
            end
            
            STATE_WRITE_ADDR: begin
                // Wait for both address and data to be accepted
                if (m_axil_awready) begin
                    m_axil_awvalid <= 1'b0;
                end
                if (m_axil_wready) begin
                    m_axil_wvalid <= 1'b0;
                end
                // Move to response wait when both channels are done
                if ((m_axil_awready || !m_axil_awvalid) && 
                    (m_axil_wready || !m_axil_wvalid)) begin
                    m_axil_bready <= 1'b1;
                    state <= STATE_WRITE_RESP;
                end
            end
            
            STATE_WRITE_RESP: begin
                // Wait for write response
                if (m_axil_bvalid) begin
                    mem_ready     <= 1'b1;
                    m_axil_bready <= 1'b0;
                    state <= STATE_IDLE;
                end
            end
            
            default: state <= STATE_IDLE;
        endcase
    end
end

endmodule
