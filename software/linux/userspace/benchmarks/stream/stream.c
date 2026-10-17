/*
 * stream_zcore.c — a VARIANT of the STREAM benchmark for Z-Core.
 *
 * Based on John D. McCalpin's STREAM (https://www.cs.virginia.edu/stream/),
 * Copyright 1991-2013 John D. McCalpin. Per the STREAM license, results from
 * this modified source must be labelled as "a variant of the STREAM
 * benchmark", NOT as official "STREAM benchmark results". The four kernels
 * (Copy/Scale/Add/Triad), the run-NTIMES-take-best methodology, and the
 * recurrence-based validation follow the original; the I/O, timing and data
 * type are adapted for bare-metal Z-Core.
 *
 * Why integer instead of double:
 *   Z-Core is RV32IM with no FPU, and the installed toolchain ships only a
 *   hard-float libgcc, so soft-float double cannot be linked (see
 *   libs/softint64.c). STREAM_TYPE is therefore a 32-bit int. This is also
 *   the right choice for a *memory* bandwidth benchmark on a no-FPU core:
 *   integer Scale/Triad use the single-cycle M-extension multiply, keeping
 *   the kernels memory-bound rather than soft-float-bound.
 *
 *   Note when reading results: Copy and Add do no multiplies at all, so they
 *   are the cleanest memory-bandwidth figures; Scale and Triad include one
 *   integer multiply per element.
 *
 * Runtime: libs/{crt0.S,uart.c,printf.c,timer.c,softint64.c}.
 */

#include <stdint.h>
#include "uart.h"
#include "printf.h"
#include "timer.h"

#ifdef STREAM_USE_DOUBLE
#error "double STREAM_TYPE is not buildable on this hard-float-only toolchain (no soft-float libgcc). Use the default int type."
#endif

#ifndef STREAM_ARRAY_SIZE
#define STREAM_ARRAY_SIZE 500000
#endif

#ifndef NTIMES
#define NTIMES 20
#endif

#ifndef OFFSET
#define OFFSET 0
#endif

typedef int STREAM_TYPE;
#define SCALAR 3

#define N ((unsigned long)STREAM_ARRAY_SIZE)

static STREAM_TYPE a[STREAM_ARRAY_SIZE + OFFSET] ;
static STREAM_TYPE b[STREAM_ARRAY_SIZE + OFFSET] ;
static STREAM_TYPE c[STREAM_ARRAY_SIZE + OFFSET] ;

static uint64_t times[4][NTIMES];

static const char *label[4] = { "Copy:  ", "Scale: ", "Add:   ", "Triad: " };

static const unsigned long bytes_per[4] = {
    2UL * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE,
    2UL * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE,
    3UL * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE,
    3UL * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE
};

static void print_banner(void)
{
    uprintf("\r\n");
    uprintf("-------------------------------------------------------------\r\n");
    uprintf(" STREAM (variant) -- Z-Core RV32IM @ %u MHz, int32 kernels\r\n",
            (unsigned)(CPU_HZ / 1000000UL));
    uprintf("-------------------------------------------------------------\r\n");
    uprintf(" Array size   = %lu elements x 3 arrays\r\n", N);
    uprintf(" Element size = %u bytes\r\n", (unsigned)sizeof(STREAM_TYPE));
    uprintf(" Memory used  = %lu KB total\r\n",
            (unsigned long)((uint64_t)3 * STREAM_ARRAY_SIZE * sizeof(STREAM_TYPE) / 1024));
    uprintf(" NTIMES       = %u (first iteration discarded)\r\n", (unsigned)NTIMES);
    uprintf("-------------------------------------------------------------\r\n");
}

int main(void)
{
    unsigned long j;
    int k;
    STREAM_TYPE scalar = SCALAR;

    uart_set_baud(27);

    print_banner();

    for (j = 0; j < N; j++) { a[j] = 1; b[j] = 2; c[j] = 0; }
    for (j = 0; j < N; j++) { a[j] = 2 * a[j]; }

    uprintf("Running %u iterations of Copy/Scale/Add/Triad...\r\n\r\n", (unsigned)NTIMES);

    for (k = 0; k < NTIMES; k++) {
        uint64_t t;

        t = cycles64();
        for (j = 0; j < N; j++) c[j] = a[j];
        times[0][k] = cycles64() - t;

        t = cycles64();
        for (j = 0; j < N; j++) b[j] = scalar * c[j];
        times[1][k] = cycles64() - t;

        t = cycles64();
        for (j = 0; j < N; j++) c[j] = a[j] + b[j];
        times[2][k] = cycles64() - t;

        t = cycles64();
        for (j = 0; j < N; j++) a[j] = b[j] + scalar * c[j];
        times[3][k] = cycles64() - t;
    }

    uprintf("Function   Best MB/s    Best (us)     Avg (us)\r\n");
    for (int f = 0; f < 4; f++) {
        uint64_t best = times[f][1];
        uint64_t sum = 0;
        for (k = 1; k < NTIMES; k++) {
            if (times[f][k] < best) best = times[f][k];
            sum += times[f][k];
        }
        uint64_t avg = sum / (NTIMES - 1);

        uint64_t bps = (uint64_t)bytes_per[f] * CPU_HZ / best;
        uint32_t mb_whole = (uint32_t)(bps / 1000000ULL);
        uint32_t mb_frac  = (uint32_t)((bps % 1000000ULL) / 10000ULL);

        uprintf("%s%6u.%02u  %11llu  %11llu\r\n",
                label[f], mb_whole, mb_frac,
                (unsigned long long)cycles_to_us(best),
                (unsigned long long)cycles_to_us(avg));
    }

    {
        STREAM_TYPE aj = 1, bj = 2, cj = 0;
        unsigned long errs = 0;
        aj = 2 * aj;
        for (k = 0; k < NTIMES; k++) {
            cj = aj;
            bj = scalar * cj;
            cj = aj + bj;
            aj = bj + scalar * cj;
        }
        for (j = 0; j < N; j++) {
            if (a[j] != aj || b[j] != bj || c[j] != cj) errs++;
        }
        uprintf("\r\n");
        if (errs == 0)
            uprintf("Validation: PASSED\r\n");
        else
            uprintf("Validation: FAILED (%lu element mismatches)\r\n", errs);
    }

    uprintf("-------------------------------------------------------------\r\n");
    uprintf("Done.\r\n");

    return 0;
}
