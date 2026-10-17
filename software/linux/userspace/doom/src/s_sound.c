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
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: s_sound.c,v 1.6 1997/02/03 22:45:12 b1 Exp $";


#include <stdio.h>
#include <stdlib.h>

#include "i_system.h"
#include "i_sound.h"
#include "sounds.h"
#include "s_sound.h"

#include "z_zone.h"
#include "m_random.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "doomstat.h"


const char snd_prefixen[]
= { 'P', 'P', 'A', 'S', 'S', 'S', 'M', 'M', 'M', 'S', 'S', 'S' };

#define S_MAX_VOLUME            127

#define S_CLIPPING_DIST         (1200*0x10000)

#define S_CLOSE_DIST            (160*0x10000)


#define S_ATTENUATOR            ((S_CLIPPING_DIST-S_CLOSE_DIST)>>FRACBITS)

#define NORM_VOLUME             snd_MaxVolume

#define NORM_PITCH              128
#define NORM_PRIORITY           64
#define NORM_SEP                128

#define S_PITCH_PERTURB         1
#define S_STEREO_SWING          (96*0x10000)

#define S_IFRACVOL              30

#define NA                      0
#define S_NUMCHANNELS           2


extern int snd_MusicDevice;
extern int snd_SfxDevice;
extern int snd_DesiredMusicDevice;
extern int snd_DesiredSfxDevice;



typedef struct
{
    sfxinfo_t*  sfxinfo;

    void*       origin;

    int         handle;

} channel_t;


static channel_t*       channels;

int             snd_SfxVolume = 15;

int             snd_MusicVolume = 15;



static boolean          mus_paused;

static musicinfo_t*     mus_playing=0;

int                     numChannels;

static int              nextcleanup;



int
S_getChannel
( void*         origin,
  sfxinfo_t*    sfxinfo );


int
S_AdjustSoundParams
( mobj_t*       listener,
  mobj_t*       source,
  int*          vol,
  int*          sep,
  int*          pitch );

void S_StopChannel(int cnum);



void S_Init
( int           sfxVolume,
  int           musicVolume )
{
  int           i;

  fprintf( stderr, "S_Init: default sfx volume %d\n", sfxVolume);

  I_SetChannels();

  S_SetSfxVolume(sfxVolume);
  S_SetMusicVolume(musicVolume);

  channels =
    (channel_t *) Z_Malloc(numChannels*sizeof(channel_t), PU_STATIC, 0);

  for (i=0 ; i<numChannels ; i++)
    channels[i].sfxinfo = 0;

  mus_paused = 0;

  for (i=1 ; i<NUMSFX ; i++)
    S_sfx[i].lumpnum = S_sfx[i].usefulness = -1;
}




void S_Start(void)
{
  int cnum;
  int mnum;

  for (cnum=0 ; cnum<numChannels ; cnum++)
    if (channels[cnum].sfxinfo)
      S_StopChannel(cnum);

  mus_paused = 0;

  if (gamemode == commercial)
    mnum = mus_runnin + gamemap - 1;
  else
  {
    int spmus[]=
    {

      mus_e3m4,
      mus_e3m2,
      mus_e3m3,
      mus_e1m5,
      mus_e2m7,
      mus_e2m4,
      mus_e2m6,
      mus_e2m5,
      mus_e1m9
    };

    if (gameepisode < 4)
      mnum = mus_e1m1 + (gameepisode-1)*9 + gamemap-1;
    else
      mnum = spmus[gamemap-1];
    }


  S_ChangeMusic(mnum, true);

  nextcleanup = 15;
}





void
S_StartSoundAtVolume
( void*         origin_p,
  int           sfx_id,
  int           volume )
{

  int           rc;
  int           sep;
  int           pitch;
  int           priority;
  sfxinfo_t*    sfx;
  int           cnum;

  mobj_t*       origin = (mobj_t *) origin_p;



  if (sfx_id < 1 || sfx_id > NUMSFX)
    I_Error("Bad sfx #: %d", sfx_id);

  sfx = &S_sfx[sfx_id];

  if (sfx->link)
  {
    pitch = sfx->pitch;
    priority = sfx->priority;
    volume += sfx->volume;

    if (volume < 1)
      return;

    if (volume > snd_SfxVolume)
      volume = snd_SfxVolume;
  }
  else
  {
    pitch = NORM_PITCH;
    priority = NORM_PRIORITY;
  }


  if (origin && origin != players[consoleplayer].mo)
  {
    rc = S_AdjustSoundParams(players[consoleplayer].mo,
                             origin,
                             &volume,
                             &sep,
                             &pitch);

    if ( origin->x == players[consoleplayer].mo->x
         && origin->y == players[consoleplayer].mo->y)
    {
      sep       = NORM_SEP;
    }

    if (!rc)
      return;
  }
  else
  {
    sep = NORM_SEP;
  }

  if (sfx_id >= sfx_sawup
      && sfx_id <= sfx_sawhit)
  {
    pitch += 8 - (M_Random()&15);

    if (pitch<0)
      pitch = 0;
    else if (pitch>255)
      pitch = 255;
  }
  else if (sfx_id != sfx_itemup
           && sfx_id != sfx_tink)
  {
    pitch += 16 - (M_Random()&31);

    if (pitch<0)
      pitch = 0;
    else if (pitch>255)
      pitch = 255;
  }

  S_StopSound(origin);

  cnum = S_getChannel(origin, sfx);

  if (cnum<0)
    return;


  if (sfx->lumpnum < 0)
    sfx->lumpnum = I_GetSfxLumpNum(sfx);

#ifndef SNDSRV
  if (!sfx->data)
  {
    fprintf( stderr,
             "S_StartSoundAtVolume: 16bit and not pre-cached - wtf?\n");


  }
#endif

  if (sfx->usefulness++ < 0)
    sfx->usefulness = 1;

  channels[cnum].handle = I_StartSound(sfx_id,
                                       volume,
                                       sep,
                                       pitch,
                                       priority);
}

void
S_StartSound
( void*         origin,
  int           sfx_id )
{
#ifdef SAWDEBUG
#endif

    S_StartSoundAtVolume(origin, sfx_id, snd_SfxVolume);


#ifdef SAWDEBUG
{
    int i;
    int n;

    static mobj_t*      last_saw_origins[10] = {1,1,1,1,1,1,1,1,1,1};
    static int          first_saw=0;
    static int          next_saw=0;

    if (sfx_id == sfx_sawidl
        || sfx_id == sfx_sawful
        || sfx_id == sfx_sawhit)
    {
        for (i=first_saw;i!=next_saw;i=(i+1)%10)
            if (last_saw_origins[i] != origin)
                fprintf(stderr, "old origin 0x%lx != "
                        "origin 0x%lx for sfx %d\n",
                        last_saw_origins[i],
                        origin,
                        sfx_id);

        last_saw_origins[next_saw] = origin;
        next_saw = (next_saw + 1) % 10;
        if (next_saw == first_saw)
            first_saw = (first_saw + 1) % 10;

        for (n=i=0; i<numChannels ; i++)
        {
            if (channels[i].sfxinfo == &S_sfx[sfx_sawidl]
                || channels[i].sfxinfo == &S_sfx[sfx_sawful]
                || channels[i].sfxinfo == &S_sfx[sfx_sawhit]) n++;
        }

        if (n>1)
        {
            for (i=0; i<numChannels ; i++)
            {
                if (channels[i].sfxinfo == &S_sfx[sfx_sawidl]
                    || channels[i].sfxinfo == &S_sfx[sfx_sawful]
                    || channels[i].sfxinfo == &S_sfx[sfx_sawhit])
                {
                    fprintf(stderr,
                            "chn: sfxinfo=0x%lx, origin=0x%lx, "
                            "handle=%d\n",
                            channels[i].sfxinfo,
                            channels[i].origin,
                            channels[i].handle);
                }
            }
            fprintf(stderr, "\n");
        }
    }
}
#endif

}




void S_StopSound(void *origin)
{

    int cnum;

    for (cnum=0 ; cnum<numChannels ; cnum++)
    {
        if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
        {
            S_StopChannel(cnum);
            break;
        }
    }
}









void S_PauseSound(void)
{
    if (mus_playing && !mus_paused)
    {
        I_PauseSong(mus_playing->handle);
        mus_paused = true;
    }
}

void S_ResumeSound(void)
{
    if (mus_playing && mus_paused)
    {
        I_ResumeSong(mus_playing->handle);
        mus_paused = false;
    }
}


void S_UpdateSounds(void* listener_p)
{
    int         audible;
    int         cnum;
    int         volume;
    int         sep;
    int         pitch;
    sfxinfo_t*  sfx;
    channel_t*  c;

    mobj_t*     listener = (mobj_t*)listener_p;




    for (cnum=0 ; cnum<numChannels ; cnum++)
    {
        c = &channels[cnum];
        sfx = c->sfxinfo;

        if (c->sfxinfo)
        {
            if (I_SoundIsPlaying(c->handle))
            {
                volume = snd_SfxVolume;
                pitch = NORM_PITCH;
                sep = NORM_SEP;

                if (sfx->link)
                {
                    pitch = sfx->pitch;
                    volume += sfx->volume;
                    if (volume < 1)
                    {
                        S_StopChannel(cnum);
                        continue;
                    }
                    else if (volume > snd_SfxVolume)
                    {
                        volume = snd_SfxVolume;
                    }
                }

                if (c->origin && listener_p != c->origin)
                {
                    audible = S_AdjustSoundParams(listener,
                                                  c->origin,
                                                  &volume,
                                                  &sep,
                                                  &pitch);

                    if (!audible)
                    {
                        S_StopChannel(cnum);
                    }
                    else
                        I_UpdateSoundParams(c->handle, volume, sep, pitch);
                }
            }
            else
            {
                S_StopChannel(cnum);
            }
        }
    }
}


void S_SetMusicVolume(int volume)
{
    if (volume < 0 || volume > 127)
    {
        I_Error("Attempt to set music volume at %d",
                volume);
    }

    I_SetMusicVolume(127);
    I_SetMusicVolume(volume);
    snd_MusicVolume = volume;
}



void S_SetSfxVolume(int volume)
{

    if (volume < 0 || volume > 127)
        I_Error("Attempt to set sfx volume at %d", volume);

    snd_SfxVolume = volume;

}

void S_StartMusic(int m_id)
{
    S_ChangeMusic(m_id, false);
}

void
S_ChangeMusic
( int                   musicnum,
  int                   looping )
{
    musicinfo_t*        music;
    char                namebuf[9];

    if ( (musicnum <= mus_None)
         || (musicnum >= NUMMUSIC) )
    {
        I_Error("Bad music number %d", musicnum);
    }
    else
        music = &S_music[musicnum];

    if (mus_playing == music)
        return;

    S_StopMusic();

    if (!music->lumpnum)
    {
        sprintf(namebuf, "d_%s", music->name);
        music->lumpnum = W_GetNumForName(namebuf);
    }

    music->data = (void *) W_CacheLumpNum(music->lumpnum, PU_MUSIC);
    music->handle = I_RegisterSong(music->data);

    I_PlaySong(music->handle, looping);

    mus_playing = music;
}


void S_StopMusic(void)
{
    if (mus_playing)
    {
        if (mus_paused)
            I_ResumeSong(mus_playing->handle);

        I_StopSong(mus_playing->handle);
        I_UnRegisterSong(mus_playing->handle);
        Z_ChangeTag(mus_playing->data, PU_CACHE);

        mus_playing->data = 0;
        mus_playing = 0;
    }
}




void S_StopChannel(int cnum)
{

    int         i;
    channel_t*  c = &channels[cnum];

    if (c->sfxinfo)
    {
        if (I_SoundIsPlaying(c->handle))
        {
#ifdef SAWDEBUG
            if (c->sfxinfo == &S_sfx[sfx_sawful])
                fprintf(stderr, "stopped\n");
#endif
            I_StopSound(c->handle);
        }

        for (i=0 ; i<numChannels ; i++)
        {
            if (cnum != i
                && c->sfxinfo == channels[i].sfxinfo)
            {
                break;
            }
        }

        c->sfxinfo->usefulness--;

        c->sfxinfo = 0;
    }
}



int
S_AdjustSoundParams
( mobj_t*       listener,
  mobj_t*       source,
  int*          vol,
  int*          sep,
  int*          pitch )
{
    fixed_t     approx_dist;
    fixed_t     adx;
    fixed_t     ady;
    angle_t     angle;

    adx = abs(listener->x - source->x);
    ady = abs(listener->y - source->y);

    approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);

    if (gamemap != 8
        && approx_dist > S_CLIPPING_DIST)
    {
        return 0;
    }

    angle = R_PointToAngle2(listener->x,
                            listener->y,
                            source->x,
                            source->y);

    if (angle > listener->angle)
        angle = angle - listener->angle;
    else
        angle = angle + (0xffffffff - listener->angle);

    angle >>= ANGLETOFINESHIFT;

    *sep = 128 - (FixedMul(S_STEREO_SWING,finesine[angle])>>FRACBITS);

    if (approx_dist < S_CLOSE_DIST)
    {
        *vol = snd_SfxVolume;
    }
    else if (gamemap == 8)
    {
        if (approx_dist > S_CLIPPING_DIST)
            approx_dist = S_CLIPPING_DIST;

        *vol = 15+ ((snd_SfxVolume-15)
                    *((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
            / S_ATTENUATOR;
    }
    else
    {
        *vol = (snd_SfxVolume
                * ((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
            / S_ATTENUATOR;
    }

    return (*vol > 0);
}




int
S_getChannel
( void*         origin,
  sfxinfo_t*    sfxinfo )
{
    int         cnum;

    channel_t*  c;

    for (cnum=0 ; cnum<numChannels ; cnum++)
    {
        if (!channels[cnum].sfxinfo)
            break;
        else if (origin &&  channels[cnum].origin ==  origin)
        {
            S_StopChannel(cnum);
            break;
        }
    }

    if (cnum == numChannels)
    {
        for (cnum=0 ; cnum<numChannels ; cnum++)
            if (channels[cnum].sfxinfo->priority >= sfxinfo->priority) break;

        if (cnum == numChannels)
        {
            return -1;
        }
        else
        {
            S_StopChannel(cnum);
        }
    }

    c = &channels[cnum];

    c->sfxinfo = sfxinfo;
    c->origin = origin;

    return cnum;
}




