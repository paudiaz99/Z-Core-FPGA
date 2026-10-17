/*
 * i_system.c
 *
 * System support code for Z-Core
 *
 * Copyright (C) 1993-1996 by id Software, Inc.
 * Copyright (C) 2021 Sylvain Munaut
 * Adapted for Z-Core RISC-V SoC
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomdef.h"
#include "doomstat.h"

#include "d_main.h"
#include "g_game.h"
#include "m_misc.h"
#include "i_sound.h"
#include "i_video.h"

#include "i_system.h"

#include "console.h"
#include "config.h"


#define TICKS_PER_TIC  1428571


void
I_Init(void)
{
	TIMER_CTRL_REG = 0x03;
}


byte *
I_ZoneBase(int *size)
{
	*size = 12 * 1024 * 1024;
	return (byte *) malloc (*size);
}


static uint32_t timer_last = 0;
static uint32_t timer_accum = 0;
static int      doom_tics = 0;

int
I_GetTime(void)
{
	uint32_t now = TIMER_LO;
	timer_accum += (now - timer_last);
	timer_last = now;

	while (timer_accum >= TICKS_PER_TIC) {
		timer_accum -= TICKS_PER_TIC;
		doom_tics++;
	}

	return doom_tics;
}


static void
I_GetRemoteEvent(void)
{
	event_t event;

	const char map[] = {
		KEY_LEFTARROW,
		KEY_RIGHTARROW,
		KEY_DOWNARROW,
		KEY_UPARROW,
		KEY_RSHIFT,
		KEY_RCTRL,
		KEY_RALT,
		KEY_ESCAPE,
		KEY_ENTER,
		KEY_TAB,
		KEY_BACKSPACE,
		KEY_PAUSE,
		KEY_EQUALS,
		KEY_MINUS,
		KEY_F1,
		KEY_F2,
		KEY_F3,
		KEY_F4,
		KEY_F5,
		KEY_F6,
		KEY_F7,
		KEY_F8,
		KEY_F9,
		KEY_F10,
		KEY_F11,
		KEY_F12,
	};

	static byte s_btn = 0;

	boolean mupd = false;
	int mdx = 0;
	int mdy = 0;

	while (1) {
		int ch = console_getchar_nowait();
		if (ch == -1)
			break;

		boolean msb = ch & 0x80;
		ch &= 0x7f;

		if (ch < 28) {
			event.type = msb ? ev_keydown : ev_keyup;
			event.data1 = map[ch];
			D_PostEvent(&event);
		} else if (ch < 31) {
			if (msb)
				s_btn |= (1 << ((ch & 0x7f) - 28));
			else
				s_btn &= ~(1 << ((ch & 0x7f) - 28));
			mupd = true;
		} else if (ch == 0x1f) {
			signed char x = console_getchar();
			signed char y = console_getchar();
			mdx += x;
			mdy += y;
			mupd = true;
		} else {
			event.type = msb ? ev_keydown : ev_keyup;
			event.data1 = ch;
			D_PostEvent(&event);
		}
	}

	if (mupd) {
		event.type = ev_mouse;
		event.data1 = s_btn;
		event.data2 =   mdx << 2;
		event.data3 = - mdy << 2;
		D_PostEvent(&event);
	}
}

void
I_StartFrame(void)
{
}

void
I_StartTic(void)
{
	I_GetRemoteEvent();
}

ticcmd_t *
I_BaseTiccmd(void)
{
	static ticcmd_t emptycmd;
	return &emptycmd;
}


void
I_Quit(void)
{
	D_QuitNetGame();
	M_SaveDefaults();
	I_ShutdownGraphics();
	exit(0);
}


byte *
I_AllocLow(int length)
{
	byte*	mem;
	mem = (byte *)malloc (length);
	memset (mem,0,length);
	return mem;
}


void
I_Tactile
( int on,
  int off,
  int total )
{
	on = off = total = 0;
}


void
I_Error(char *error, ...)
{
	va_list	argptr;

	va_start (argptr,error);
	fprintf (stderr, "Error: ");
	vfprintf (stderr,error,argptr);
	fprintf (stderr, "\n");
	va_end (argptr);

	fflush( stderr );

	if (demorecording)
		G_CheckDemoStatus();

	D_QuitNetGame ();
	I_ShutdownGraphics();

	exit(-1);
}
