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

module axil_cdc_bridge #(
    parameter ADDR_WIDTH = 26,
    parameter DATA_WIDTH = 32,
    parameter STRB_WIDTH = DATA_WIDTH / 8
)(
    // ---- Slave side (CPU clock domain) ----
    input  wire                     clk_s,
    input  wire                     rst_s_n,

    input  wire [ADDR_WIDTH-1:0]    s_axil_awaddr,
    input  wire                     s_axil_awvalid,
    output reg                      s_axil_awready,
    input  wire [DATA_WIDTH-1:0]    s_axil_wdata,
    input  wire [STRB_WIDTH-1:0]    s_axil_wstrb,
    input  wire                     s_axil_wvalid,
    output reg                      s_axil_wready,
    output reg  [1:0]               s_axil_bresp,
    output reg                      s_axil_bvalid,
    input  wire                     s_axil_bready,

    input  wire [ADDR_WIDTH-1:0]    s_axil_araddr,
    input  wire                     s_axil_arvalid,
    output reg                      s_axil_arready,
    output reg  [DATA_WIDTH-1:0]    s_axil_rdata,
    output reg  [1:0]               s_axil_rresp,
    output reg                      s_axil_rvalid,
    input  wire                     s_axil_rready,

    // ---- Master side (SDRAM clock domain) ----
    input  wire                     clk_m,
    input  wire                     rst_m_n,

    output reg  [ADDR_WIDTH-1:0]    m_axil_awaddr,
    output reg                      m_axil_awvalid,
    input  wire                     m_axil_awready,
    output reg  [DATA_WIDTH-1:0]    m_axil_wdata,
    output reg  [STRB_WIDTH-1:0]    m_axil_wstrb,
    output reg                      m_axil_wvalid,
    input  wire                     m_axil_wready,
    input  wire [1:0]               m_axil_bresp,
    input  wire                     m_axil_bvalid,
    output reg                      m_axil_bready,

    output reg  [ADDR_WIDTH-1:0]    m_axil_araddr,
    output reg                      m_axil_arvalid,
    input  wire                     m_axil_arready,
    input  wire [DATA_WIDTH-1:0]    m_axil_rdata,
    input  wire [1:0]               m_axil_rresp,
    input  wire                     m_axil_rvalid,
    output reg                      m_axil_rready
);

    // ================================================================
    //  WRITE CHANNEL CDC
    // ================================================================

    // --- Request payload (latched in slave domain) ---
    reg [ADDR_WIDTH-1:0]    wr_req_addr;
    reg [DATA_WIDTH-1:0]    wr_req_data;
    reg [STRB_WIDTH-1:0]    wr_req_strb;

    // --- Response payload (latched in master domain) ---
    reg [1:0]               wr_resp_bresp;

    // --- Toggle handshake ---
    reg  wr_req_toggle_s;    // slave domain
    wire wr_req_toggle_m;    // synchronized into master domain
    reg  wr_ack_toggle_m;    // master domain
    wire wr_ack_toggle_s;    // synchronized into slave domain

    // 2-FF synchronizers
    reg [1:0] wr_req_sync_m;
    always @(posedge clk_m or negedge rst_m_n)
        if (!rst_m_n) wr_req_sync_m <= 2'b0;
        else          wr_req_sync_m <= {wr_req_sync_m[0], wr_req_toggle_s};
    assign wr_req_toggle_m = wr_req_sync_m[1];

    reg [1:0] wr_ack_sync_s;
    always @(posedge clk_s or negedge rst_s_n)
        if (!rst_s_n) wr_ack_sync_s <= 2'b0;
        else          wr_ack_sync_s <= {wr_ack_sync_s[0], wr_ack_toggle_m};
    assign wr_ack_toggle_s = wr_ack_sync_s[1];

    // --- Slave-side write FSM ---
    localparam WS_IDLE    = 2'd0;
    localparam WS_WAIT    = 2'd1;
    localparam WS_RESP    = 2'd2;

    reg [1:0] ws_state;

    always @(posedge clk_s or negedge rst_s_n) begin
        if (!rst_s_n) begin
            ws_state         <= WS_IDLE;
            s_axil_awready   <= 0;
            s_axil_wready    <= 0;
            s_axil_bvalid    <= 0;
            s_axil_bresp     <= 2'b00;
            wr_req_toggle_s  <= 0;
            wr_req_addr      <= 0;
            wr_req_data      <= 0;
            wr_req_strb      <= 0;
        end else begin
            s_axil_awready <= 0;
            s_axil_wready  <= 0;

            case (ws_state)
                WS_IDLE: begin
                    s_axil_bvalid <= 0;
                    if (s_axil_awvalid && s_axil_wvalid) begin
                        wr_req_addr     <= s_axil_awaddr;
                        wr_req_data     <= s_axil_wdata;
                        wr_req_strb     <= s_axil_wstrb;
                        s_axil_awready  <= 1;
                        s_axil_wready   <= 1;
                        wr_req_toggle_s <= ~wr_req_toggle_s;
                        ws_state        <= WS_WAIT;
                    end
                end

                WS_WAIT: begin
                    // Wait for ack toggle to match req toggle
                    if (wr_ack_toggle_s == wr_req_toggle_s) begin
                        s_axil_bvalid <= 1;
                        s_axil_bresp  <= wr_resp_bresp;
                        ws_state      <= WS_RESP;
                    end
                end

                WS_RESP: begin
                    if (s_axil_bready) begin
                        s_axil_bvalid <= 0;
                        ws_state      <= WS_IDLE;
                    end
                end
            endcase
        end
    end

    // --- Master-side write FSM ---
    localparam WM_IDLE     = 3'd0;
    localparam WM_ADDR     = 3'd1;
    localparam WM_DATA     = 3'd2;
    localparam WM_RESP     = 3'd3;
    localparam WM_ACK      = 3'd4;

    reg [2:0] wm_state;
    reg       wm_last_toggle;

    always @(posedge clk_m or negedge rst_m_n) begin
        if (!rst_m_n) begin
            wm_state        <= WM_IDLE;
            wm_last_toggle  <= 0;
            m_axil_awvalid  <= 0;
            m_axil_wvalid   <= 0;
            m_axil_bready   <= 0;
            m_axil_awaddr   <= 0;
            m_axil_wdata    <= 0;
            m_axil_wstrb    <= 0;
            wr_ack_toggle_m <= 0;
            wr_resp_bresp   <= 2'b00;
        end else begin
            case (wm_state)
                WM_IDLE: begin
                    m_axil_bready <= 0;
                    if (wr_req_toggle_m != wm_last_toggle) begin
                        // New write request arrived
                        m_axil_awaddr  <= wr_req_addr;
                        m_axil_wdata   <= wr_req_data;
                        m_axil_wstrb   <= wr_req_strb;
                        m_axil_awvalid <= 1;
                        m_axil_wvalid  <= 1;
                        wm_state       <= WM_ADDR;
                    end
                end

                WM_ADDR: begin
                    if (m_axil_awready) m_axil_awvalid <= 0;
                    if (m_axil_wready)  m_axil_wvalid  <= 0;
                    if ((!m_axil_awvalid || m_axil_awready) &&
                        (!m_axil_wvalid  || m_axil_wready)) begin
                        m_axil_awvalid <= 0;
                        m_axil_wvalid  <= 0;
                        m_axil_bready  <= 1;
                        wm_state       <= WM_RESP;
                    end
                end

                WM_RESP: begin
                    if (m_axil_bvalid) begin
                        wr_resp_bresp   <= m_axil_bresp;
                        m_axil_bready   <= 0;
                        wr_ack_toggle_m <= ~wr_ack_toggle_m;
                        wm_last_toggle  <= wr_req_toggle_m;
                        wm_state        <= WM_IDLE;
                    end
                end
            endcase
        end
    end

    // ================================================================
    //  READ CHANNEL CDC
    // ================================================================

    // --- Request payload ---
    reg [ADDR_WIDTH-1:0] rd_req_addr;

    // --- Response payload ---
    reg [DATA_WIDTH-1:0] rd_resp_data;
    reg [1:0]            rd_resp_rresp;

    // --- Toggle handshake ---
    reg  rd_req_toggle_s;
    wire rd_req_toggle_m;
    reg  rd_ack_toggle_m;
    wire rd_ack_toggle_s;

    reg [1:0] rd_req_sync_m;
    always @(posedge clk_m or negedge rst_m_n)
        if (!rst_m_n) rd_req_sync_m <= 2'b0;
        else          rd_req_sync_m <= {rd_req_sync_m[0], rd_req_toggle_s};
    assign rd_req_toggle_m = rd_req_sync_m[1];

    reg [1:0] rd_ack_sync_s;
    always @(posedge clk_s or negedge rst_s_n)
        if (!rst_s_n) rd_ack_sync_s <= 2'b0;
        else          rd_ack_sync_s <= {rd_ack_sync_s[0], rd_ack_toggle_m};
    assign rd_ack_toggle_s = rd_ack_sync_s[1];

    // --- Slave-side read FSM ---
    localparam RS_IDLE = 2'd0;
    localparam RS_WAIT = 2'd1;
    localparam RS_RESP = 2'd2;

    reg [1:0] rs_state;

    always @(posedge clk_s or negedge rst_s_n) begin
        if (!rst_s_n) begin
            rs_state         <= RS_IDLE;
            s_axil_arready   <= 0;
            s_axil_rvalid    <= 0;
            s_axil_rdata     <= 0;
            s_axil_rresp     <= 2'b00;
            rd_req_toggle_s  <= 0;
            rd_req_addr      <= 0;
        end else begin
            s_axil_arready <= 0;

            case (rs_state)
                RS_IDLE: begin
                    s_axil_rvalid <= 0;
                    if (s_axil_arvalid) begin
                        rd_req_addr     <= s_axil_araddr;
                        s_axil_arready  <= 1;
                        rd_req_toggle_s <= ~rd_req_toggle_s;
                        rs_state        <= RS_WAIT;
                    end
                end

                RS_WAIT: begin
                    if (rd_ack_toggle_s == rd_req_toggle_s) begin
                        s_axil_rdata  <= rd_resp_data;
                        s_axil_rresp  <= rd_resp_rresp;
                        s_axil_rvalid <= 1;
                        rs_state      <= RS_RESP;
                    end
                end

                RS_RESP: begin
                    if (s_axil_rready) begin
                        s_axil_rvalid <= 0;
                        rs_state      <= RS_IDLE;
                    end
                end
            endcase
        end
    end

    // --- Master-side read FSM ---
    localparam RM_IDLE  = 2'd0;
    localparam RM_ADDR  = 2'd1;
    localparam RM_DATA  = 2'd2;

    reg [1:0] rm_state;
    reg       rm_last_toggle;

    always @(posedge clk_m or negedge rst_m_n) begin
        if (!rst_m_n) begin
            rm_state        <= RM_IDLE;
            rm_last_toggle  <= 0;
            m_axil_arvalid  <= 0;
            m_axil_araddr   <= 0;
            m_axil_rready   <= 0;
            rd_ack_toggle_m <= 0;
            rd_resp_data    <= 0;
            rd_resp_rresp   <= 2'b00;
        end else begin
            case (rm_state)
                RM_IDLE: begin
                    m_axil_rready <= 0;
                    if (rd_req_toggle_m != rm_last_toggle) begin
                        m_axil_araddr  <= rd_req_addr;
                        m_axil_arvalid <= 1;
                        rm_state       <= RM_ADDR;
                    end
                end

                RM_ADDR: begin
                    if (m_axil_arready) begin
                        m_axil_arvalid <= 0;
                        m_axil_rready  <= 1;
                        rm_state       <= RM_DATA;
                    end
                end

                RM_DATA: begin
                    if (m_axil_rvalid) begin
                        rd_resp_data    <= m_axil_rdata;
                        rd_resp_rresp   <= m_axil_rresp;
                        m_axil_rready   <= 0;
                        rd_ack_toggle_m <= ~rd_ack_toggle_m;
                        rm_last_toggle  <= rd_req_toggle_m;
                        rm_state        <= RM_IDLE;
                    end
                end
            endcase
        end
    end

endmodule
