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

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#ifndef CPU_HZ
#define CPU_HZ 50000000UL
#endif

uint32_t cycles32(void);

uint64_t cycles64(void);

static inline uint64_t cycles_to_us(uint64_t cyc) { return cyc / (CPU_HZ / 1000000UL); }
static inline uint64_t cycles_to_ms(uint64_t cyc) { return cyc / (CPU_HZ / 1000UL); }

void delay_cycles(uint64_t n);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

#endif
