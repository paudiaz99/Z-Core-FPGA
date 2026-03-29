/*
 * VGA Test — Z-Core RISC-V
 *
 * Draws SMPTE-style color bars, then animates a bouncing square
 * on the 160x120 framebuffer (4x upscaled to 640x480 VGA).
 */

#include "libs/uart.h"
#include "libs/vga.h"

#define BALL_SIZE 6

static void draw_gradient(int y0, int h) {
    for (int y = y0; y < y0 + h && y < VGA_HEIGHT; y++) {
        VGA_FB_ADDR = (unsigned int)(y * VGA_WIDTH);
        for (int x = 0; x < VGA_WIDTH; x++) {
            unsigned char r = (x * 7) / VGA_WIDTH;
            unsigned char g = (y - y0) * 7 / h;
            unsigned char b = 3 - ((x * 3) / VGA_WIDTH);
            VGA_FB_DATA = VGA_RGB(r, g, b);
        }
    }
}

int main(void) {
    uart_puts("VGA Test\r\n");

    /* Phase 1: color bars (top 60 rows) */
    vga_fill(VGA_BLACK);

    uart_puts("Color bars...\r\n");
    for (int y = 0; y < 60; y++) {
        static const unsigned char bars[] = {
            VGA_WHITE, VGA_YELLOW, VGA_CYAN, VGA_GREEN,
            VGA_MAGENTA, VGA_RED, VGA_BLUE, VGA_BLACK
        };
        int bar_w = VGA_WIDTH / 8;
        VGA_FB_ADDR = (unsigned int)(y * VGA_WIDTH);
        for (int x = 0; x < VGA_WIDTH; x++) {
            int idx = x / bar_w;
            if (idx > 7) idx = 7;
            VGA_FB_DATA = bars[idx];
        }
    }

    /* Phase 2: gradient (bottom 60 rows) */
    uart_puts("Gradient...\r\n");
    draw_gradient(60, 60);

    uart_puts("Bouncing ball...\r\n");

    /* Phase 3: bouncing square */
    int bx = 40, by = 40;
    int dx = 1, dy = 1;
    int frame = 0;

    while (1) {
        vga_wait_vsync();

        /* Erase old ball */
        vga_fill_rect(bx, by, BALL_SIZE, BALL_SIZE, VGA_BLACK);

        /* Update position */
        bx += dx;
        by += dy;

        if (bx <= 0 || bx >= VGA_WIDTH - BALL_SIZE) {
            dx = -dx;
            bx += dx;
        }
        if (by <= 0 || by >= VGA_HEIGHT - BALL_SIZE) {
            dy = -dy;
            by += dy;
        }

        /* Draw ball with a simple color cycle */
        unsigned char color = VGA_RGB(
            (frame >> 3) & 7,
            (frame >> 5) & 7,
            (frame >> 7) & 3
        );
        vga_fill_rect(bx, by, BALL_SIZE, BALL_SIZE, color);

        frame++;
    }

    return 0;
}
