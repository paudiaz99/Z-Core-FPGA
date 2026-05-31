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

#include "timer.h"

uint32_t cycles32(void)
{
    uint32_t c;
    asm volatile("csrr %0, cycle" : "=r"(c));
    return c;
}

uint64_t cycles64(void)
{
    uint32_t hi, lo, hi2;
    /* Re-read the high word and retry if it changed mid-read (low word
       rolled over between the two CSR reads). */
    do {
        asm volatile("csrr %0, cycleh" : "=r"(hi));
        asm volatile("csrr %0, cycle"  : "=r"(lo));
        asm volatile("csrr %0, cycleh" : "=r"(hi2));
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
}

void delay_cycles(uint64_t n)
{
    uint64_t start = cycles64();
    while ((cycles64() - start) < n)
        ;
}

void delay_us(uint32_t us)
{
    delay_cycles((uint64_t)us * (CPU_HZ / 1000000UL));
}

void delay_ms(uint32_t ms)
{
    delay_cycles((uint64_t)ms * (CPU_HZ / 1000UL));
}
