// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Log:$
//
// DESCRIPTION:
//      Archiving: SaveGame I/O.
//      Thinker, Ticker.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: p_tick.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";


#include <stdint.h>

#include "z_zone.h"
#include "p_local.h"

#include "doomstat.h"

#include "riscv/console.h"

extern char _etext[];
#define TEXT_LO  ((uintptr_t)0x10000000)
#define TEXT_HI  ((uintptr_t)_etext)


int     leveltime;




thinker_t       thinkercap;


void P_InitThinkers (void)
{
    thinkercap.prev = thinkercap.next  = &thinkercap;
}




void P_AddThinker (thinker_t* thinker)
{
    thinkercap.prev->next = thinker;
    thinker->next = &thinkercap;
    thinker->prev = thinkercap.prev;
    thinkercap.prev = thinker;
}



void P_RemoveThinker (thinker_t* thinker)
{
  thinker->function.acv = (actionf_v)(-1);
}



void P_AllocateThinker (thinker_t*      thinker)
{
}



void P_RunThinkers (void)
{
    thinker_t*  currentthinker;

    currentthinker = thinkercap.next;
    while (currentthinker != &thinkercap)
    {
        volatile actionf_p1 fn = currentthinker->function.acp1;

        if ( (actionf_v)fn == (actionf_v)(-1) )
        {
            currentthinker->next->prev = currentthinker->prev;
            currentthinker->prev->next = currentthinker->next;
            Z_Free (currentthinker);
        }
        else if (fn)
        {
#if 0
            uintptr_t fp = (uintptr_t)fn;
            if (fp < TEXT_LO || fp >= TEXT_HI || (fp & 3u)) {
                const uint32_t *raw = (const uint32_t *)currentthinker;
                const uint32_t *hdr = raw - (sizeof(memblock_t) / 4);
                console_printf(
                    "[THINK] bad fn=%08x thk=%08x "
                    "prev=%08x next=%08x etext=%08x\r\n",
                    (unsigned)fp, (unsigned)(uintptr_t)currentthinker,
                    (unsigned)raw[0], (unsigned)raw[1],
                    (unsigned)TEXT_HI);
                console_printf(
                    "        blk: size=%08x user=%08x tag=%08x id=%08x\r\n",
                    (unsigned)hdr[0], (unsigned)hdr[1],
                    (unsigned)hdr[2], (unsigned)hdr[3]);
                currentthinker->next->prev = currentthinker->prev;
                currentthinker->prev->next = currentthinker->next;
                thinker_t *next = currentthinker->next;
                Z_Free(currentthinker);
                currentthinker = next;
                continue;
            }
#endif
            fn(currentthinker);
        }
        currentthinker = currentthinker->next;
    }
}




void P_Ticker (void)
{
    int         i;

    if (paused)
        return;

    if ( !netgame
         && menuactive
         && !demoplayback
         && players[consoleplayer].viewz != 1)
    {
        return;
    }


    for (i=0 ; i<MAXPLAYERS ; i++)
        if (playeringame[i])
            P_PlayerThink (&players[i]);

    P_RunThinkers ();
    P_UpdateSpecials ();
    P_RespawnSpecials ();

    leveltime++;
}
