#include "libs/uart.h"

/* SDRAM base address (M5 on the interconnect) */
#define SDRAM_BASE 0x10000000

/* Simple LFSR for pseudorandom patterns */
static unsigned int lfsr = 0xACE1u;
static unsigned int lfsr_next(void) {
    unsigned int bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1u;
    lfsr = (lfsr >> 1) | (bit << 15);
    return lfsr;
}

/* Write and read back a single 32-bit word; return 1 on mismatch */
static int test_word(volatile unsigned int *addr, unsigned int pattern) {
    *addr = pattern;
    unsigned int readback = *addr;
    if (readback != pattern) {
        uart_puts("FAIL @ 0x");
        uart_puthex((unsigned int)addr);
        uart_puts(": wrote 0x");
        uart_puthex(pattern);
        uart_puts(", read 0x");
        uart_puthex(readback);
        uart_puts("\r\n");
        return 1;
    }
    return 0;
}

void main(void) {
    uart_set_baud(27); /* 115200 baud */

    uart_puts("\r\n========================================\r\n");
    uart_puts("       Z-Core SDRAM Test\r\n");
    uart_puts("========================================\r\n");

    volatile unsigned int *sdram = (volatile unsigned int *)SDRAM_BASE;
    int errors = 0;

    /* ---- Test 1: Sequential pattern ---- */
    uart_puts("\r\n[1] Sequential pattern (256 words)...\r\n");
    for (int i = 0; i < 256; i++)
        sdram[i] = (unsigned int)i;

    for (int i = 0; i < 256; i++) {
        unsigned int v = sdram[i];
        if (v != (unsigned int)i) {
            uart_puts("  SEQ FAIL @[");
            uart_putint(i);
            uart_puts("]: exp 0x");
            uart_puthex((unsigned int)i);
            uart_puts(", got 0x");
            uart_puthex(v);
            uart_puts("\r\n");
            errors++;
            if (errors > 16) { uart_puts("  ...too many errors\r\n"); break; }
        }
    }
    if (errors == 0) uart_puts("  PASS\r\n");

    /* ---- Test 2: Walking ones ---- */
    uart_puts("[2] Walking ones (32 words)...\r\n");
    int e2 = 0;
    for (int bit = 0; bit < 32; bit++) {
        unsigned int pat = 1u << bit;
        e2 += test_word(&sdram[0x1000 + bit], pat);
    }
    if (e2 == 0) uart_puts("  PASS\r\n");
    errors += e2;

    /* ---- Test 3: Scattered random addresses ---- */
    uart_puts("[3] Random patterns at scattered addresses (128 writes)...\r\n");
    int e3 = 0;
    lfsr = 0xACE1u;
    /* Write phase */
    for (int i = 0; i < 128; i++) {
        unsigned int offset = (lfsr_next() << 16) | lfsr_next();
        offset = (offset & 0x003FFFFFu); /* keep within 64 MB / 4 = 16M words */
        unsigned int pat = (lfsr_next() << 16) | lfsr_next();
        sdram[offset] = pat;
    }
    /* Read-back phase (re-seed LFSR for same sequence) */
    lfsr = 0xACE1u;
    for (int i = 0; i < 128; i++) {
        unsigned int offset = (lfsr_next() << 16) | lfsr_next();
        offset = (offset & 0x003FFFFFu);
        unsigned int pat = (lfsr_next() << 16) | lfsr_next();
        unsigned int v = sdram[offset];
        if (v != pat) {
            uart_puts("  RAND FAIL @[0x");
            uart_puthex(offset);
            uart_puts("]: exp 0x");
            uart_puthex(pat);
            uart_puts(", got 0x");
            uart_puthex(v);
            uart_puts("\r\n");
            e3++;
            if (e3 > 16) { uart_puts("  ...too many errors\r\n"); break; }
        }
    }
    if (e3 == 0) uart_puts("  PASS\r\n");
    errors += e3;

    /* ---- Test 4: Byte-strobe partial write ---- */
    uart_puts("[4] Byte-strobe partial write...\r\n");
    int e4 = 0;
    sdram[0x2000] = 0xDEADBEEF;
    /* Write only byte 1 (bits [15:8]) — this tests wstrb != 0xF in the bridge */
    volatile unsigned char *byte_ptr = (volatile unsigned char *)&sdram[0x2000];
    byte_ptr[1] = 0x42;
    unsigned int result = sdram[0x2000];
    if (result != 0xDEAD42EF) {
        uart_puts("  BYTE FAIL: exp 0xDEAD42EF, got 0x");
        uart_puthex(result);
        uart_puts("\r\n");
        e4 = 1;
    }
    if (e4 == 0) uart_puts("  PASS\r\n");
    errors += e4;

    /* ---- Summary ---- */
    uart_puts("\r\n========================================\r\n");
    if (errors == 0) {
        uart_puts("ALL TESTS PASSED\r\n");
    } else {
        uart_puts("TESTS FAILED: ");
        uart_putint(errors);
        uart_puts(" error(s)\r\n");
    }
    uart_puts("========================================\r\n");

    while (1); /* halt */
}
