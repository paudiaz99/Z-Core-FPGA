/*
Copyright 2018 Embedded Microprocessor Benchmark Consortium (EEMBC)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Derived from coremark/barebones/ee_printf.c — uart_send_char implemented
for Z-Core (UART TX at 0x04000000).
*/

#include <coremark.h>
#include <stdarg.h>

#define ZEROPAD   (1<<0)
#define SIGN      (1<<1)
#define PLUS      (1<<2)
#define SPACE     (1<<3)
#define LEFT      (1<<4)
#define HEX_PREP  (1<<5)
#define UPPERCASE (1<<6)

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static char *digits       = "0123456789abcdefghijklmnopqrstuvwxyz";
static char *upper_digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static ee_size_t strnlen(const char *s, ee_size_t count)
{
    const char *sc;
    for (sc = s; *sc != '\0' && count--; ++sc);
    return sc - s;
}

static int skip_atoi(const char **s)
{
    int i = 0;
    while (is_digit(**s)) i = i * 10 + *((*s)++) - '0';
    return i;
}

static char *number(char *str, long num, int base, int size, int precision, int type)
{
    char c, sign, tmp[66];
    char *dig = digits;
    int i;

    if (type & UPPERCASE)  dig = upper_digits;
    if (type & LEFT) type &= ~ZEROPAD;
    if (base < 2 || base > 36) return 0;

    c = (type & ZEROPAD) ? '0' : ' ';
    sign = 0;
    if (type & SIGN)
    {
        if (num < 0)
        {
            sign = '-';
            num = -num;
            size--;
        }
        else if (type & PLUS)
        {
            sign = '+';
            size--;
        }
        else if (type & SPACE)
        {
            sign = ' ';
            size--;
        }
    }

    if (type & HEX_PREP)
    {
        if (base == 16) size -= 2;
        else if (base == 8) size--;
    }

    i = 0;
    if (num == 0)
        tmp[i++] = '0';
    else
    {
        while (num != 0)
        {
            tmp[i++] = dig[((unsigned long) num) % (unsigned) base];
            num = ((unsigned long) num) / (unsigned) base;
        }
    }

    if (i > precision) precision = i;
    size -= precision;
    if (!(type & (ZEROPAD | LEFT))) while (size-- > 0) *str++ = ' ';
    if (sign) *str++ = sign;

    if (type & HEX_PREP)
    {
        if (base == 8)
            *str++ = '0';
        else if (base == 16)
        {
            *str++ = '0';
            *str++ = digits[33];
        }
    }

    if (!(type & LEFT)) while (size-- > 0) *str++ = c;
    while (i < precision--) *str++ = '0';
    while (i-- > 0) *str++ = tmp[i];
    while (size-- > 0) *str++ = ' ';

    return str;
}

static char *eaddr(char *str, unsigned char *addr, int size, int precision, int type)
{
    char tmp[24];
    char *dig = digits;
    int i, len;

    if (type & UPPERCASE) dig = upper_digits;
    len = 0;
    for (i = 0; i < 6; i++)
    {
        if (i != 0) tmp[len++] = ':';
        tmp[len++] = dig[addr[i] >> 4];
        tmp[len++] = dig[addr[i] & 0x0F];
    }

    if (!(type & LEFT)) while (len < size--) *str++ = ' ';
    for (i = 0; i < len; ++i) *str++ = tmp[i];
    while (len < size--) *str++ = ' ';

    return str;
}

static char *iaddr(char *str, unsigned char *addr, int size, int precision, int type)
{
    char tmp[24];
    int i, n, len;

    len = 0;
    for (i = 0; i < 4; i++)
    {
        if (i != 0) tmp[len++] = '.';
        n = addr[i];
        if (n == 0)
        {
            tmp[len++] = digits[0];
        }
        else
        {
            if (n >= 100) { tmp[len++] = digits[n / 100]; n %= 100; tmp[len++] = digits[n / 10]; n %= 10; }
            else if (n >= 10) { tmp[len++] = digits[n / 10]; n %= 10; }
            tmp[len++] = digits[n];
        }
    }

    if (!(type & LEFT)) while (len < size--) *str++ = ' ';
    for (i = 0; i < len; ++i) *str++ = tmp[i];
    while (len < size--) *str++ = ' ';

    return str;
}

#if HAS_FLOAT
char *ecvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf);
char *fcvtbuf(double arg, int ndigits, int *decpt, int *sign, char *buf);
static void ee_bufcpy(char *pd, char *ps, int count) { char *pe = ps + count; while (ps != pe) *pd++ = *ps++; }

static void parse_float(double value, char *buffer, char fmt, int precision)
{
    int decpt, sign, exp, pos;
    char *dig = NULL;
    char cvtbuf[80];
    int capexp = 0, magnitude;

    if (fmt == 'G' || fmt == 'E') { capexp = 1; fmt += 'a' - 'A'; }

    if (fmt == 'g')
    {
        dig = ecvtbuf(value, precision, &decpt, &sign, cvtbuf);
        magnitude = decpt - 1;
        if (magnitude < -4 || magnitude > precision - 1) { fmt = 'e'; precision -= 1; }
        else { fmt = 'f'; precision -= decpt; }
    }

    if (fmt == 'e')
    {
        dig = ecvtbuf(value, precision + 1, &decpt, &sign, cvtbuf);
        if (sign) *buffer++ = '-';
        *buffer++ = *dig;
        if (precision > 0) *buffer++ = '.';
        ee_bufcpy(buffer, dig + 1, precision);
        buffer += precision;
        *buffer++ = capexp ? 'E' : 'e';
        if (decpt == 0) exp = (value == 0.0) ? 0 : -1;
        else exp = decpt - 1;
        if (exp < 0) { *buffer++ = '-'; exp = -exp; } else *buffer++ = '+';
        buffer[2] = (exp % 10) + '0'; exp /= 10;
        buffer[1] = (exp % 10) + '0'; exp /= 10;
        buffer[0] = (exp % 10) + '0';
        buffer += 3;
    }
    else if (fmt == 'f')
    {
        dig = fcvtbuf(value, precision, &decpt, &sign, cvtbuf);
        if (sign) *buffer++ = '-';
        if (*dig)
        {
            if (decpt <= 0) { *buffer++ = '0'; *buffer++ = '.'; for (pos = 0; pos < -decpt; pos++) *buffer++ = '0'; while (*dig) *buffer++ = *dig++; }
            else { pos = 0; while (*dig) { if (pos++ == decpt) *buffer++ = '.'; *buffer++ = *dig++; } }
        }
        else
        {
            *buffer++ = '0';
            if (precision > 0) { *buffer++ = '.'; for (pos = 0; pos < precision; pos++) *buffer++ = '0'; }
        }
    }
    *buffer = '\0';
}

static char *flt(char *str, double num, int size, int precision, char fmt, int flags)
{
    char tmp[80], c, sign;
    int n, i;
    if (flags & LEFT) flags &= ~ZEROPAD;
    c = (flags & ZEROPAD) ? '0' : ' ';
    sign = 0;
    if (flags & SIGN)
    {
        if (num < 0.0) { sign = '-'; num = -num; size--; }
        else if (flags & PLUS) { sign = '+'; size--; }
        else if (flags & SPACE) { sign = ' '; size--; }
    }
    if (precision < 0) precision = 6;
    parse_float(num, tmp, fmt, precision);
    n = strnlen(tmp, 256);
    size -= n;
    if (!(flags & (ZEROPAD | LEFT))) while (size-- > 0) *str++ = ' ';
    if (sign) *str++ = sign;
    if (!(flags & LEFT)) while (size-- > 0) *str++ = c;
    for (i = 0; i < n; i++) *str++ = tmp[i];
    while (size-- > 0) *str++ = ' ';
    return str;
}
#endif

static int ee_vsprintf(char *buf, const char *fmt, va_list args)
{
    int len;
    unsigned long num;
    int i, base;
    char *str, *s;
    int flags, field_width, precision, qualifier;

    for (str = buf; *fmt; fmt++)
    {
        if (*fmt != '%') { *str++ = *fmt; continue; }

        flags = 0;
repeat:
        fmt++;
        switch (*fmt)
        {
            case '-': flags |= LEFT;    goto repeat;
            case '+': flags |= PLUS;    goto repeat;
            case ' ': flags |= SPACE;   goto repeat;
            case '#': flags |= HEX_PREP; goto repeat;
            case '0': flags |= ZEROPAD; goto repeat;
        }

        field_width = -1;
        if (is_digit(*fmt))
            field_width = skip_atoi(&fmt);
        else if (*fmt == '*')
        {
            fmt++;
            field_width = va_arg(args, int);
            if (field_width < 0) { field_width = -field_width; flags |= LEFT; }
        }

        precision = -1;
        if (*fmt == '.')
        {
            ++fmt;
            if (is_digit(*fmt))       precision = skip_atoi(&fmt);
            else if (*fmt == '*')     { ++fmt; precision = va_arg(args, int); }
            if (precision < 0) precision = 0;
        }

        qualifier = -1;
        if (*fmt == 'l' || *fmt == 'L') { qualifier = *fmt; fmt++; }

        base = 10;

        switch (*fmt)
        {
            case 'c':
                if (!(flags & LEFT)) while (--field_width > 0) *str++ = ' ';
                *str++ = (unsigned char) va_arg(args, int);
                while (--field_width > 0) *str++ = ' ';
                continue;

            case 's':
                s = va_arg(args, char *);
                if (!s) s = "<NULL>";
                len = strnlen(s, precision);
                if (!(flags & LEFT)) while (len < field_width--) *str++ = ' ';
                for (i = 0; i < len; ++i) *str++ = *s++;
                while (len < field_width--) *str++ = ' ';
                continue;

            case 'p':
                if (field_width == -1) { field_width = 2 * sizeof(void *); flags |= ZEROPAD; }
                str = number(str, (unsigned long) va_arg(args, void *), 16, field_width, precision, flags);
                continue;

            case 'A': flags |= UPPERCASE;
            case 'a':
                if (qualifier == 'l')
                    str = eaddr(str, va_arg(args, unsigned char *), field_width, precision, flags);
                else
                    str = iaddr(str, va_arg(args, unsigned char *), field_width, precision, flags);
                continue;

            case 'o': base = 8; break;
            case 'X': flags |= UPPERCASE;
            case 'x': base = 16; break;
            case 'd':
            case 'i': flags |= SIGN;
            case 'u': break;

#if HAS_FLOAT
            case 'f':
                str = flt(str, va_arg(args, double), field_width, precision, *fmt, flags | SIGN);
                continue;
#endif

            default:
                if (*fmt != '%') *str++ = '%';
                if (*fmt) *str++ = *fmt;
                else --fmt;
                continue;
        }

        if (qualifier == 'l')      num = va_arg(args, unsigned long);
        else if (flags & SIGN)     num = va_arg(args, int);
        else                       num = va_arg(args, unsigned int);

        str = number(str, num, base, field_width, precision, flags);
    }

    *str = '\0';
    return str - buf;
}

#define UART_BASE          0x04000000U
#define UART_TX_REG        (*(volatile unsigned int *)(UART_BASE + 0x00))
#define UART_STAT_REG      (*(volatile unsigned int *)(UART_BASE + 0x08))
#define UART_STAT_TX_EMPTY 0x01

void uart_send_char(char c)
{
    UART_TX_REG = (unsigned int)c;
    while (!(UART_STAT_REG & UART_STAT_TX_EMPTY))
        ;
}

int ee_printf(const char *fmt, ...)
{
    char buf[256], *p;
    va_list args;
    int n = 0;

    va_start(args, fmt);
    ee_vsprintf(buf, fmt, args);
    va_end(args);

    p = buf;
    while (*p) { uart_send_char(*p); n++; p++; }

    return n;
}
