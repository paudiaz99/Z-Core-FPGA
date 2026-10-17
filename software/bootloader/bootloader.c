#include "uart.h"

#define SYNC_REQ       0x5A
#define SYNC_ACK       0xA5
#define ACK            0x06
#define NAK            0x15

#define BAUD_DIV_115200 27

#define MAX_SEGMENTS    8

#define SDRAM_BASE     0x10000000

void trap_report(unsigned int mcause, unsigned int mepc, unsigned int mtval) {
    uart_puts("\r\nTRAP c=");
    uart_puthex(mcause);
    uart_puts(" epc=");
    uart_puthex(mepc);
    uart_puts(" tv=");
    uart_puthex(mtval);
    uart_puts("\r\n");
    while (!(UART_STAT & UART_STAT_TX_EMPTY))
        ;
    for (;;)
        ;
}

static unsigned int recv_le32(void) {
    unsigned int v = 0;
    v |= ((unsigned int)(unsigned char)uart_getc_blocking());
    v |= ((unsigned int)(unsigned char)uart_getc_blocking()) << 8;
    v |= ((unsigned int)(unsigned char)uart_getc_blocking()) << 16;
    v |= ((unsigned int)(unsigned char)uart_getc_blocking()) << 24;
    return v;
}

static void print_banner(void) {
    uart_puts("\r\nZ-Core Bootloader v3.2 (SDRAM)\r\n"
              " BRAM 16K@0  SDRAM 64M@0x10000000  UART 115200 8N1\r\n");
}

static void dcache_flush(void) {
    volatile unsigned int * const scratch =
        (volatile unsigned int *)0x10100000;
    for (unsigned int i = 0; i < (64u * 1024u / 4u); i++) {
        (void)scratch[i];
    }
}

static void write_buffer_drain(void) {
    __asm__ volatile ("fence rw, rw" ::: "memory");
}

static int sdram_bist(void) {
    volatile unsigned int *sdram = (volatile unsigned int *)SDRAM_BASE;
    int errors = 0;

    static const unsigned int offsets[] = {
        0,
        17,
        0x100,
        0x1000,
        0x10000,
        0x20000,
        0x3DEAD,
        0x3FFFF,
    };
    static const int n_offsets = sizeof(offsets) / sizeof(offsets[0]);

    uart_puts("BIST: ");

    for (int i = 0; i < n_offsets; i++)
        sdram[offsets[i]] = offsets[i] ^ 0xA5A5A5A5;


    for (int bit = 0; bit < 32; bit++) {
        unsigned int pattern = 1u << bit;
        sdram[0] = pattern;
        unsigned int got = sdram[0];
        if (got != pattern) {
            errors++;
            uart_puts("FAIL bit");
            uart_putint(bit);
            uart_puts(" exp=");
            uart_puthex(pattern);
            uart_puts(" got=");
            uart_puthex(got);
            uart_puts("\r\n       ");
        }
    }

    {
        volatile unsigned int *word = &sdram[0x2000];
        volatile unsigned char *bytes = (volatile unsigned char *)word;

        *word = 0xDEADBEEF;
        unsigned int rb = *word;
        if (rb != 0xDEADBEEF) {
            errors++;
            uart_puts("FAIL byte-setup w=DEADBEEF got=");
            uart_puthex(rb);
            uart_puts("\r\n       ");
        }

        bytes[0] = 0x42;
        rb = *word;
        if (rb != 0xDEADBE42u) {
            errors++;
            uart_puts("FAIL byte[0] exp=DEADBE42 got=");
            uart_puthex(rb);
            uart_puts("\r\n       ");
        }

        bytes[1] = 0x55;
        rb = *word;
        if (rb != 0xDEAD5542u) {
            errors++;
            uart_puts("FAIL byte[1] exp=DEAD5542 got=");
            uart_puthex(rb);
            uart_puts("\r\n       ");
        }

        bytes[2] = 0xAA;
        rb = *word;
        if (rb != 0xDEAA5542u) {
            errors++;
            uart_puts("FAIL byte[2] exp=DEAA5542 got=");
            uart_puthex(rb);
            uart_puts("\r\n       ");
        }

        bytes[3] = 0x77;
        rb = *word;
        if (rb != 0x77AA5542u) {
            errors++;
            uart_puts("FAIL byte[3] exp=77AA5542 got=");
            uart_puthex(rb);
            uart_puts("\r\n       ");
        }

        *word = 0x04030201;
        for (int b = 0; b < 4; b++) {
            unsigned char got_b = bytes[b];
            unsigned char exp_b = (unsigned char)(b + 1);
            if (got_b != exp_b) {
                errors++;
                uart_puts("FAIL lb[");
                uart_putint(b);
                uart_puts("] exp=");
                uart_puthex(exp_b);
                uart_puts(" got=");
                uart_puthex(got_b);
                uart_puts("\r\n       ");
            }
        }
    }

    {
        volatile unsigned int *word = &sdram[0x2001];
        volatile unsigned short *halfs = (volatile unsigned short *)word;

        *word = 0xCAFEBABE;

        unsigned short h0 = halfs[0];
        unsigned short h1 = halfs[1];
        if (h0 != 0xBABEu) {
            errors++;
            uart_puts("FAIL lh[0] exp=BABE got=");
            uart_puthex(h0);
            uart_puts("\r\n       ");
        }
        if (h1 != 0xCAFEu) {
            errors++;
            uart_puts("FAIL lh[1] exp=CAFE got=");
            uart_puthex(h1);
            uart_puts("\r\n       ");
        }

        halfs[0] = 0x1234;
        unsigned int rb = *word;
        if (rb != 0xCAFE1234u) {
            errors++;
            uart_puts("FAIL sh[0] exp=CAFE1234 got=");
            uart_puthex(rb);
            uart_puts("\r\n       ");
        }

        halfs[1] = 0x5678;
        rb = *word;
        if (rb != 0x56781234u) {
            errors++;
            uart_puts("FAIL sh[1] exp=56781234 got=");
            uart_puthex(rb);
            uart_puts("\r\n       ");
        }
    }

    {
        volatile unsigned int *base = &sdram[0x3000];

        static const unsigned int patterns[] = {
            0x00000000,
            0xFF00FF00,
            0x00FF00FF,
            0x00004142,
            0x00004E57,
            0x31574F52,
            0x00000042,
            0x42000000,
        };
        int np = sizeof(patterns) / sizeof(patterns[0]);

        for (int i = 0; i < np; i++) {
            base[i] = patterns[i];
        }
        for (int i = 0; i < np; i++) {
            unsigned int got = base[i];
            if (got != patterns[i]) {
                errors++;
                uart_puts("FAIL nul[");
                uart_putint(i);
                uart_puts("] exp=");
                uart_puthex(patterns[i]);
                uart_puts(" got=");
                uart_puthex(got);
                uart_puts("\r\n       ");
            }
        }
    }

    {
        volatile unsigned int *addr = &sdram[0x4000];
        *addr = 0x12003400;

        int consist_errs = 0;
        for (int r = 0; r < 16; r++) {
            unsigned int r1 = *addr;
            unsigned int r2 = *addr;
            if (r1 != r2) {
                consist_errs++;
                if (consist_errs <= 3) {
                    uart_puts("FAIL consist r1=");
                    uart_puthex(r1);
                    uart_puts(" r2=");
                    uart_puthex(r2);
                    uart_puts("\r\n       ");
                }
            }
        }
        errors += consist_errs;
    }

    {
        #define CHASE_N   8
        static const unsigned int node_off[CHASE_N] = {
            0x5000, 0x5400, 0x5010, 0x5804,
            0x5040, 0x5C08, 0x5020, 0x5410,
        };

        for (int i = 0; i < CHASE_N; i++) {
            volatile unsigned int *n = &sdram[node_off[i]];
            int ni = (i + 1) % CHASE_N;
            n[0] = (unsigned int)&sdram[node_off[ni]];
            n[2] = (unsigned int)i;
        }

        int chase_errs = 0;
        volatile unsigned int *cur =
            (volatile unsigned int *)&sdram[node_off[0]];
        for (int hop = 0; hop < CHASE_N; hop++) {
            unsigned int tag = cur[2];
            unsigned int nxt = cur[0];
            unsigned int nx2 = cur[0];

            int bad = (tag != (unsigned int)hop) ||
                      (nxt != nx2) ||
                      (nxt < 0x10000000u) ||
                      (nxt >= 0x14000000u) ||
                      (nxt & 3u);
            if (bad) {
                chase_errs++;
                uart_puts("FAIL chase ");
                uart_puthex(hop);
                uart_putc(' ');
                uart_puthex(tag);
                uart_putc(' ');
                uart_puthex(nxt);
                uart_putc(' ');
                uart_puthex(nx2);
                uart_puts("\r\n       ");
                if (nxt < 0x10000000u || nxt >= 0x14000000u) break;
            }
            cur = (volatile unsigned int *)nxt;
        }
        errors += chase_errs;
    }

    if (errors == 0)
        uart_puts("PASS (all + chase)\r\n");
    else {
        uart_putint(errors);
        uart_puts(" error(s)\r\n");
    }

    return errors;
}

void main(void) {
    uart_set_baud(BAUD_DIV_115200);
    print_banner();

    sdram_bist();

    uart_puts("Waiting for upload...\r\n");

    while ((unsigned char)uart_getc_blocking() != SYNC_REQ)
        ;
    uart_putc((char)SYNC_ACK);

    unsigned int new_baud_div = recv_le32();
    if (new_baud_div != 0) {
        uart_putc((char)ACK);
        while (!(UART_STAT & UART_STAT_TX_EMPTY))
            ;
        for (volatile int i = 0; i < 5000; i++)
            ;
        uart_set_baud(new_baud_div);
        for (volatile int i = 0; i < 5000; i++)
            ;
        uart_putc((char)ACK);
    } else {
        uart_putc((char)ACK);
    }

    unsigned int seg_count = (unsigned int)(unsigned char)uart_getc_blocking();
    if (seg_count == 0 || seg_count > MAX_SEGMENTS) {
        uart_putc((char)NAK);
        uart_puts("ERR: bad seg count\r\n");
        while (1)
            ;
    }
    uart_putc((char)ACK);
    uart_puts("Segments: ");
    uart_putint((int)seg_count);
    uart_puts("\r\n");

    for (unsigned int s = 0; s < seg_count; s++) {
        unsigned int base = recv_le32();
        unsigned int size = recv_le32();

        if (size == 0) {
            uart_putc((char)NAK);
            uart_puts("ERR: seg size 0\r\n");
            while (1)
                ;
        }

        uart_putc((char)ACK);
        uart_puts("[");
        uart_putint((int)s);
        uart_puts("] ");
        uart_puthex(base);
        uart_puts(" ");
        uart_putint((int)size);
        uart_puts(" B\r\n");

        volatile unsigned int *dest32 = (volatile unsigned int *)base;
        unsigned int checksum = 0;
        unsigned int full_words = size / 4;
        unsigned int remainder = size % 4;

        for (unsigned int w = 0; w < full_words; w++) {
            unsigned int word = 0;
            for (int j = 0; j < 4; j++) {
                unsigned char b = (unsigned char)uart_getc_blocking();
                checksum += b;
                word |= ((unsigned int)b << (j * 8));
            }
            dest32[w] = word;
        }

        if (remainder > 0) {
            unsigned char *tail = (unsigned char *)&dest32[full_words];
            for (unsigned int j = 0; j < remainder; j++) {
                unsigned char b = (unsigned char)uart_getc_blocking();
                checksum += b;
                tail[j] = b;
            }
        }

        unsigned int expected = recv_le32();
        if (checksum != expected) {
            uart_putc((char)NAK);
            uart_puts("ERR: cksum ");
            uart_puthex(checksum);
            uart_puts("!=");
            uart_puthex(expected);
            uart_puts("\r\n");
            while (1)
                ;
        }
        uart_putc((char)ACK);
    }

    unsigned int entry = recv_le32();
    uart_putc((char)ACK);
    uart_puts("Jump ");
    uart_puthex(entry);
    uart_puts("\r\n");

    volatile unsigned int *ep = (volatile unsigned int *)entry;
    uart_puts("Verify:");

    for (int i = 0; i < 4; i++) {
        uart_putc(' ');
        uart_puthex(ep[i]);
    }
    uart_putc('\n');

    volatile unsigned int *far_addr = (volatile unsigned int *)0x10010000;
    uart_puts("Far  :");
    for (int i = 0; i < 4; i++) {
        uart_putc(' ');
        uart_puthex(far_addr[i]);
    }
    uart_putc('\n');

    while (!(UART_STAT & UART_STAT_TX_EMPTY))
        ;

    dcache_flush();
    write_buffer_drain();

    uart_puts("VfyAft:");
    for (int i = 0; i < 4; i++) {
        uart_putc(' ');
        uart_puthex(ep[i]);
    }
    uart_putc('\n');
    while (!(UART_STAT & UART_STAT_TX_EMPTY))
        ;

    void (*app)(void) = (void (*)(void))entry;
    app();
}
