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

#include "printf.h"
#include "uart.h"
#include <stdint.h>

int uvprintf(const char *fmt, va_list ap)
{
    int count = 0;
    char numbuf[24]; /* enough for 64-bit in any base + sign */

    while (*fmt) {
        char ch = *fmt++;
        if (ch != '%') {
            uart_putc(ch);
            count++;
            continue;
        }

        /* ---- flags ---- */
        int left = 0, zero = 0, plus = 0, space = 0;
        for (;;) {
            char f = *fmt;
            if      (f == '-') left = 1;
            else if (f == '0') zero = 1;
            else if (f == '+') plus = 1;
            else if (f == ' ') space = 1;
            else break;
            fmt++;
        }

        /* ---- field width ---- */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9')
            width = width * 10 + (*fmt++ - '0');

        /* ---- length modifier ---- */
        int lng = 0; /* 0 = int, 1 = long, 2 = long long */
        while (*fmt == 'l') { lng++; fmt++; }

        char conv = *fmt++;

        /* Output assembly. For numeric conversions we build digits into
           numbuf (right to left); for %c/%s we point at the source. */
        const char *out = numbuf;
        int outlen = 0;
        char sign = 0;
        int is_num = 0;
        unsigned long long uv = 0;
        int base = 10, upper = 0;

        switch (conv) {
        case 'c':
            numbuf[0] = (char)va_arg(ap, int);
            outlen = 1;
            break;
        case 's':
            out = va_arg(ap, const char *);
            if (!out) out = "(null)";
            { const char *p = out; while (*p++) outlen++; }
            break;
        case '%':
            numbuf[0] = '%';
            outlen = 1;
            break;
        case 'd':
        case 'i': {
            long long sv = (lng >= 2) ? va_arg(ap, long long)
                         : (lng == 1) ? va_arg(ap, long)
                                      : va_arg(ap, int);
            if (sv < 0) { sign = '-'; uv = (unsigned long long)(-sv); }
            else        { uv = (unsigned long long)sv; if (plus) sign = '+'; else if (space) sign = ' '; }
            is_num = 1;
            break;
        }
        case 'u':
            uv = (lng >= 2) ? va_arg(ap, unsigned long long)
               : (lng == 1) ? va_arg(ap, unsigned long)
                            : va_arg(ap, unsigned int);
            is_num = 1;
            break;
        case 'p':
            uv = (unsigned long long)(uintptr_t)va_arg(ap, void *);
            base = 16;
            is_num = 1;
            break;
        case 'X':
            upper = 1;
            /* fall through */
        case 'x':
            uv = (lng >= 2) ? va_arg(ap, unsigned long long)
               : (lng == 1) ? va_arg(ap, unsigned long)
                            : va_arg(ap, unsigned int);
            base = 16;
            is_num = 1;
            break;
        default:
            /* unknown conversion: emit literally */
            uart_putc('%');
            uart_putc(conv);
            count += 2;
            continue;
        }

        if (is_num) {
            const char *digs = upper ? "0123456789ABCDEF" : "0123456789abcdef";
            char *p = numbuf + sizeof(numbuf);
            int len = 0;
            if (uv == 0) { *--p = '0'; len = 1; }
            else while (uv) { *--p = digs[uv % base]; uv /= base; len++; }
            out = p;
            outlen = len;
        }

        /* ---- padding ---- */
        int signlen = sign ? 1 : 0;
        int pad = width - (outlen + signlen);
        if (pad < 0) pad = 0;

        if (!left && !zero) { while (pad-- > 0) { uart_putc(' '); count++; } }
        if (sign)           { uart_putc(sign); count++; }
        if (!left && zero)  { while (pad-- > 0) { uart_putc('0'); count++; } }
        for (int i = 0; i < outlen; i++) { uart_putc(out[i]); count++; }
        if (left)           { while (pad-- > 0) { uart_putc(' '); count++; } }
    }

    return count;
}

int uprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = uvprintf(fmt, ap);
    va_end(ap);
    return n;
}
