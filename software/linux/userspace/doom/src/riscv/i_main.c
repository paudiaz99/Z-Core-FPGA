#include <stdint.h>
#include <string.h>

#include "doomdef.h"
#include "d_main.h"

#include "config.h"
#include "console.h"



static int pf_errors;

static void pf_pass(const char *name)
{
	console_printf("  [OK]   %s\n", name);
}

static void pf_fail(const char *name, const char *detail)
{
	console_printf("  [FAIL] %s: %s\n", name, detail);
	pf_errors++;
}


static void pf_uart(void)
{
	uint32_t stat = UART_STAT;
	if (stat == 0xFFFFFFFF || stat == 0xDEADDEAD) {
		pf_fail("UART", "STATUS reads bus error");
		return;
	}
	if (!(stat & UART_STAT_TX_EMPTY)) {
		pf_fail("UART", "TX not empty after init");
		return;
	}
	pf_pass("UART");
}


static void pf_timer(void)
{
	TIMER_CTRL_REG = 0x03;

	uint32_t t0 = TIMER_LO;

	for (volatile int i = 0; i < 200; i++)
		;

	uint32_t t1 = TIMER_LO;

	if (t1 == t0) {
		pf_fail("Timer", "counter did not increment");
		console_printf("         TIMER_LO = 0x%08x (both reads)\n", t0);
		return;
	}
	if (t0 == 0xFFFFFFFF || t1 == 0xFFFFFFFF) {
		pf_fail("Timer", "reads bus error (0xFFFFFFFF)");
		return;
	}

	uint32_t delta = t1 - t0;
	console_printf("  [OK]   Timer (delta=%u ticks)\n", delta);
}


static void pf_vga(void)
{
	uint32_t status = VGA_FB_STATUS;
	if (status == 0xFFFFFFFF) {
		pf_fail("VGA", "STATUS reads bus error");
		return;
	}

	VGA_FB_ADDR = 42;
	uint32_t readback = VGA_FB_ADDR;
	if (readback != 42) {
		console_printf("  [WARN] VGA FB_ADDR: wrote 42, read %u\n", readback);
	}

	pf_pass("VGA");
}


static void pf_gpio(void)
{
	volatile uint32_t *gpio_dir  = (volatile uint32_t *)(GPIO_BASE + 0x04);
	uint32_t val = *gpio_dir;
	if (val == 0xFFFFFFFF && val == 0xDEADDEAD) {
		pf_fail("GPIO", "direction reg reads bus error");
		return;
	}
	pf_pass("GPIO");
}


static void pf_sdram(void)
{
	volatile uint32_t *code_base = (volatile uint32_t *)SDRAM_BASE;
	uint32_t first_word = code_base[0];

	if (first_word == 0x00000000) {
		pf_fail("SDRAM", "first word is 0 (code not loaded?)");
		return;
	}
	if (first_word == 0xFFFFFFFF) {
		pf_fail("SDRAM", "first word is 0xFFFFFFFF (bus error?)");
		return;
	}

	uint32_t mid_word = code_base[0x1000];
	if (mid_word == 0x00000000 && code_base[0x1001] == 0x00000000) {
		pf_fail("SDRAM", "mid-region reads all zeros");
		return;
	}

	console_printf("  [OK]   SDRAM (code[0]=0x%08x)\n", first_word);
}



#define WAD_EXPECTED_MAGIC      0x44415749
#define WAD_EXPECTED_NUMLUMPS   1264
#define WAD_EXPECTED_DIROFS     0x003FB7B4
#define WAD_EXPECTED_BYTESUM    0x1683486D

static void pf_wad(void)
{
	volatile uint32_t *wad = (volatile uint32_t *)WAD_BASE;

	uint32_t magic = wad[0];
	if (magic != WAD_EXPECTED_MAGIC) {
		pf_fail("WAD", "bad magic");
		console_printf("         got 0x%08x, expected 0x%08x\n",
			magic, WAD_EXPECTED_MAGIC);
		return;
	}

	uint32_t numLumps = wad[1];
	if (numLumps != WAD_EXPECTED_NUMLUMPS)
		console_printf("  [WARN] WAD numLumps=%u (expected %u)\n",
			numLumps, WAD_EXPECTED_NUMLUMPS);

	console_printf("  [OK]   WAD (%u lumps)\n", numLumps);
}



static volatile uint32_t g_canary = 0xDEADBEEF;

static void test_global_consistency(void)
{
	uint32_t errs = 0;
	const uint32_t N = 1000000;
	for (uint32_t i = 0; i < N; i++) {
		uint32_t a = g_canary;
		uint32_t b = g_canary;
		uint32_t c = g_canary;
		if (a != 0xDEADBEEF || b != 0xDEADBEEF || c != 0xDEADBEEF) {
			if (errs < 8)
				console_printf("[GLB] i=%u a=%08x b=%08x c=%08x\r\n",
					i, a, b, c);
			errs++;
		}
	}
	console_printf("[GLB] errs=%u/%u\r\n", errs, N);
}

static void test_stack_consistency(void)
{
	volatile uint32_t buf[16];
	uint32_t errs = 0;
	const uint32_t N = 1000000;
	for (uint32_t i = 0; i < N; i++) {
		uint32_t ref = 0xCAFE0000u | (i & 0xFFFFu);
		buf[3] = ref;
		uint32_t a = buf[3];
		uint32_t b = buf[3];
		uint32_t c = buf[3];
		if (a != b || b != c || a != ref) {
			if (errs < 8)
				console_printf("[STK] i=%u a=%08x b=%08x c=%08x ref=%08x\r\n",
					i, a, b, c, ref);
			errs++;
		}
	}
	console_printf("[STK] errs=%u/%u\r\n", errs, N);
}

static void test_bram_consistency(void)
{
	volatile uint32_t *p = (volatile uint32_t *)0x00000FF0u;
	uint32_t errs = 0;
	const uint32_t N = 1000000;
	for (uint32_t i = 0; i < N; i++) {
		uint32_t ref = 0xBEEF0000u | (i & 0xFFFFu);
		*p = ref;
		uint32_t a = *p;
		uint32_t b = *p;
		uint32_t c = *p;
		if (a != b || b != c || a != ref) {
			if (errs < 8)
				console_printf("[BRM] i=%u a=%08x b=%08x c=%08x ref=%08x\r\n",
					i, a, b, c, ref);
			errs++;
		}
	}
	console_printf("[BRM] errs=%u/%u\r\n", errs, N);
}

static void test_b2b_sdram(void)
{
	volatile uint32_t buf;
	uint32_t errs = 0;
	const uint32_t N = 1000000;
	for (uint32_t i = 0; i < N; i++) {
		uint32_t ref = 0xA5A50000u | (i & 0xFFFFu);
		buf = ref;
		uint32_t a, b, c;
		asm volatile (
			"lw %0, 0(%3)\n\t"
			"lw %1, 0(%3)\n\t"
			"lw %2, 0(%3)\n\t"
			: "=&r"(a), "=&r"(b), "=&r"(c)
			: "r"(&buf)
			: "memory"
		);
		if (a != b || b != c || a != ref) {
			if (errs < 8)
				console_printf("[B2B] i=%u a=%08x b=%08x c=%08x ref=%08x\r\n",
					i, a, b, c, ref);
			errs++;
		}
	}
	console_printf("[B2B] errs=%u/%u\r\n", errs, N);
}

static void test_b2b_bram(void)
{
	volatile uint32_t *p = (volatile uint32_t *)0x00000FF0u;
	uint32_t errs = 0;
	const uint32_t N = 1000000;
	for (uint32_t i = 0; i < N; i++) {
		uint32_t ref = 0x5A5A0000u | (i & 0xFFFFu);
		*p = ref;
		uint32_t a, b, c;
		asm volatile (
			"lw %0, 0(%3)\n\t"
			"lw %1, 0(%3)\n\t"
			"lw %2, 0(%3)\n\t"
			: "=&r"(a), "=&r"(b), "=&r"(c)
			: "r"(p)
			: "memory"
		);
		if (a != b || b != c || a != ref) {
			if (errs < 8)
				console_printf("[B2B-BR] i=%u a=%08x b=%08x c=%08x ref=%08x\r\n",
					i, a, b, c, ref);
			errs++;
		}
	}
	console_printf("[B2B-BR] errs=%u/%u\r\n", errs, N);
}

static void test_diversify_bank(void)
{
	volatile uint32_t buf;
	uint32_t errs = 0;
	const uint32_t N = 500000;
	for (uint32_t i = 0; i < N; i++) {
		uint32_t ref = 0xC0DE0000u | (i & 0xFFFFu);
		buf = ref;
		uint32_t a;
		asm volatile (
			"lui  t0, 0x10000\n\t"
			"lw   zero, 0(t0)\n\t"
			"lui  t0, 0x10800\n\t"
			"lw   zero, 0(t0)\n\t"
			"lui  t0, 0x11000\n\t"
			"lw   zero, 0(t0)\n\t"
			"lui  t0, 0x11800\n\t"
			"lw   zero, 0(t0)\n\t"
			"lw   %0, 0(%1)\n\t"
			: "=r"(a)
			: "r"(&buf)
			: "t0", "memory"
		);
		if (a != ref) {
			if (errs < 8)
				console_printf("[DIV] i=%u a=%08x ref=%08x\r\n",
					i, a, ref);
			errs++;
		}
	}
	console_printf("[DIV] errs=%u/%u\r\n", errs, N);
}

static void test_write_storm(void)
{
	volatile uint32_t buf[32];
	uint32_t errs = 0;
	const uint32_t N = 500000;
	for (uint32_t i = 0; i < N; i++) {
		uint32_t ref = 0xDA7A0000u | (i & 0xFFFFu);
		buf[0] = ref;
		uint32_t a;
		asm volatile (
			"sw   zero,  4(%1)\n\t"
			"sw   zero,  8(%1)\n\t"
			"sw   zero, 12(%1)\n\t"
			"sw   zero, 16(%1)\n\t"
			"sw   zero, 20(%1)\n\t"
			"sw   zero, 24(%1)\n\t"
			"sw   zero, 28(%1)\n\t"
			"sw   zero, 32(%1)\n\t"
			"sw   zero, 36(%1)\n\t"
			"sw   zero, 40(%1)\n\t"
			"sw   zero, 44(%1)\n\t"
			"sw   zero, 48(%1)\n\t"
			"sw   zero, 52(%1)\n\t"
			"sw   zero, 56(%1)\n\t"
			"sw   zero, 60(%1)\n\t"
			"sw   zero, 64(%1)\n\t"
			"lw   %0,    0(%1)\n\t"
			: "=r"(a)
			: "r"(&buf[0])
			: "memory"
		);
		if (a != ref) {
			if (errs < 8)
				console_printf("[WST] i=%u a=%08x ref=%08x\r\n",
					i, a, ref);
			errs++;
		}
	}
	console_printf("[WST] errs=%u/%u\r\n", errs, N);
}


#define THRASH_FN(n)                                                    \
__attribute__((noinline,aligned(1024),used,                             \
               section(".text.thrash" #n)))                             \
static void thrash##n(void) { asm volatile (""); }

THRASH_FN(0)  THRASH_FN(1)  THRASH_FN(2)  THRASH_FN(3)
THRASH_FN(4)  THRASH_FN(5)  THRASH_FN(6)  THRASH_FN(7)
THRASH_FN(8)  THRASH_FN(9)  THRASH_FN(10) THRASH_FN(11)
THRASH_FN(12) THRASH_FN(13) THRASH_FN(14) THRASH_FN(15)

static void (*const thrash_funcs[16])(void) = {
	thrash0,  thrash1,  thrash2,  thrash3,
	thrash4,  thrash5,  thrash6,  thrash7,
	thrash8,  thrash9,  thrash10, thrash11,
	thrash12, thrash13, thrash14, thrash15
};

static void test_icache_thrash(void)
{
	volatile uint32_t buf;
	uint32_t errs = 0;
	const uint32_t N = 200000;
	for (uint32_t i = 0; i < N; i++) {
		uint32_t ref = 0x1CA70000u | (i & 0xFFFFu);
		buf = ref;
		for (int j = 0; j < 16; j++)
			thrash_funcs[j]();
		uint32_t a = buf;
		if (a != ref) {
			if (errs < 8)
				console_printf("[ITH] i=%u a=%08x ref=%08x &buf=%08x\r\n",
					i, a, ref, (unsigned)(uintptr_t)&buf);
			errs++;
		}
	}
	console_printf("[ITH] errs=%u/%u\r\n", errs, N);
}

static void test_icache_plus_bank(void)
{
	volatile uint32_t buf;
	uint32_t errs = 0;
	const uint32_t N = 100000;
	for (uint32_t i = 0; i < N; i++) {
		uint32_t ref = 0xCABA0000u | (i & 0xFFFFu);
		buf = ref;
		asm volatile (
			"lui  t0, 0x10000\n\t" "lw zero, 0(t0)\n\t"
			"lui  t0, 0x10800\n\t" "lw zero, 0(t0)\n\t"
			"lui  t0, 0x11000\n\t" "lw zero, 0(t0)\n\t"
			"lui  t0, 0x11800\n\t" "lw zero, 0(t0)\n\t"
			::: "t0", "memory"
		);
		for (int j = 0; j < 8; j++)
			thrash_funcs[j]();
		uint32_t a = buf;
		if (a != ref) {
			if (errs < 8)
				console_printf("[IPB] i=%u a=%08x ref=%08x\r\n",
					i, a, ref);
			errs++;
		}
	}
	console_printf("[IPB] errs=%u/%u\r\n", errs, N);
}

static void test_sdram_full(void)
{
	volatile uint32_t * const base = (volatile uint32_t *)0x12500000u;
	const uint32_t words = (0x13F00000u - 0x12500000u) / 4u;

	uint32_t errs = 0;
	uint32_t reported = 0;
	uint32_t bad_bits_or = 0;

	for (uint32_t i = 0; i < words; i++)
		base[i] = (uint32_t)(uintptr_t)&base[i];
	for (uint32_t i = 0; i < words; i++) {
		uint32_t exp = (uint32_t)(uintptr_t)&base[i];
		uint32_t got = base[i];
		if (got != exp) {
			errs++;
			bad_bits_or |= (got ^ exp);
			if (reported < 32) {
				console_printf("[SDR-A] @%08x got=%08x exp=%08x diff=%08x\r\n",
					(uint32_t)&base[i], got, exp, got ^ exp);
				reported++;
			}
		}
	}

	for (uint32_t i = 0; i < words; i++)
		base[i] = 0xAAAAAAAAu;
	for (uint32_t i = 0; i < words; i++) {
		uint32_t got = base[i];
		if (got != 0xAAAAAAAAu) {
			errs++;
			bad_bits_or |= (got ^ 0xAAAAAAAAu);
			if (reported < 32) {
				console_printf("[SDR-AA] @%08x got=%08x diff=%08x\r\n",
					(uint32_t)&base[i], got, got ^ 0xAAAAAAAAu);
				reported++;
			}
		}
	}

	for (uint32_t i = 0; i < words; i++)
		base[i] = 0x55555555u;
	for (uint32_t i = 0; i < words; i++) {
		uint32_t got = base[i];
		if (got != 0x55555555u) {
			errs++;
			bad_bits_or |= (got ^ 0x55555555u);
			if (reported < 32) {
				console_printf("[SDR-55] @%08x got=%08x diff=%08x\r\n",
					(uint32_t)&base[i], got, got ^ 0x55555555u);
				reported++;
			}
		}
	}

	console_printf("[SDR] full-walk: words=%u patterns=3 errs=%u bad_bits_OR=%08x\r\n",
		words, errs, bad_bits_or);
}

static void run_consistency_probes(void)
{
	console_puts("[PROBE] global read consistency (SDRAM .data)...\r\n");
	test_global_consistency();
	console_puts("[PROBE] stack write/read consistency (SDRAM stack)...\r\n");
	test_stack_consistency();
	console_puts("[PROBE] BRAM write/read consistency (1-cycle slave)...\r\n");
	test_bram_consistency();
	console_puts("[PROBE] back-to-back lw SDRAM (forced asm)...\r\n");
	test_b2b_sdram();
	console_puts("[PROBE] back-to-back lw BRAM (forced asm)...\r\n");
	test_b2b_bram();
	console_puts("[PROBE] diversify-bank (write/cross-bank reads/re-read)...\r\n");
	test_diversify_bank();
	console_puts("[PROBE] write storm (many stack writes then re-read)...\r\n");
	test_write_storm();
	console_puts("[PROBE] I-cache thrash (16 aliased stubs)...\r\n");
	test_icache_thrash();
	console_puts("[PROBE] I-cache thrash + bank diversify...\r\n");
	test_icache_plus_bank();
	console_puts("[PROBE] SDRAM full walk (3 patterns, ~26 MB)...\r\n");
	console_puts("[PROBE] SDRAM full walk skipped\r\n");
}



static void preflight(void)
{
	pf_errors = 0;

	console_puts("\n--- Z-Core DOOM Pre-flight ---\n");
	console_printf("  Code : 0x%08x\n", (uint32_t)SDRAM_BASE);
	console_printf("  Stack: 0x%08x\n", (uint32_t)0x10F00000);
	console_printf("  WAD  : 0x%08x (%u bytes)\n", (uint32_t)WAD_BASE, (uint32_t)WAD_SIZE);
	console_puts("  Checking peripherals...\n");

	pf_uart();
	pf_timer();
	pf_vga();
	pf_gpio();
	pf_sdram();
	pf_wad();

	if (pf_errors == 0)
		console_puts("  All checks passed.\n");
	else
		console_printf("  *** %d check(s) FAILED ***\n", pf_errors);

	console_printf("[UART] SDRAM code @ 0x10000000, WAD @ 0x%08x\r\n",
	               (uint32_t)WAD_BASE);
	console_puts("--- Starting DOOM ---\n\n");
}


int main(void)
{
	console_init();
	console_puts("[UART] console_init OK\r\n");

	preflight();

	run_consistency_probes();

	console_puts("[UART] preflight done, entering D_DoomMain\r\n");
	D_DoomMain();
	return 0;
}
