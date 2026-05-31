
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

#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>

/* Compact formatted output over the Z-Core UART (uses uart_putc()).
 *
 * Supported conversions: %c %s %d %i %u %x %X %p %%
 * Length modifiers:       l (long), ll (long long)
 * Flags:                  '-' (left align), '0' (zero pad), '+', ' '
 * Field width:            decimal, e.g. %8u, %012llx
 *
 * No floating-point support (Z-Core has no FPU); format fixed-point
 * values yourself, e.g.  uprintf("%u.%02u", whole, frac);
 */
int uprintf(const char *fmt, ...);
int uvprintf(const char *fmt, va_list ap);

#endif /* PRINTF_H */
