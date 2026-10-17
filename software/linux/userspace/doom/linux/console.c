/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
void console_init(void) { setbuf(stdout,NULL); }
void console_putchar(char c) { putchar(c); }
char console_getchar(void) { unsigned char c; return read(0,&c,1)==1?c:0; }
int console_getchar_nowait(void) { unsigned char c; return read(0,&c,1)==1?c:-1; }
void console_puts(const char *s) { fputs(s,stdout); }
int console_printf(const char *fmt,...) {
    va_list ap; va_start(ap,fmt); int n=vprintf(fmt,ap); va_end(ap); return n;
}
