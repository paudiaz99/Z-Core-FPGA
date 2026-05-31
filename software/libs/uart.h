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

#ifndef UART_H
#define UART_H

#define UART_BASE     0x04000000
#define UART_TX       (*((volatile unsigned int *)(UART_BASE + 0x00)))
#define UART_RX       (*((volatile unsigned int *)(UART_BASE + 0x04)))
#define UART_STAT     (*((volatile unsigned int *)(UART_BASE + 0x08)))
#define UART_CTRL     (*((volatile unsigned int *)(UART_BASE + 0x0C)))
#define UART_BAUD_DIV (*((volatile unsigned int *)(UART_BASE + 0x10)))

// STATUS register bits
#define UART_STAT_TX_EMPTY  0x01
#define UART_STAT_TX_BUSY   0x02
#define UART_STAT_RX_VALID  0x04
#define UART_STAT_RX_ERROR  0x08

void uart_putc(char c);
void uart_puts(const char *s);
char uart_getc(void);
char uart_getc_blocking(void);
void uart_puthex(unsigned int val);
void uart_putint(int val);
void uart_set_baud(unsigned int divisor);

#endif // UART_H
