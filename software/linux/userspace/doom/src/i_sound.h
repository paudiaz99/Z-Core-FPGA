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
//
// DESCRIPTION:
//      System interface, sound.
//
//-----------------------------------------------------------------------------

#ifndef __I_SOUND__
#define __I_SOUND__

#include "doomdef.h"

#ifdef SNDSERV
#include <stdio.h>
extern FILE* sndserver;
extern char* sndserver_filename;
#endif

#include "doomstat.h"
#include "sounds.h"



void I_InitSound();

void I_UpdateSound(void);
void I_SubmitSound(void);

void I_ShutdownSound(void);



void I_SetChannels();

int I_GetSfxLumpNum (sfxinfo_t* sfxinfo );


int
I_StartSound
( int           id,
  int           vol,
  int           sep,
  int           pitch,
  int           priority );


void I_StopSound(int handle);

int I_SoundIsPlaying(int handle);

void
I_UpdateSoundParams
( int           handle,
  int           vol,
  int           sep,
  int           pitch );


void I_InitMusic(void);
void I_ShutdownMusic(void);
void I_SetMusicVolume(int volume);
void I_PauseSong(int handle);
void I_ResumeSong(int handle);
int I_RegisterSong(void *data);
void
I_PlaySong
( int           handle,
  int           looping );
void I_StopSong(int handle);
void I_UnRegisterSong(int handle);



#endif
