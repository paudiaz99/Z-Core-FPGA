// ================================================================
// Simple Pong - Z-Core RV32IM Demo (4KB RAM optimized)
// Uses UART output and MUL/DIV instructions
// ================================================================

#include "libs/uart.h"

#define GPIO_LOW (*((volatile unsigned int *)0x04001000))
#define GPIO_DIR_LOW (*((volatile unsigned int *)0x04001008))

// Game constants
#define W 32 // Screen width
#define H 12 // Screen height
#define PH 3 // Paddle height

void configure_gpio(void) {
  GPIO_DIR_LOW = 0xFF; // Set first 8 bits as output (1=OUT, 0=IN)
}

void gotoxy(int x, int y) {
  uart_puts("\033[");
  uart_putint(y + 1);
  uart_putc(';');
  uart_putint(x + 1);
  uart_putc('H');
}

void delay(int n) {
  for (int i = 0; i < n * 100; i++)
    asm volatile("nop");
}

static unsigned int seed = 12345;
int rnd(void) {
  seed = seed * 1103515245 + 12345; // MUL instruction
  return (int)((seed >> 16) & 0x7);
}

void draw_border(void) {
  int i;

  // Top border
  gotoxy(0, 0);
  uart_putc('+');
  for (i = 0; i < W - 2; i++)
    uart_putc('-');
  uart_putc('+');

  // Side borders
  for (i = 1; i < H - 1; i++) {
    gotoxy(0, i);
    uart_putc('|');
    gotoxy(W - 1, i);
    uart_putc('|');
  }

  // Bottom border
  gotoxy(0, H - 1);
  uart_putc('+');
  for (i = 0; i < W - 2; i++)
    uart_putc('-');
  uart_putc('+');

  // Center line
  for (i = 1; i < H - 1; i++) {
    gotoxy(W / 2, i);
    uart_putc(':');
  }
}

int main(void) {
  int bx = W / 2, by = H / 2;         // Ball position
  int old_bx, old_by;                 // Previous ball position
  int dx = 1, dy = 1;                 // Ball velocity
  int p1 = H / 2 - 1, p2 = H / 2 - 1; // Paddle positions
  int old_p1, old_p2;                 // Previous paddle positions
  int s1 = 0, s2 = 0;                 // Scores
  int frame = 0;
  int i;

  configure_gpio();
  GPIO_LOW = 0x00;
  

  uart_puts("\033[2J"); // Clear screen
  draw_border();        // Draw border once

  // Initialize old positions
  old_bx = bx;
  old_by = by;
  old_p1 = p1;
  old_p2 = p2;

  while (1) {
    // Clear old ball position (before drawing new one)
    if (old_bx != bx || old_by != by) {
      gotoxy(old_bx, old_by);
      // Restore what was there (center line or space)
      if (old_bx == W / 2) {
        uart_putc(':');
      } else {
        uart_putc(' ');
      }
    }
    
    // Clear old paddle positions if they moved
    if (old_p1 != p1) {
      for (i = 0; i < PH; i++) {
        int y = old_p1 + i;
        if (y > 0 && y < H - 1) {
          gotoxy(2, y);
          uart_putc(' ');
        }
      }
    }
    if (old_p2 != p2) {
      for (i = 0; i < PH; i++) {
        int y = old_p2 + i;
        if (y > 0 && y < H - 1) {
          gotoxy(W - 3, y);
          uart_putc(' ');
        }
      }
    }

    // Save current positions as old
    old_bx = bx;
    old_by = by;
    old_p1 = p1;
    old_p2 = p2;

    // Update ball position
    bx += dx;
    by += dy;

    // Bounce top/bottom
    if (by <= 1 || by >= H - 2) {
      dy = -dy;
      by += dy;
    }

    // Paddle collision (left)
    if (bx == 2 && by >= p1 && by < p1 + PH) {
      dx = 1;
      bx = 3;
    }

    // Paddle collision (right)
    if (bx == W - 3 && by >= p2 && by < p2 + PH) {
      dx = -1;
      bx = W - 4;
    }

    // Score detection
    if (bx <= 1) {
      s2++;
      bx = W / 2;
      by = H / 2;
      dx = 1;
      dy = (rnd() % 3) - 1;
      if (dy == 0)
        dy = 1;
      GPIO_LOW = 0xF0;
    }
    if (bx >= W - 2) {
      s1++;
      bx = W / 2;
      by = H / 2;
      dx = -1;
      dy = (rnd() % 3) - 1;
      if (dy == 0)
        dy = -1;
      GPIO_LOW = 0x0F;
    }

    // Simple AI for Paddle 2- uses multiplication for timing
    if ((frame * 5) % 11 == 0) {
      if (by < p2 + 1 && p2 > 1)
        p2--;
      if (by > p2 + 1 && p2 < H - PH - 1)
        p2++;
    }
    // Player 1 control via GPIO inputs (bits 8-9)
    // Read once per frame - no blocking
    unsigned int buttons = (GPIO_LOW >> 8) & 0x03;

    // Bit 8: move paddle up
    if ((buttons & 0x01) && p1 > 1) {
      p1--;
    }

    // Bit 9: move paddle down
    if ((buttons & 0x02) && p1 < H - PH - 1) {
      p1++;
    }

    // Draw paddles
    for (i = 0; i < PH; i++) {
      if (p1 + i > 0 && p1 + i < H - 1) {
        gotoxy(2, p1 + i);
        uart_putc('#');
      }
      if (p2 + i > 0 && p2 + i < H - 1) {
        gotoxy(W - 3, p2 + i);
        uart_putc('#');
      }
    }

    // Draw ball
    gotoxy(bx, by);
    uart_putc('O');

    // Score display
    gotoxy((W / 2) - 3, 0);
    uart_puts(" ");
    uart_putint(s1);
    uart_puts(" - ");
    uart_putint(s2);
    uart_puts(" ");

    // Stats using multiplication
    gotoxy(0, H);
    uart_puts("F:");
    uart_putint(frame);

    // GPIO shows points
    GPIO_LOW = (unsigned int)((s1 << 4) | (s2 & 0x0F));

    frame++;
    delay(100);

    // Win condition
    if (s1 >= 5 || s2 >= 5) {
      gotoxy(W / 2 - 5, H / 2);
      uart_puts(s1 >= 5 ? " P1 WINS! " : " P2 WINS! ");
      GPIO_LOW = 0xFF;
      delay(5000);
      s1 = s2 = 0;
      bx = W / 2;
      by = H / 2;
      old_bx = bx;
      old_by = by;
      uart_puts("\033[2J");
      draw_border();
    }
  }

  return 0;
}
