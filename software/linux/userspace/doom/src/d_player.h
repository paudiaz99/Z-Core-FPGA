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


#ifndef __D_PLAYER__
#define __D_PLAYER__


#include "d_items.h"
#include "p_pspr.h"

#include "p_mobj.h"

#include "d_ticcmd.h"

#ifdef __GNUG__
#pragma interface
#endif




typedef enum
{
    PST_LIVE,
    PST_DEAD,
    PST_REBORN

} playerstate_t;


typedef enum
{
    CF_NOCLIP           = 1,
    CF_GODMODE          = 2,
    CF_NOMOMENTUM       = 4

} cheat_t;


typedef struct player_s
{
    mobj_t*             mo;
    playerstate_t       playerstate;
    ticcmd_t            cmd;

    fixed_t             viewz;
    fixed_t             viewheight;
    fixed_t             deltaviewheight;
    fixed_t             bob;

    int                 health;
    int                 armorpoints;
    int                 armortype;

    int                 powers[NUMPOWERS];
    boolean             cards[NUMCARDS];
    boolean             backpack;

    int                 frags[MAXPLAYERS];
    weapontype_t        readyweapon;

    weapontype_t        pendingweapon;

    boolean             weaponowned[NUMWEAPONS];
    int                 ammo[NUMAMMO];
    int                 maxammo[NUMAMMO];

    int                 attackdown;
    int                 usedown;

    int                 cheats;

    int                 refire;

    int                 killcount;
    int                 itemcount;
    int                 secretcount;

    char*               message;

    int                 damagecount;
    int                 bonuscount;

    mobj_t*             attacker;

    int                 extralight;

    int                 fixedcolormap;

    int                 colormap;

    pspdef_t            psprites[NUMPSPRITES];

    boolean             didsecret;

} player_t;


typedef struct
{
    boolean     in;

    int         skills;
    int         sitems;
    int         ssecret;
    int         stime;
    int         frags[4];
    int         score;

} wbplayerstruct_t;

typedef struct
{
    int         epsd;

    boolean     didsecret;

    int         last;
    int         next;

    int         maxkills;
    int         maxitems;
    int         maxsecret;
    int         maxfrags;

    int         partime;

    int         pnum;

    wbplayerstruct_t    plyr[MAXPLAYERS];

} wbstartstruct_t;


#endif
