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
//
//
//-----------------------------------------------------------------------------


#ifndef __D_EVENT__
#define __D_EVENT__


#include "doomtype.h"



typedef enum
{
    ev_keydown,
    ev_keyup,
    ev_mouse,
    ev_joystick
} evtype_t;

typedef struct
{
    evtype_t    type;
    int         data1;
    int         data2;
    int         data3;
} event_t;


typedef enum
{
    ga_nothing,
    ga_loadlevel,
    ga_newgame,
    ga_loadgame,
    ga_savegame,
    ga_playdemo,
    ga_completed,
    ga_victory,
    ga_worlddone,
    ga_screenshot
} gameaction_t;



typedef enum
{
    BT_ATTACK           = 1,
    BT_USE              = 2,

    BT_SPECIAL          = 128,
    BT_SPECIALMASK      = 3,

    BT_CHANGE           = 4,
    BT_WEAPONMASK       = (8+16+32),
    BT_WEAPONSHIFT      = 3,

    BTS_PAUSE           = 1,
    BTS_SAVEGAME        = 2,

    BTS_SAVEMASK        = (4+8+16),
    BTS_SAVESHIFT       = 2,

} buttoncode_t;




#define MAXEVENTS               64

extern  event_t         events[MAXEVENTS];
extern  int             eventhead;
extern  int             eventtail;

extern  gameaction_t    gameaction;


#endif
