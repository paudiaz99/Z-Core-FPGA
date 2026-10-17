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
//   All the global variables that store the internal state.
//   Theoretically speaking, the internal state of the engine
//    should be found by looking at the variables collected
//    here, and every relevant module will have to include
//    this header file.
//   In practice, things are a bit messy.
//
//-----------------------------------------------------------------------------


#ifndef __D_STATE__
#define __D_STATE__

#include "doomdata.h"
#include "d_net.h"

#include "d_player.h"


#ifdef __GNUG__
#pragma interface
#endif



extern  boolean nomonsters;
extern  boolean respawnparm;
extern  boolean fastparm;

extern  boolean devparm;



extern GameMode_t       gamemode;
extern GameMission_t    gamemission;

extern  boolean modifiedgame;


extern  Language_t   language;



extern  skill_t         startskill;
extern  int             startepisode;
extern  int             startmap;

extern  boolean         autostart;

extern  skill_t         gameskill;
extern  int             gameepisode;
extern  int             gamemap;

extern  boolean         respawnmonsters;

extern  boolean netgame;

extern  boolean deathmatch;


extern int snd_SfxVolume;
extern int snd_MusicVolume;

extern int snd_MusicDevice;
extern int snd_SfxDevice;
extern int snd_DesiredMusicDevice;
extern int snd_DesiredSfxDevice;



extern  boolean statusbaractive;

extern  boolean automapactive;
extern  boolean menuactive;
extern  boolean paused;


extern  boolean         viewactive;

extern  boolean         nodrawers;
extern  boolean         noblit;

extern  int             viewwindowx;
extern  int             viewwindowy;
extern  int             viewheight;
extern  int             viewwidth;
extern  int             scaledviewwidth;






extern  int     viewangleoffset;

extern  int     consoleplayer;
extern  int     displayplayer;


extern  int     totalkills;
extern  int     totalitems;
extern  int     totalsecret;

extern  int     levelstarttic;
extern  int     leveltime;



extern  boolean usergame;

extern  boolean demoplayback;
extern  boolean demorecording;

extern  boolean         singledemo;




extern  gamestate_t     gamestate;









extern  int             gametic;


extern  player_t        players[MAXPLAYERS];

extern  boolean         playeringame[MAXPLAYERS];


#define MAX_DM_STARTS   10
extern  mapthing_t      deathmatchstarts[MAX_DM_STARTS];
extern  mapthing_t*     deathmatch_p;

extern  mapthing_t      playerstarts[MAXPLAYERS];

extern  wbstartstruct_t         wminfo;


extern  int             maxammo[NUMAMMO];






extern  char            basedefault[];

#ifdef DEBUG
extern  FILE*           debugfile;
#else
#define debugfile 0
#endif

extern  boolean         precache;


extern  gamestate_t     wipegamestate;

extern  int             mouseSensitivity;
extern  boolean         singletics;

extern  int             bodyqueslot;



extern int              skyflatnum;




extern  doomcom_t*      doomcom;

extern  doomdata_t*     netbuffer;


extern  ticcmd_t        localcmds[BACKUPTICS];
extern  int             rndindex;

extern  int             maketic;
extern  int             nettics[MAXNETNODES];

extern  ticcmd_t        netcmds[MAXPLAYERS][BACKUPTICS];
extern  int             ticdup;



#endif
