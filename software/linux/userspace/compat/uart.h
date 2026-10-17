#ifndef ZCORE_LINUX_UART_H
#define ZCORE_LINUX_UART_H
#include <stdio.h>
static inline void uart_set_baud(unsigned n) { (void)n; setbuf(stdout,NULL); }
static inline void uart_puts(const char *s) { fputs(s,stdout); }
static inline void uart_putchar(char c) { putchar(c); }
#endif
