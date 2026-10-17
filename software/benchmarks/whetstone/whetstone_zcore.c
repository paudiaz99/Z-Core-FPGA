/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025–2026 Pau Díaz Cuesta
 *
 * Q16.16 fixed-point Whetstone-style workload for Z-Core (RV32IM, no FPU).
 * Algorithm lineage: classic Whetstone; C conversion © 1998 Painter
 * Engineering, Inc. (see software/linux/userspace/benchmarks/original/whetstone.c
 * — redistribute that file only with its full comment block). This port is
 * new source under MIT; it is not a verbatim copy of the Painter program.
 */

#include <stdint.h>
#include "uart.h"
#include "printf.h"
#include "timer.h"


typedef int32_t  fixed_t;

#define FIX_SHIFT   16
#define FIX_ONE     (1 << FIX_SHIFT)
#define FIX_HALF    (1 << (FIX_SHIFT-1))

static inline fixed_t fix_from_int(int32_t x) { return x << FIX_SHIFT; }

static inline fixed_t fix_mul(fixed_t a, fixed_t b)
{
    return (fixed_t)(((int64_t)a * (int64_t)b) >> FIX_SHIFT);
}

static inline fixed_t fix_div(fixed_t a, fixed_t b)
{
    if (b == 0) return (a >= 0) ? 0x7FFFFFFF : (fixed_t)0x80000001;
    int neg = 0;
    uint64_t ua, ub;
    if (a < 0) { neg ^= 1; ua = (uint64_t)(-(int64_t)a); } else { ua = (uint64_t)a; }
    if (b < 0) { neg ^= 1; ub = (uint64_t)(-(int64_t)b); } else { ub = (uint64_t)b; }
    uint64_t result = (ua << FIX_SHIFT) / ub;
    return neg ? -(fixed_t)result : (fixed_t)result;
}

static inline fixed_t fix_abs(fixed_t x) { return x < 0 ? -x : x; }

static fixed_t fix_sin(fixed_t x) __attribute__((noinline));
static fixed_t fix_sin(fixed_t x)
{
    fixed_t x2 = fix_mul(x, x);
    fixed_t x3 = fix_mul(x2, x);
    return x - fix_mul(x3, 10923);
}

static fixed_t fix_cos(fixed_t x) __attribute__((noinline));
static fixed_t fix_cos(fixed_t x)
{
    fixed_t x2 = fix_mul(x, x);
    return FIX_ONE - (x2 >> 1);
}

static fixed_t fix_atan(fixed_t x) __attribute__((noinline));
static fixed_t fix_atan(fixed_t x)
{
    fixed_t x2 = fix_mul(x, x);
    fixed_t x3 = fix_mul(x2, x);
    return x - fix_mul(x3, 21845);
}

static fixed_t fix_sqrt(fixed_t x) __attribute__((noinline));
static fixed_t fix_sqrt(fixed_t x)
{
    if (x <= 0) return 0;
    fixed_t guess = x;
    if (guess > FIX_ONE) guess = FIX_ONE + ((x - FIX_ONE) >> 1);
    for (int i = 0; i < 6; i++) {
        if (guess == 0) return 0;
        guess = (guess + fix_div(x, guess)) >> 1;
    }
    return guess;
}

static fixed_t fix_exp(fixed_t x) __attribute__((noinline));
static fixed_t fix_exp(fixed_t x)
{
    fixed_t term = FIX_ONE;
    fixed_t sum  = FIX_ONE;
    for (int i = 1; i <= 5; i++) {
        term = fix_mul(term, fix_div(x, fix_from_int(i)));
        sum += term;
    }
    return sum;
}

static fixed_t fix_log(fixed_t x) __attribute__((noinline));
static fixed_t fix_log(fixed_t x)
{
    if (x <= 0) return -fix_from_int(100);
    fixed_t y = fix_div(x - FIX_ONE, x + FIX_ONE);
    fixed_t y2 = fix_mul(y, y);
    fixed_t y3 = fix_mul(y2, y);
    return 2 * (y + fix_mul(y3, 21845));
}


#define ARRAY_SIZE 4
static fixed_t e1[ARRAY_SIZE + 1];

#ifndef WHET_LOOPS
#define WHET_LOOPS 1000
#endif

static volatile fixed_t sink;

static void __attribute__((noinline))
module1(int n)
{
    fixed_t x1 = fix_from_int(1);
    fixed_t y1 = fix_from_int(-1);
    fixed_t z1 = fix_from_int(1);
    for (int i = 0; i < n; i++) {
        x1 = (x1 + y1 + z1 - fix_mul(e1[3], e1[2])) >> 1;
        y1 = (y1 + z1 + x1 + fix_mul(e1[4], e1[3])) >> 1;
        z1 = (z1 + x1 + y1 - fix_mul(e1[2], e1[1])) >> 1;
        e1[1] = x1;
        e1[2] = y1;
        e1[3] = z1;
    }
    sink = z1;
}

static void __attribute__((noinline))
module2(int n, fixed_t arr[], int len)
{
    (void)len;
    for (int i = 0; i < n; i++) {
        for (int j = 1; j < len; j++) {
            arr[j] = arr[j - 1] + fix_mul(arr[j], fix_from_int(j));
            arr[j] = fix_div(arr[j], fix_from_int(j + 1));
        }
    }
    sink = arr[len - 1];
}

static void __attribute__((noinline))
module3(int n)
{
    int j = 1;
    for (int i = 0; i < n; i++) {
        if (j == 1) j = 2; else j = 3;
        if (j > 2)  j = 0; else j = 1;
        if (j < 1)  j = 1; else j = 0;
    }
    sink = fix_from_int(j);
}

static void __attribute__((noinline))
module4(int n)
{
    int32_t j = 45;
    int32_t k = 23;
    int32_t l = 7;
    for (int i = 0; i < n; i++) {
        j = j * (k - j) * (l - k);
        k = l * k - (l - j) * k;
        l = (l - k) * (k + j);
        j = j % 1000 + 1;
        k = k % 1000 + 1;
        l = l % 100 + 1;
        e1[l % ARRAY_SIZE + 1] = fix_from_int(j + k + l);
    }
    sink = fix_from_int(j + k + l);
}

static void __attribute__((noinline))
module5(int n)
{
    fixed_t x = FIX_HALF;
    fixed_t y = FIX_HALF;
    for (int i = 0; i < n; i++) {
        x = fix_sin(x) + fix_cos(y);
        y = fix_sin(y) + fix_cos(x);
        fixed_t a = fix_atan(fix_div(x, y + FIX_ONE));
        x = fix_mul(a, x);
        y = fix_mul(a, y);
    }
    sink = x + y;
}

static fixed_t __attribute__((noinline))
proc_called(fixed_t a, fixed_t b, fixed_t c, fixed_t d)
{
    return fix_mul(a + b, c - d);
}

static void __attribute__((noinline))
module6(int n)
{
    fixed_t x = FIX_ONE;
    fixed_t y = FIX_ONE;
    fixed_t z = FIX_ONE;
    for (int i = 0; i < n; i++) {
        x = proc_called(x, y, z, fix_from_int(i % 7 + 1));
        y = proc_called(y, z, x, fix_from_int(i % 5 + 1));
        z = proc_called(z, x, y, fix_from_int(i % 3 + 1));
        if (fix_abs(x) > fix_from_int(1000)) x = FIX_ONE;
        if (fix_abs(y) > fix_from_int(1000)) y = FIX_ONE;
        if (fix_abs(z) > fix_from_int(1000)) z = FIX_ONE;
    }
    sink = x + y + z;
}

static void __attribute__((noinline))
module7(int n)
{
    fixed_t arr[10];
    for (int j = 0; j < 10; j++) arr[j] = fix_from_int(j + 1);

    for (int i = 0; i < n; i++) {
        for (int j = 1; j < 10; j++) {
            arr[j] = arr[j - 1] + fix_mul(arr[j], FIX_HALF);
        }
        if (fix_abs(arr[9]) > fix_from_int(100000)) {
            for (int j = 0; j < 10; j++)
                arr[j] = fix_from_int(j + 1);
        }
    }
    sink = arr[9];
}

static void __attribute__((noinline))
module8(int n)
{
    fixed_t x = FIX_ONE;
    for (int i = 0; i < n; i++) {
        fixed_t t = fix_mul(x, x);
        t = fix_sqrt(t);
        t = fix_log(t + 1);
        t = fix_exp(t);
        x = t;
        if (x > fix_from_int(100)) x = FIX_ONE;
        if (x < -fix_from_int(100)) x = FIX_ONE;
    }
    sink = x;
}


int main(void)
{
    uart_set_baud(27);

    volatile int loops = WHET_LOOPS;

    int w1 = 12 * loops;
    int w2 =  4 * loops;
    int w3 = 12 * loops;
    int w4 = 12 * loops;
    int w5 =  4 * loops;
    int w6 =  8 * loops;
    int w7 =  4 * loops;
    int w8 =  2 * loops;

    e1[0] = FIX_ONE;
    e1[1] = fix_from_int(-1);
    e1[2] = fix_from_int(-1);
    e1[3] = fix_from_int(-1);
    e1[4] = fix_from_int(-1);

    uprintf("\r\n");
    uprintf("=============================================================\r\n");
    uprintf(" Whetstone (integer) -- Z-Core RV32IM @ %u MHz\r\n",
            (unsigned)(CPU_HZ / 1000000UL));
    uprintf("=============================================================\r\n");
    uprintf(" Loops: %d   (Q16.16 fixed-point, no FPU)\r\n", loops);
    uprintf(" Running 8 modules...\r\n");

    uint64_t start = cycles64();

    module1(w1);
    module2(w2, e1, ARRAY_SIZE + 1);
    module3(w3);
    module4(w4);
    module5(w5);
    module6(w6);
    module7(w7);
    module8(w8);

    uint64_t end = cycles64();
    uint64_t elapsed = end - start;

    uint64_t elapsed_us = cycles_to_us(elapsed);
    uint64_t elapsed_ms = cycles_to_ms(elapsed);

    uint32_t kwips_x100 = 0;
    if (elapsed_us > 0) {
        kwips_x100 = (uint32_t)((uint64_t)loops * 10000000ULL / elapsed_us);
    }

    uprintf("\r\n");
    uprintf("-------------------------------------------------------------\r\n");
    uprintf(" Results\r\n");
    uprintf("-------------------------------------------------------------\r\n");
    uprintf(" Loops              : %d\r\n", loops);
    uprintf(" Elapsed cycles     : %llu\r\n", (unsigned long long)elapsed);
    uprintf(" Elapsed time       : %llu ms\r\n", (unsigned long long)elapsed_ms);
    uprintf(" KWIPS              : %u.%02u\r\n",
            kwips_x100 / 100, kwips_x100 % 100);
    uprintf("=============================================================\r\n");

    while (1)
        ;
    return 0;
}
