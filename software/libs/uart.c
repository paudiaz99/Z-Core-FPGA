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

#include "uart.h"

void uart_putc(char c) {
  UART_TX = (unsigned int)c;
  // Wait for transmission to complete (tx_empty = bit 0)
  while (!(UART_STAT & 0x01))
    ; // Wait until tx_empty is 1
}

void uart_puts(const char *s) {
  while (*s) {
    uart_putc(*s++);
  }
}

char uart_getc(void) { return (char)(UART_RX & 0xFF); }

void uart_puthex(unsigned int val) {
  const char hex[] = "0123456789ABCDEF";
  uart_puts("0x");
  for (int i = 28; i >= 0; i -= 4) {
    uart_putc(hex[(val >> i) & 0xF]);
  }
}

void uart_putint(int val) {
  char buf[12];
  int i = 0;
  int neg = 0;

  if (val < 0) {
    neg = 1;
    val = -val;
  }

  if (val == 0) {
    uart_putc('0');
    return;
  }

  while (val > 0) {
    buf[i++] = '0' + (val % 10);
    val = val / 10;
  }

  if (neg)
    uart_putc('-');
  while (i > 0) {
    uart_putc(buf[--i]);
  }
}
