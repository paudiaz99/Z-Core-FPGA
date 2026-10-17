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
#include <time.h>
#include <unistd.h>


#define TICKS_PER_TIC  1428571


void
I_Init(void)
{
}


byte *
I_ZoneBase(int *size)
{
	byte *zone;
	*size = 6 * 1024 * 1024;
	zone = (byte *)malloc(*size);
	if (!zone) {
		*size = 3 * 1024 * 1024;
		zone = (byte *)malloc(*size);
		if (zone)
			fprintf(stderr, "DOOM: using a 3 MiB zone after allocation failure\n");
	}
	if (!zone)
		I_Error("Cannot allocate even a 3 MiB zone; physical RAM is fragmented");
	return zone;
}


int I_GetTime(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC,&t);
    return (int)(t.tv_sec*35 + t.tv_nsec*35LL/1000000000LL);
}

static void I_GetRemoteEvent(void) {
    static int previous=-1, release_at;
    event_t event;
    if(previous>=0 && I_GetTime()>=release_at) {
        event.type=ev_keyup; event.data1=previous; event.data2=event.data3=0;
        D_PostEvent(&event); previous=-1;
    }
    unsigned char ch;
    while(read(0,&ch,1)==1) {
        int key=ch;
        switch(ch) {
        case 'w': key=KEY_UPARROW; break;
        case 's': key=KEY_DOWNARROW; break;
        case 'a': key=KEY_LEFTARROW; break;
        case 'd': key=KEY_RIGHTARROW; break;
        case ' ': key=KEY_RCTRL; break;
        case 'e': key=' '; break;
        case '\r': case '\n': key=KEY_ENTER; break;
        case 27: key=KEY_ESCAPE; break;
        }
        if(previous>=0 && previous!=key) {
            event.type=ev_keyup; event.data1=previous; D_PostEvent(&event);
        }
        event.type=ev_keydown; event.data1=key; event.data2=event.data3=0;
        D_PostEvent(&event); previous=key; release_at=I_GetTime()+4;
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
