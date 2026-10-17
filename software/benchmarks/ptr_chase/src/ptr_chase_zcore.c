/*
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019 Milan Radulovic et al. (BSC pointer-chase)
 * Copyright (c) 2025–2026 Pau Díaz Cuesta (Z-Core port)
 *
 * ptr_chase_zcore.c — pointer-chase memory-latency benchmark for Z-Core.
 *
 * A single-binary adaptation of the BSC pointer-chase benchmark
 * (arraygen + ptr_chase, Copyright (c) 2019 Milan Radulovic et al., BSD-3),
 * merged so the random walk is generated in memory (no array.dat file, no
 * libc) and run directly on bare-metal Z-Core.
 * Full license text: ../LICENSE
 *
 * Method: build a random single Hamiltonian cycle over N elements with
 * Sattolo's algorithm, then chase dependent loads (off = mem[base+off]) so
 * each access must complete before the next address is known — this exposes
 * the full load-to-use latency with no memory-level parallelism. Sweeping the
 * working-set size from below to far above the cache size yields a latency
 * curve: small sizes measure cache-hit latency, large sizes measure SDRAM
 * latency, and the knee reveals the cache capacity.
 *
 * Z-Core specifics:
 *   - D-cache is 2-way, 32 KB, with a 4-byte line (CACHE_LINE_SIZE=4). The
 *     element stride is therefore 4 bytes: every access touches a distinct
 *     line, so working-set bytes == cached footprint and the knee lands at
 *     the true cache size.
 *   - The chase stores successor byte-offsets (index*4); the inner loop is a
 *     compact .rept unroll (replaces the original 8000-line loop.h) using lw.
 *
 * Runtime: libs/{crt0.S,uart.c,printf.c,timer.c,softint64.c}.
 */

#include <stdint.h>
#include "uart.h"
#include "printf.h"
#include "timer.h"

#define STR(x)  #x
#define XSTR(x) STR(x)

#define CHASE_UNROLL 32

#define MAX_BYTES (8u * 1024u * 1024u)
#define MAX_ELEMS (MAX_BYTES / 4u)

#define ACCESSES (1u << 20)

static uint32_t g_buf[MAX_ELEMS] __attribute__((section(".noinit")));

static const uint32_t ws_bytes[] = {
    1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072,
    262144, 524288, 1048576, 2097152, 4194304, 8388608
};
#define NSIZES (sizeof(ws_bytes) / sizeof(ws_bytes[0]))

static uint32_t xs = 0x12345678u;
static inline uint32_t xrand(void)
{
    uint32_t x = xs;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    xs = x;
    return x;
}

static void gen_cycle(uint32_t n)
{
    for (uint32_t i = 0; i < n; i++)
        g_buf[i] = i;
    for (uint32_t i = n - 1; i > 0; i--) {
        uint32_t j = xrand() % i;
        uint32_t t = g_buf[i];
        g_buf[i] = g_buf[j];
        g_buf[j] = t;
    }
    for (uint32_t i = 0; i < n; i++)
        g_buf[i] <<= 2;
}

static uint32_t do_chase(const void *base, uint32_t start_off, uint32_t naccesses)
{
    uint32_t off = start_off;
    uint32_t cnt = naccesses;
    __asm__ __volatile__(
        "1:\n"
        ".rept " XSTR(CHASE_UNROLL) "\n"
        "  add  t0, %[base], %[off]\n"
        "  lw   %[off], 0(t0)\n"
        ".endr\n"
        "  addi %[cnt], %[cnt], -" XSTR(CHASE_UNROLL) "\n"
        "  bgtz %[cnt], 1b\n"
        : [off] "+r"(off), [cnt] "+r"(cnt)
        : [base] "r"(base)
        : "t0", "memory");
    return off;
}

int main(void)
{
    uart_set_baud(27);

    uprintf("\r\n");
    uprintf("-------------------------------------------------------------\r\n");
    uprintf(" Pointer-chase latency -- Z-Core RV32IM @ %u MHz\r\n",
            (unsigned)(CPU_HZ / 1000000UL));
    uprintf(" D-cache: 2-way, 32 KB, 4 B line. Stride = 4 B (1 line/access).\r\n");
    uprintf(" %u dependent loads per size; random single-cycle walk.\r\n",
            (unsigned)ACCESSES);
    uprintf("-------------------------------------------------------------\r\n");
    uprintf(" WorkSet(B)   Elems   cyc/access   ns/access\r\n");

    volatile uint32_t sink = 0;

    for (unsigned s = 0; s < NSIZES; s++) {
        uint32_t bytes = ws_bytes[s];
        uint32_t n = bytes / 4u;
        if (n < CHASE_UNROLL) continue;

        gen_cycle(n);

        sink ^= do_chase(g_buf, 0, ACCESSES);

        uint64_t t0 = cycles64();
        uint32_t last = do_chase(g_buf, 0, ACCESSES);
        uint64_t elapsed = cycles64() - t0;
        sink ^= last;

        uint64_t lat_x100 = elapsed * 100ULL / ACCESSES;
        uint32_t lw = (uint32_t)(lat_x100 / 100);
        uint32_t lf = (uint32_t)(lat_x100 % 100);
        uint32_t ns = (uint32_t)(lat_x100 / 5);

        uprintf("%9lu  %7lu     %4u.%02u    %7u\r\n",
                (unsigned long)bytes, (unsigned long)n, lw, lf, ns);
    }

    uprintf("-------------------------------------------------------------\r\n");
    uprintf("Done. (sink=%08x)\r\n", (unsigned)sink);

    while (1)
        ;
    return 0;
}
