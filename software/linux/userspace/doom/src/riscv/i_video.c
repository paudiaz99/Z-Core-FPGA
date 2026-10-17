#include <stdint.h>
#include <string.h>

#include "doomdef.h"

#include "i_system.h"
#include "v_video.h"
#include "i_video.h"

#include "config.h"
#include "console.h"


static uint8_t rgb332_palette[256];

static uint32_t cycle_count = 0;
static uint32_t instret_count = 0;
static uint32_t cycle_count_prev = 0;
static uint32_t instret_count_prev = 0;
static uint32_t data_cache_hits = 0;
static uint32_t data_cache_hits_prev = 0;
static uint32_t load_requests = 0;
static uint32_t load_requests_prev = 0;
static uint32_t store_requests = 0;
static uint32_t store_requests_prev = 0;
static uint32_t uncached_requests = 0;
static uint32_t uncached_requests_prev = 0;
static uint32_t instruction_cache_misses = 0;
static uint32_t instruction_cache_misses_prev = 0;
static uint32_t instruction_cache_hits = 0;
static uint32_t instruction_cache_hits_prev = 0;
static uint32_t branch_misspredictions = 0;
static uint32_t branch_misspredictions_prev = 0;



void
I_InitGraphics(void)
{
	usegamma = 1;
}

void
I_ShutdownGraphics(void)
{
}


void
I_SetPalette(byte* palette)
{
	for (int i = 0; i < 256; i++) {
		uint8_t r = gammatable[usegamma][*palette++];
		uint8_t g = gammatable[usegamma][*palette++];
		uint8_t b = gammatable[usegamma][*palette++];
		rgb332_palette[i] = (r & 0xE0) | ((g >> 3) & 0x1C) | (b >> 6);
	}
}


void
I_UpdateNoBlit(void)
{
}

void
I_FinishUpdate(void)
{
	VGA_FB_ADDR = 0;

	for (int y = 0; y < VGA_HEIGHT; y++) {
		const byte *src_row = screens[0] + y * SCREENWIDTH;
		for (int x = 0; x < VGA_WIDTH; x++)
			VGA_FB_DATA = rgb332_palette[src_row[x]];
	}

	{
		static int frame_cnt = 0;
		static int tick_prev = 0;

		if (++frame_cnt == 100) {
			int tick_now = I_GetTime();
			int ticks = tick_now - tick_prev;
			int fps = (ticks > 0) ? (100 * 35 / ticks) : 99;
			console_printf("[FPS] %d\n", fps);
			tick_prev = tick_now;
			frame_cnt = 0;
			cycle_count_prev = cycle_count;
			instret_count_prev = instret_count;
			data_cache_hits_prev = data_cache_hits;
			load_requests_prev = load_requests;
			store_requests_prev = store_requests;
			uncached_requests_prev = uncached_requests;
			instruction_cache_misses_prev = instruction_cache_misses;
			instruction_cache_hits_prev = instruction_cache_hits;
			branch_misspredictions_prev = branch_misspredictions;
			__asm__ volatile ("csrr %0, mcycle" : "=r" (cycle_count));
			__asm__ volatile ("csrr %0, minstret" : "=r" (instret_count));
			__asm__ volatile ("csrr %0, mhpmcounter4" : "=r" (data_cache_hits));
			__asm__ volatile ("csrr %0, mhpmcounter5" : "=r" (load_requests));
			__asm__ volatile ("csrr %0, mhpmcounter6" : "=r" (store_requests));
			__asm__ volatile ("csrr %0, mhpmcounter11" : "=r" (uncached_requests));
			__asm__ volatile ("csrr %0, mhpmcounter9" : "=r" (instruction_cache_misses));
			__asm__ volatile ("csrr %0, mhpmcounter3" : "=r" (instruction_cache_hits));
			__asm__ volatile ("csrr %0, mhpmcounter7" : "=r" (branch_misspredictions));
			int cycle_diff = cycle_count - cycle_count_prev;
			int instret_diff = instret_count - instret_count_prev;
			int data_cache_hits_diff = data_cache_hits - data_cache_hits_prev;
			int load_requests_diff = load_requests - load_requests_prev;
			int store_requests_diff = store_requests - store_requests_prev;
			int uncached_requests_diff = uncached_requests - uncached_requests_prev;
			int instruction_cache_misses_diff = instruction_cache_misses - instruction_cache_misses_prev;
			int instruction_cache_hits_diff = instruction_cache_hits - instruction_cache_hits_prev;
			int branch_misspredictions_diff = branch_misspredictions - branch_misspredictions_prev;
			console_printf("[Cycle Count] %d\n", cycle_diff);
			console_printf("[Instret Count] %d\n", instret_diff);
			console_printf("[Data Cache Hits] %d \n", data_cache_hits_diff);
			console_printf("[Load/Store Ops] %d \n", (load_requests_diff + store_requests_diff) - uncached_requests_diff);
			console_printf("[Inst Cache Hits] %d \n", instruction_cache_hits_diff);
			console_printf("[Inst Cache Misses] %d \n", instruction_cache_misses_diff);
			console_printf("[Branch Misspredictions] %d \n", branch_misspredictions_diff);
		}
	}
}


void
I_WaitVBL(int count)
{
	while (count-- > 0) {
		while (!(VGA_FB_STATUS & 1))
			;
	}
}


void
I_ReadScreen(byte* scr)
{
	memcpy(
		scr,
		screens[0],
		SCREENHEIGHT * SCREENWIDTH
	);
}
