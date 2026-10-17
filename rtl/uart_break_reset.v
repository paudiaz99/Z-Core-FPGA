// **************************************************
//   UART break detector -> CPU reset
// **************************************************
// SPDX-License-Identifier: MIT
// Copyright (c) 2025–2026 Pau Díaz Cuesta
//
// Resets the CPU when the host holds the UART line low long enough to be a
// BREAK condition. This needs no extra pin: it watches the uart_rx pin the
// board already has, so a host can reset the core over the same USB-serial
// cable that carries the data.
//
// The host side is termios tcsendbreak(), which drives TX low for ~250 ms.
//
// Why a break is unambiguous
// --------------------------
// A UART frame cannot hold the line low indefinitely: the stop bit forces it
// high every frame. The worst case is a 0x00 data byte, which is low for the
// start bit plus 8 data bits = 9 bit times, and is then followed by a high
// stop bit. So the longest legal low period is 9 bit times:
//
//     460800 baud ->  19.5 us
//     115200 baud ->  78.1 us
//       9600 baud -> 937.5 us
//
// BREAK_US defaults to 5000 (5 ms), which is 5x the slowest legal low period
// and 50x shorter than the break the host sends. Binary payloads full of zero
// bytes are safe: every frame still ends with a stop bit.
//
// This lives outside the CPU deliberately -- it still works when the program
// has hung or taken an unhandled trap, which is exactly when a reset is
// needed and when a software "reboot on magic byte" would not respond.
//
// Note: uart_rx must have a weak pull-up (see Z-Core.qsf), or an unplugged
// adapter leaves the pin floating and may hold the core in reset.

module uart_break_reset #(
    parameter CLK_HZ   = 50_000_000,
    parameter BREAK_US = 5000
)(
    input  wire clk,
    input  wire uart_rx,
    output wire break_rst      // active high, held for as long as the break is
);

    localparam integer BREAK_CYCLES = (CLK_HZ / 1_000_000) * BREAK_US;
    localparam integer CNT_W        = $clog2(BREAK_CYCLES + 1);

    // uart_rx is asynchronous to clk.
    reg rx_meta_r, rx_sync_r;
    always @(posedge clk) begin
        rx_meta_r <= uart_rx;
        rx_sync_r <= rx_meta_r;
    end

    reg [CNT_W-1:0] low_cnt_r;
    reg             break_r;

    // Free-running: not gated by any reset source, so a break still fires
    // while the CPU is held in reset by something else.
    always @(posedge clk) begin
        if (rx_sync_r) begin           // line idle high: rearm
            low_cnt_r <= {CNT_W{1'b0}};
            break_r   <= 1'b0;
        end else if (low_cnt_r == BREAK_CYCLES[CNT_W-1:0]) begin
            break_r   <= 1'b1;         // hold until the line returns high
        end else begin
            low_cnt_r <= low_cnt_r + 1'b1;
        end
    end

    assign break_rst = break_r;

endmodule
