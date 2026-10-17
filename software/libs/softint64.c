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

/*
 * softint64.c — 64-bit integer divide/modulo helpers, compiled in the
 * target's own (soft-float ilp32) ABI.
 *
 * Why this exists: the installed riscv32-unknown-elf toolchain is built
 * --with-arch=rv32gc --with-abi=ilp32d (hard float) and ships no soft-float
 * multilib, so its libgcc.a cannot be linked against our soft-float
 * (-mabi=ilp32) objects ("can't link double-float modules with soft-float
 * modules"). Pure 32-bit integer code never needs libgcc (the M extension
 * inlines mul/div/rem), but any 64-bit division emits a call to __udivdi3 /
 * __umoddi3. Providing them here lets us use 64-bit math while linking with
 * -nostdlib and no -lgcc.
 *
 * 32/64-bit multiply and variable shifts are inlined by GCC (M extension),
 * so only the division helpers are required.
 */

typedef unsigned long long u64;

u64 __udivmoddi4(u64 n, u64 d, u64 *rem)
{
    u64 q = 0, r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1ULL);
        if (r >= d) {
            r -= d;
            q |= (1ULL << i);
        }
    }
    if (rem)
        *rem = r;
    return q;
}

u64 __udivdi3(u64 n, u64 d)
{
    return __udivmoddi4(n, d, 0);
}

u64 __umoddi3(u64 n, u64 d)
{
    u64 r;
    __udivmoddi4(n, d, &r);
    return r;
}
