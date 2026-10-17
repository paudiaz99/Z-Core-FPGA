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
//      The not so system specific sound interface.
//
//-----------------------------------------------------------------------------


#ifndef __S_SOUND__
#define __S_SOUND__


#ifdef __GNUG__
#pragma interface
#endif



void
S_Init
( int           sfxVolume,
  int           musicVolume );




void S_Start(void);


void
S_StartSound
( void*         origin,
  int           sound_id );



void
S_StartSoundAtVolume
( void*         origin,
  int           sound_id,
  int           volume );


void S_StopSound(void* origin);


void S_StartMusic(int music_id);

void
S_ChangeMusic
( int           music_id,
  int           looping );

void S_StopMusic(void);

void S_PauseSound(void);
void S_ResumeSound(void);


void S_UpdateSounds(void* listener);

void S_SetMusicVolume(int volume);
void S_SetSfxVolume(int volume);


#endif
