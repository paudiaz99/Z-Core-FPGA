/*
 * SDRAM instruction fetch test for Z-Core.
 *
 * Tests progressively harder scenarios:
 *  1. Simple sequential prints (few cache lines)
 *  2. Loads interleaved with prints (triggers fetch/data overlap)
 *  3. Function calls (cache pressure from distant code)
 *  4. Loop with SDRAM data access (sustained fetch + load)
 *
 * Expected output is deterministic — any corruption is visible.
 */

#define UART_BASE       0x04000000
#define UART_TX         (*(volatile unsigned int *)(UART_BASE + 0x00))
#define UART_STAT       (*(volatile unsigned int *)(UART_BASE + 0x08))
#define UART_STAT_TX_EMPTY 0x01

static void __attribute__((noinline)) putc_uart(char c)
{
    while (!(UART_STAT & UART_STAT_TX_EMPTY))
        ;
    UART_TX = (unsigned int)c;
    while (!(UART_STAT & UART_STAT_TX_EMPTY))
        ;
}

static void __attribute__((noinline)) puts_uart(const char *s)
{
    while (*s)
        putc_uart(*s++);
}

static void __attribute__((noinline)) puthex(unsigned int val)
{
    static const char hex[] = "0123456789ABCDEF";
    puts_uart("0x");
    for (int i = 28; i >= 0; i -= 4)
        putc_uart(hex[(val >> i) & 0xF]);
}

/* ---- Test 1: Sequential string prints ---- */
static void __attribute__((noinline)) test1_sequential(void)
{
    puts_uart("[T1] ABCDEFGHIJKLMNOP\n");
    puts_uart("[T1] 0123456789\n");
    puts_uart("[T1] The quick brown fox\n");
}

/* ---- Test 2: Loads interleaved with prints ---- */
/* This is the critical test: loads from SDRAM data while
   executing from SDRAM .text — triggers the fetch/data race. */
static volatile unsigned int test_data[8] = {
    0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0x9ABCDEF0,
    0x11111111, 0x22222222, 0x33333333, 0x44444444
};

static void __attribute__((noinline)) test2_load_interleave(void)
{
    puts_uart("[T2] Load test: ");
    for (int i = 0; i < 8; i++) {
        unsigned int val = test_data[i];
        puthex(val);
        putc_uart(' ');
    }
    putc_uart('\n');
}

/* ---- Test 3: Function calls (distant code, cache pressure) ---- */
static int __attribute__((noinline)) add_func(int a, int b) { return a + b; }
static int __attribute__((noinline)) mul_func(int a, int b) { return a * b; }
static int __attribute__((noinline)) sub_func(int a, int b) { return a - b; }

static void __attribute__((noinline)) test3_calls(void)
{
    int r1 = add_func(100, 200);    /* expect 300  */
    int r2 = mul_func(12, 25);      /* expect 300  */
    int r3 = sub_func(1000, 700);   /* expect 300  */
    puts_uart("[T3] 100+200=");
    puthex((unsigned int)r1);
    puts_uart(" 12*25=");
    puthex((unsigned int)r2);
    puts_uart(" 1000-700=");
    puthex((unsigned int)r3);
    if (r1 == 300 && r2 == 300 && r3 == 300)
        puts_uart(" OK\n");
    else
        puts_uart(" FAIL\n");
}

/* ---- Test 4: Sustained SDRAM data load in loop ---- */
static void __attribute__((noinline)) test4_sdram_load(void)
{
    /* Read from our own .text as data — we know it's in SDRAM */
    volatile unsigned int *code = (volatile unsigned int *)0x10000000;
    unsigned int sum = 0;
    for (int i = 0; i < 64; i++)
        sum += code[i];
    puts_uart("[T4] sum64=");
    puthex(sum);
    putc_uart('\n');
}

/* ---- Test 5: Repeated pattern to detect intermittent errors ---- */
static void __attribute__((noinline)) test5_repeat(void)
{
    for (int round = 0; round < 5; round++) {
        puts_uart("[T5] round ");
        putc_uart('0' + round);
        puts_uart(": ABCDEFGHIJKLMNOPQRSTUVWXYZ\n");
        /* Do a load between rounds to trigger potential fetch/data race */
        volatile unsigned int dummy = test_data[round % 8];
        (void)dummy;
    }
}

void main(void)
{
    puts_uart("\n=== SDRAM Fetch Test ===\n");
    test1_sequential();
    test2_load_interleave();
    test3_calls();
    test4_sdram_load();
    test5_repeat();
    puts_uart("=== ALL DONE ===\n");
    while (1)
        ;
}
