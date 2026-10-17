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
// DESCRIPTION:
//      Map Objects, MObj, definition and handling.
//
//-----------------------------------------------------------------------------


#ifndef __P_MOBJ__
#define __P_MOBJ__

#include "tables.h"
#include "m_fixed.h"

#include "d_think.h"

#include "doomdata.h"

#include "info.h"



#ifdef __GNUG__
#pragma interface
#endif




typedef enum
{
    MF_SPECIAL          = 1,
    MF_SOLID            = 2,
    MF_SHOOTABLE        = 4,
    MF_NOSECTOR         = 8,
    MF_NOBLOCKMAP       = 16,

    MF_AMBUSH           = 32,
    MF_JUSTHIT          = 64,
    MF_JUSTATTACKED     = 128,
    MF_SPAWNCEILING     = 256,
    MF_NOGRAVITY        = 512,

    MF_DROPOFF          = 0x400,
    MF_PICKUP           = 0x800,
    MF_NOCLIP           = 0x1000,
    MF_SLIDE            = 0x2000,
    MF_FLOAT            = 0x4000,
    MF_TELEPORT         = 0x8000,
    MF_MISSILE          = 0x10000,
    MF_DROPPED          = 0x20000,
    MF_SHADOW           = 0x40000,
    MF_NOBLOOD          = 0x80000,
    MF_CORPSE           = 0x100000,
    MF_INFLOAT          = 0x200000,

    MF_COUNTKILL        = 0x400000,

    MF_COUNTITEM        = 0x800000,

    MF_SKULLFLY         = 0x1000000,

    MF_NOTDMATCH        = 0x2000000,

    MF_TRANSLATION      = 0xc000000,
    MF_TRANSSHIFT       = 26

} mobjflag_t;


typedef struct mobj_s
{
    thinker_t           thinker;

    fixed_t             x;
    fixed_t             y;
    fixed_t             z;

    struct mobj_s*      snext;
    struct mobj_s*      sprev;

    angle_t             angle;
    spritenum_t         sprite;
    int                 frame;

    struct mobj_s*      bnext;
    struct mobj_s*      bprev;

    struct subsector_s* subsector;

    fixed_t             floorz;
    fixed_t             ceilingz;

    fixed_t             radius;
    fixed_t             height;

    fixed_t             momx;
    fixed_t             momy;
    fixed_t             momz;

    int                 validcount;

    mobjtype_t          type;
    mobjinfo_t*         info;

    int                 tics;
    state_t*            state;
    int                 flags;
    int                 health;

    int                 movedir;
    int                 movecount;

    struct mobj_s*      target;

    int                 reactiontime;

    int                 threshold;

    struct player_s*    player;

    int                 lastlook;

    mapthing_t          spawnpoint;

    struct mobj_s*      tracer;

} mobj_t;



#endif
