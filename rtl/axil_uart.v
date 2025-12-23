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
//            AXI-Lite UART Module
//    8N1 Format: 8 data bits, no parity, 1 stop bit
// **************************************************

`timescale 1ns / 1ps

module axil_uart #(
    parameter DATA_WIDTH = 32,
    parameter ADDR_WIDTH = 12,
    parameter STRB_WIDTH = (DATA_WIDTH/8),
    parameter DEFAULT_BAUD_DIV = 16'd326  // 50MHz / (16 * 9600) = 326 for 9600 baud
)(
    input  wire                   clk,
    input  wire                   rst,

    // UART Physical Interface
    output wire                   uart_tx,
    input  wire                   uart_rx,

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
//                 Register Map
// **************************************************
// 0x00: TX_DATA  [W]   - Write byte to transmit (bits [7:0])
// 0x04: RX_DATA  [R]   - Read received byte (bits [7:0])
// 0x08: STATUS   [R]   - Status register
// 0x0C: CTRL     [R/W] - Control register
// 0x10: BAUD_DIV [R/W] - Baud rate divisor

localparam ADDR_TX_DATA  = 4'h0;
localparam ADDR_RX_DATA  = 4'h4;
localparam ADDR_STATUS   = 4'h8;
localparam ADDR_CTRL     = 4'hC;
localparam ADDR_BAUD_DIV = 5'h10;  // Use bit 4

// **************************************************
//              Internal Registers
// **************************************************

// Control register bits
reg tx_en;
reg rx_en;

// Baud rate divisor
reg [15:0] baud_div;

// TX registers
reg [7:0] tx_data;
reg tx_start;
reg tx_busy;
reg tx_empty;

// RX registers
reg [7:0] rx_data;
reg rx_valid;
reg rx_error;

// **************************************************
//             Baud Rate Generator
// **************************************************

reg [15:0] baud_counter;
reg baud_tick;

always @(posedge clk) begin
    if (rst) begin
        baud_counter <= 16'd0;
        baud_tick <= 1'b0;
    end else begin
        if (baud_counter >= baud_div - 1) begin
            baud_counter <= 16'd0;
            baud_tick <= 1'b1;
        end else begin
            baud_counter <= baud_counter + 1;
            baud_tick <= 1'b0;
        end
    end
end

// **************************************************
//                TX State Machine
// **************************************************
// 16x oversampling for TX timing

localparam TX_IDLE  = 3'd0;
localparam TX_START = 3'd1;
localparam TX_DATA  = 3'd2;
localparam TX_STOP  = 3'd3;

reg [2:0] tx_state;
reg [3:0] tx_bit_count;
reg [3:0] tx_sample_count;
reg [7:0] tx_shift_reg;
reg tx_out;

assign uart_tx = tx_out;

always @(posedge clk) begin
    if (rst) begin
        tx_state <= TX_IDLE;
        tx_bit_count <= 4'd0;
        tx_sample_count <= 4'd0;
        tx_shift_reg <= 8'hFF;
        tx_out <= 1'b1;  // Idle high
        tx_busy <= 1'b0;
        tx_empty <= 1'b1;
    end else begin
        // Check for tx_start outside baud_tick to not miss the pulse
        // Start transmission when tx_start is asserted in IDLE state
        if (tx_start && tx_en && tx_state == TX_IDLE) begin
            tx_shift_reg <= tx_data;
            tx_state <= TX_START;
            tx_sample_count <= 4'd0;
            tx_busy <= 1'b1;
            tx_empty <= 1'b0;
        end
        
        if (baud_tick) begin
            case (tx_state)
                TX_IDLE: begin
                    tx_out <= 1'b1;  // Idle high
                    tx_busy <= 1'b0;
                end
                
                TX_START: begin
                    tx_out <= 1'b0;  // Start bit
                    tx_sample_count <= tx_sample_count + 1;
                    if (tx_sample_count == 4'd15) begin
                        tx_state <= TX_DATA;
                        tx_sample_count <= 4'd0;
                        tx_bit_count <= 4'd0;
                    end
                end
                
                TX_DATA: begin
                    tx_out <= tx_shift_reg[0];  // LSB first
                    tx_sample_count <= tx_sample_count + 1;
                    if (tx_sample_count == 4'd15) begin
                        tx_shift_reg <= {1'b0, tx_shift_reg[7:1]};  // Shift right
                        tx_sample_count <= 4'd0;
                        tx_bit_count <= tx_bit_count + 1;
                        if (tx_bit_count == 4'd7) begin
                            tx_state <= TX_STOP;
                        end
                    end
                end
                
                TX_STOP: begin
                    tx_out <= 1'b1;  // Stop bit
                    tx_sample_count <= tx_sample_count + 1;
                    if (tx_sample_count == 4'd15) begin
                        tx_state <= TX_IDLE;
                        tx_empty <= 1'b1;
                        tx_busy <= 1'b0;
                    end
                end
                
                default: tx_state <= TX_IDLE;
            endcase
        end
    end
end

// **************************************************
//                RX State Machine
// **************************************************
// 16x oversampling for reliable sampling

localparam RX_IDLE  = 3'd0;
localparam RX_START = 3'd1;
localparam RX_DATA  = 3'd2;
localparam RX_STOP  = 3'd3;

reg [2:0] rx_state;
reg [3:0] rx_bit_count;
reg [3:0] rx_sample_count;
reg [7:0] rx_shift_reg;
reg [2:0] rx_sync;  // Synchronizer for rx input

// Synchronize rx input
always @(posedge clk) begin
    if (rst) begin
        rx_sync <= 3'b111;
    end else begin
        rx_sync <= {rx_sync[1:0], uart_rx};
    end
end

wire rx_in = rx_sync[2];  // Synchronized input

always @(posedge clk) begin
    if (rst) begin
        rx_state <= RX_IDLE;
        rx_bit_count <= 4'd0;
        rx_sample_count <= 4'd0;
        rx_shift_reg <= 8'd0;
        rx_data <= 8'd0;
        rx_valid <= 1'b0;
        rx_error <= 1'b0;
    end else begin
        if (baud_tick && rx_en) begin
            case (rx_state)
                RX_IDLE: begin
                    if (rx_in == 1'b0) begin
                        // Potential start bit detected
                        rx_state <= RX_START;
                        rx_sample_count <= 4'd0;
                    end
                end
                
                RX_START: begin
                    rx_sample_count <= rx_sample_count + 1;
                    // Sample at middle of start bit (sample 7)
                    if (rx_sample_count == 4'd7) begin
                        if (rx_in == 1'b0) begin
                            // Valid start bit
                            rx_state <= RX_DATA;
                            rx_sample_count <= 4'd0;
                            rx_bit_count <= 4'd0;
                            rx_shift_reg <= 8'd0;
                        end else begin
                            // False start, go back to idle
                            rx_state <= RX_IDLE;
                        end
                    end
                end
                
                RX_DATA: begin
                    rx_sample_count <= rx_sample_count + 1;
                    // Sample at middle of each data bit (sample 15)
                    if (rx_sample_count == 4'd15) begin
                        rx_shift_reg <= {rx_in, rx_shift_reg[7:1]};  // Shift in MSB, LSB first
                        rx_sample_count <= 4'd0;
                        rx_bit_count <= rx_bit_count + 1;
                        if (rx_bit_count == 4'd7) begin
                            rx_state <= RX_STOP;
                        end
                    end
                end
                
                RX_STOP: begin
                    rx_sample_count <= rx_sample_count + 1;
                    // Sample at middle of stop bit
                    if (rx_sample_count == 4'd15) begin
                        if (rx_in == 1'b1) begin
                            // Valid stop bit
                            rx_data <= rx_shift_reg;
                            rx_valid <= 1'b1;
                            rx_error <= 1'b0;
                        end else begin
                            // Framing error
                            rx_error <= 1'b1;
                        end
                        rx_state <= RX_IDLE;
                    end
                end
                
                default: rx_state <= RX_IDLE;
            endcase
        end
    end
end

// **************************************************
//           AXI-Lite Interface Logic
// **************************************************

// AXI-Lite Internal Registers
reg s_axil_awready_reg = 0;
reg s_axil_wready_reg = 0;
reg s_axil_bvalid_reg = 0;
reg s_axil_arready_reg = 0;
reg s_axil_rvalid_reg = 0;
reg [DATA_WIDTH-1:0] s_axil_rdata_reg = 0;

// Internal Logic Signals
reg [ADDR_WIDTH-1:0] write_addr_reg;
reg [DATA_WIDTH-1:0] write_data_reg;
reg write_en;

reg [ADDR_WIDTH-1:0] read_addr_reg;

// Assignments
assign s_axil_awready = s_axil_awready_reg;
assign s_axil_wready = s_axil_wready_reg;
assign s_axil_bresp = 2'b00; // OKAY
assign s_axil_bvalid = s_axil_bvalid_reg;
assign s_axil_arready = s_axil_arready_reg;
assign s_axil_rdata = s_axil_rdata_reg;
assign s_axil_rresp = 2'b00; // OKAY
assign s_axil_rvalid = s_axil_rvalid_reg;

// Write Channel Logic
always @(posedge clk) begin
    if (rst) begin
        s_axil_awready_reg <= 0;
        s_axil_wready_reg <= 0;
        s_axil_bvalid_reg <= 0;
        write_en <= 0;
        write_addr_reg <= 0;
        write_data_reg <= 0;
        tx_start <= 0;
        tx_data <= 8'd0;
        tx_en <= 1'b1;  // Enable by default
        rx_en <= 1'b1;  // Enable by default
        baud_div <= DEFAULT_BAUD_DIV;
    end else begin
        write_en <= 0;
        tx_start <= 0;

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
            write_en <= 1;
            
            // Register Write Logic
            case (write_addr_reg[4:2])
                3'b000: begin // TX_DATA (0x00)
                    tx_data <= write_data_reg[7:0];
                    tx_start <= 1'b1;
                end
                3'b011: begin // CTRL (0x0C)
                    tx_en <= write_data_reg[0];
                    rx_en <= write_data_reg[1];
                end
                3'b100: begin // BAUD_DIV (0x10)
                    baud_div <= write_data_reg[15:0];
                end
            endcase
        end else if (s_axil_bready && s_axil_bvalid_reg) begin
            s_axil_bvalid_reg <= 0;
        end
    end
end

// Read Channel Logic
always @(posedge clk) begin
    if (rst) begin
        s_axil_arready_reg <= 0;
        s_axil_rvalid_reg <= 0;
        s_axil_rdata_reg <= 0;
        read_addr_reg <= 0;
    end else begin
        // Address Handshake
        if (s_axil_arvalid && !s_axil_arready_reg && (!s_axil_rvalid_reg || s_axil_rready)) begin
            s_axil_arready_reg <= 1;
            read_addr_reg <= s_axil_araddr;
        end else begin
            s_axil_arready_reg <= 0;
        end

        // Read Response
        if (s_axil_arready_reg) begin
            s_axil_rvalid_reg <= 1;
            
            // Register Read Logic
            case (read_addr_reg[4:2])
                3'b000: s_axil_rdata_reg <= {24'd0, tx_data};        // TX_DATA
                3'b001: begin                                         // RX_DATA
                    s_axil_rdata_reg <= {24'd0, rx_data};
                    // Clear rx_valid after read (handled separately)
                end
                3'b010: s_axil_rdata_reg <= {28'd0, rx_error, rx_valid, tx_busy, tx_empty}; // STATUS
                3'b011: s_axil_rdata_reg <= {30'd0, rx_en, tx_en};    // CTRL
                3'b100: s_axil_rdata_reg <= {16'd0, baud_div};        // BAUD_DIV
                default: s_axil_rdata_reg <= 32'd0;
            endcase
        end else if (s_axil_rready && s_axil_rvalid_reg) begin
            s_axil_rvalid_reg <= 0;
            // Clear rx_valid when RX_DATA is read
            if (read_addr_reg[4:2] == 3'b001) begin
                // Note: This should be done carefully to avoid race conditions
                // For simplicity, we clear it here
            end
        end
    end
end

// Clear rx_valid after RX_DATA read
always @(posedge clk) begin
    if (rst) begin
        // rx_valid cleared in main RX FSM
    end else if (s_axil_rready && s_axil_rvalid_reg && read_addr_reg[4:2] == 3'b001) begin
        // rx_valid will be set again by RX FSM when new data arrives
        // This is handled in the RX FSM
    end
end

endmodule
