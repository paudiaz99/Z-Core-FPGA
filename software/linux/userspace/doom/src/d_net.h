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
//      Networking stuff.
//
//-----------------------------------------------------------------------------


#ifndef __D_NET__
#define __D_NET__

#include "d_player.h"


#ifdef __GNUG__
#pragma interface
#endif



#define DOOMCOM_ID              0x12345678l

#define MAXNETNODES             8


#define BACKUPTICS              12

typedef enum
{
    CMD_SEND    = 1,
    CMD_GET     = 2

} command_t;


typedef struct
{
    unsigned            checksum;
    byte                retransmitfrom;

    byte                starttic;
    byte                player;
    byte                numtics;
    ticcmd_t            cmds[BACKUPTICS];

} doomdata_t;




typedef struct
{
    long                id;

    short               intnum;
    short               command;
    short               remotenode;

    short               datalength;

    short               numnodes;
    short               ticdup;
    short               extratics;
    short               deathmatch;
    short               savegame;
    short               episode;
    short               map;
    short               skill;

    short               consoleplayer;
    short               numplayers;

    short               angleoffset;
    short               drone;

    doomdata_t          data;

} doomcom_t;



void NetUpdate (void);

void D_QuitNetGame (void);

void TryRunTics (void);


#endif


