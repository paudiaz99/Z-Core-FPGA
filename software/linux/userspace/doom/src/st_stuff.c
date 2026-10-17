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
//      Status bar code.
//      Does the face/direction indicator animatin.
//      Does palette indicators as well (red pain/berserk, bright pickup)
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: st_stuff.c,v 1.6 1997/02/03 22:45:13 b1 Exp $";


#include <stdio.h>

#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "m_random.h"
#include "w_wad.h"

#include "doomdef.h"

#include "g_game.h"

#include "st_stuff.h"
#include "st_lib.h"
#include "r_local.h"

#include "p_local.h"
#include "p_inter.h"

#include "am_map.h"
#include "m_cheat.h"

#include "s_sound.h"

#include "v_video.h"

#include "doomstat.h"

#include "dstrings.h"
#include "sounds.h"



#define STARTREDPALS            1
#define STARTBONUSPALS          9
#define NUMREDPALS                      8
#define NUMBONUSPALS            4
#define RADIATIONPAL            13

#define ST_FACEPROBABILITY              96

#define ST_TOGGLECHAT           KEY_ENTER

#define ST_X                            0
#define ST_X2                           104

#define ST_FX                   143
#define ST_FY                   169

#define ST_TALLNUMWIDTH         (tallnum[0]->width)

#define ST_NUMPAINFACES         5
#define ST_NUMSTRAIGHTFACES     3
#define ST_NUMTURNFACES         2
#define ST_NUMSPECIALFACES              3

#define ST_FACESTRIDE \
          (ST_NUMSTRAIGHTFACES+ST_NUMTURNFACES+ST_NUMSPECIALFACES)

#define ST_NUMEXTRAFACES                2

#define ST_NUMFACES \
          (ST_FACESTRIDE*ST_NUMPAINFACES+ST_NUMEXTRAFACES)

#define ST_TURNOFFSET           (ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET           (ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET               (ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET                (ST_EVILGRINOFFSET + 1)
#define ST_GODFACE                      (ST_NUMPAINFACES*ST_FACESTRIDE)
#define ST_DEADFACE                     (ST_GODFACE+1)

#define ST_FACESX                       143
#define ST_FACESY                       168

#define ST_EVILGRINCOUNT                (2*TICRATE)
#define ST_STRAIGHTFACECOUNT    (TICRATE/2)
#define ST_TURNCOUNT            (1*TICRATE)
#define ST_OUCHCOUNT            (1*TICRATE)
#define ST_RAMPAGEDELAY         (2*TICRATE)

#define ST_MUCHPAIN                     20



#define ST_AMMOWIDTH            3
#define ST_AMMOX                        44
#define ST_AMMOY                        171

#define ST_HEALTHWIDTH          3
#define ST_HEALTHX                      90
#define ST_HEALTHY                      171

#define ST_ARMSX                        111
#define ST_ARMSY                        172
#define ST_ARMSBGX                      104
#define ST_ARMSBGY                      168
#define ST_ARMSXSPACE           12
#define ST_ARMSYSPACE           10

#define ST_FRAGSX                       138
#define ST_FRAGSY                       171
#define ST_FRAGSWIDTH           2

#define ST_ARMORWIDTH           3
#define ST_ARMORX                       221
#define ST_ARMORY                       171

#define ST_KEY0WIDTH            8
#define ST_KEY0HEIGHT           5
#define ST_KEY0X                        239
#define ST_KEY0Y                        171
#define ST_KEY1WIDTH            ST_KEY0WIDTH
#define ST_KEY1X                        239
#define ST_KEY1Y                        181
#define ST_KEY2WIDTH            ST_KEY0WIDTH
#define ST_KEY2X                        239
#define ST_KEY2Y                        191

#define ST_AMMO0WIDTH           3
#define ST_AMMO0HEIGHT          6
#define ST_AMMO0X                       288
#define ST_AMMO0Y                       173
#define ST_AMMO1WIDTH           ST_AMMO0WIDTH
#define ST_AMMO1X                       288
#define ST_AMMO1Y                       179
#define ST_AMMO2WIDTH           ST_AMMO0WIDTH
#define ST_AMMO2X                       288
#define ST_AMMO2Y                       191
#define ST_AMMO3WIDTH           ST_AMMO0WIDTH
#define ST_AMMO3X                       288
#define ST_AMMO3Y                       185

#define ST_MAXAMMO0WIDTH                3
#define ST_MAXAMMO0HEIGHT               5
#define ST_MAXAMMO0X            314
#define ST_MAXAMMO0Y            173
#define ST_MAXAMMO1WIDTH                ST_MAXAMMO0WIDTH
#define ST_MAXAMMO1X            314
#define ST_MAXAMMO1Y            179
#define ST_MAXAMMO2WIDTH                ST_MAXAMMO0WIDTH
#define ST_MAXAMMO2X            314
#define ST_MAXAMMO2Y            191
#define ST_MAXAMMO3WIDTH                ST_MAXAMMO0WIDTH
#define ST_MAXAMMO3X            314
#define ST_MAXAMMO3Y            185

#define ST_WEAPON0X                     110
#define ST_WEAPON0Y                     172

#define ST_WEAPON1X                     122
#define ST_WEAPON1Y                     172

#define ST_WEAPON2X                     134
#define ST_WEAPON2Y                     172

#define ST_WEAPON3X                     110
#define ST_WEAPON3Y                     181

#define ST_WEAPON4X                     122
#define ST_WEAPON4Y                     181

#define ST_WEAPON5X                     134
#define ST_WEAPON5Y                     181

#define ST_WPNSX                        109
#define ST_WPNSY                        191

#define ST_DETHX                        109
#define ST_DETHY                        191

#define ST_MSGTEXTX                     0
#define ST_MSGTEXTY                     0
#define ST_MSGWIDTH                     52
#define ST_MSGHEIGHT            1

#define ST_OUTTEXTX                     0
#define ST_OUTTEXTY                     6

#define ST_OUTWIDTH                     52
#define ST_OUTHEIGHT            1

#define ST_MAPWIDTH     \
    (strlen(mapnames[(gameepisode-1)*9+(gamemap-1)]))

#define ST_MAPTITLEX \
    (SCREENWIDTH - ST_MAPWIDTH * ST_CHATFONTWIDTH)

#define ST_MAPTITLEY            0
#define ST_MAPHEIGHT            1


static player_t*        plyr;

static boolean          st_firsttime;

static int              veryfirsttime = 1;

static int              lu_palette;

static unsigned int     st_clock;

static int              st_msgcounter=0;

static st_chatstateenum_t       st_chatstate;

static st_stateenum_t   st_gamestate;

static boolean          st_statusbaron;

static boolean          st_chat;

static boolean          st_oldchat;

static boolean          st_cursoron;

static boolean          st_notdeathmatch;

static boolean          st_armson;

static boolean          st_fragson;

static patch_t*         sbar;

static patch_t*         tallnum[10];

static patch_t*         tallpercent;

static patch_t*         shortnum[10];

static patch_t*         keys[NUMCARDS];

static patch_t*         faces[ST_NUMFACES];

static patch_t*         faceback;

static patch_t*         armsbg;

static patch_t*         arms[6][2];

static st_number_t      w_ready;

static st_number_t      w_frags;

static st_percent_t     w_health;

static st_binicon_t     w_armsbg;


static st_multicon_t    w_arms[6];

static st_multicon_t    w_faces;

static st_multicon_t    w_keyboxes[3];

static st_percent_t     w_armor;

static st_number_t      w_ammo[4];

static st_number_t      w_maxammo[4];



static int      st_fragscount;

static int      st_oldhealth = -1;

static boolean  oldweaponsowned[NUMWEAPONS];

static int      st_facecount = 0;

static int      st_faceindex = 0;

static int      keyboxes[3];

static int      st_randomnumber;



unsigned char   cheat_mus_seq[] =
{
    0xb2, 0x26, 0xb6, 0xae, 0xea, 1, 0, 0, 0xff
};

unsigned char   cheat_choppers_seq[] =
{
    0xb2, 0x26, 0xe2, 0x32, 0xf6, 0x2a, 0x2a, 0xa6, 0x6a, 0xea, 0xff
};

unsigned char   cheat_god_seq[] =
{
    0xb2, 0x26, 0x26, 0xaa, 0x26, 0xff
};

unsigned char   cheat_ammo_seq[] =
{
    0xb2, 0x26, 0xf2, 0x66, 0xa2, 0xff
};

unsigned char   cheat_ammonokey_seq[] =
{
    0xb2, 0x26, 0x66, 0xa2, 0xff
};


unsigned char   cheat_noclip_seq[] =
{
    0xb2, 0x26, 0xea, 0x2a, 0xb2,
    0xea, 0x2a, 0xf6, 0x2a, 0x26, 0xff
};

unsigned char   cheat_commercial_noclip_seq[] =
{
    0xb2, 0x26, 0xe2, 0x36, 0xb2, 0x2a, 0xff
};



unsigned char   cheat_powerup_seq[7][10] =
{
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0x6e, 0xff },
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xea, 0xff },
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xb2, 0xff },
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0x6a, 0xff },
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xa2, 0xff },
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0x36, 0xff },
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xff }
};


unsigned char   cheat_clev_seq[] =
{
    0xb2, 0x26,  0xe2, 0x36, 0xa6, 0x6e, 1, 0, 0, 0xff
};


unsigned char   cheat_mypos_seq[] =
{
    0xb2, 0x26, 0xb6, 0xba, 0x2a, 0xf6, 0xea, 0xff
};


cheatseq_t      cheat_mus = { cheat_mus_seq, 0 };
cheatseq_t      cheat_god = { cheat_god_seq, 0 };
cheatseq_t      cheat_ammo = { cheat_ammo_seq, 0 };
cheatseq_t      cheat_ammonokey = { cheat_ammonokey_seq, 0 };
cheatseq_t      cheat_noclip = { cheat_noclip_seq, 0 };
cheatseq_t      cheat_commercial_noclip = { cheat_commercial_noclip_seq, 0 };

cheatseq_t      cheat_powerup[7] =
{
    { cheat_powerup_seq[0], 0 },
    { cheat_powerup_seq[1], 0 },
    { cheat_powerup_seq[2], 0 },
    { cheat_powerup_seq[3], 0 },
    { cheat_powerup_seq[4], 0 },
    { cheat_powerup_seq[5], 0 },
    { cheat_powerup_seq[6], 0 }
};

cheatseq_t      cheat_choppers = { cheat_choppers_seq, 0 };
cheatseq_t      cheat_clev = { cheat_clev_seq, 0 };
cheatseq_t      cheat_mypos = { cheat_mypos_seq, 0 };


extern char*    mapnames[];


void ST_Stop(void);

void ST_refreshBackground(void)
{

    if (st_statusbaron)
    {
        V_DrawPatch(ST_X, 0, BG, sbar);

        if (netgame)
            V_DrawPatch(ST_FX, 0, BG, faceback);

        V_CopyRect(ST_X, 0, BG, ST_WIDTH, ST_HEIGHT, ST_X, ST_Y, FG);
    }

}


boolean
ST_Responder (event_t* ev)
{
  int           i;

  if (ev->type == ev_keyup
      && ((ev->data1 & 0xffff0000) == AM_MSGHEADER))
  {
    switch(ev->data1)
    {
      case AM_MSGENTERED:
        st_gamestate = AutomapState;
        st_firsttime = true;
        break;

      case AM_MSGEXITED:
        st_gamestate = FirstPersonState;
        break;
    }
  }

  else if (ev->type == ev_keydown)
  {
    if (!netgame)
    {

      if (cht_CheckCheat(&cheat_god, ev->data1))
      {
        plyr->cheats ^= CF_GODMODE;
        if (plyr->cheats & CF_GODMODE)
        {
          if (plyr->mo)
            plyr->mo->health = 100;

          plyr->health = 100;
          plyr->message = STSTR_DQDON;
        }
        else
          plyr->message = STSTR_DQDOFF;
      }
      else if (cht_CheckCheat(&cheat_ammonokey, ev->data1))
      {
        plyr->armorpoints = 200;
        plyr->armortype = 2;

        for (i=0;i<NUMWEAPONS;i++)
          plyr->weaponowned[i] = true;

        for (i=0;i<NUMAMMO;i++)
          plyr->ammo[i] = plyr->maxammo[i];

        plyr->message = STSTR_FAADDED;
      }
      else if (cht_CheckCheat(&cheat_ammo, ev->data1))
      {
        plyr->armorpoints = 200;
        plyr->armortype = 2;

        for (i=0;i<NUMWEAPONS;i++)
          plyr->weaponowned[i] = true;

        for (i=0;i<NUMAMMO;i++)
          plyr->ammo[i] = plyr->maxammo[i];

        for (i=0;i<NUMCARDS;i++)
          plyr->cards[i] = true;

        plyr->message = STSTR_KFAADDED;
      }
      else if (cht_CheckCheat(&cheat_mus, ev->data1))
      {

        char    buf[3];
        int             musnum;

        plyr->message = STSTR_MUS;
        cht_GetParam(&cheat_mus, buf);

        if (gamemode == commercial)
        {
          musnum = mus_runnin + (buf[0]-'0')*10 + buf[1]-'0' - 1;

          if (((buf[0]-'0')*10 + buf[1]-'0') > 35)
            plyr->message = STSTR_NOMUS;
          else
            S_ChangeMusic(musnum, 1);
        }
        else
        {
          musnum = mus_e1m1 + (buf[0]-'1')*9 + (buf[1]-'1');

          if (((buf[0]-'1')*9 + buf[1]-'1') > 31)
            plyr->message = STSTR_NOMUS;
          else
            S_ChangeMusic(musnum, 1);
        }
      }
      else if ( cht_CheckCheat(&cheat_noclip, ev->data1)
                || cht_CheckCheat(&cheat_commercial_noclip,ev->data1) )
      {
        plyr->cheats ^= CF_NOCLIP;

        if (plyr->cheats & CF_NOCLIP)
          plyr->message = STSTR_NCON;
        else
          plyr->message = STSTR_NCOFF;
      }
      for (i=0;i<6;i++)
      {
        if (cht_CheckCheat(&cheat_powerup[i], ev->data1))
        {
          if (!plyr->powers[i])
            P_GivePower( plyr, i);
          else if (i!=pw_strength)
            plyr->powers[i] = 1;
          else
            plyr->powers[i] = 0;

          plyr->message = STSTR_BEHOLDX;
        }
      }

      if (cht_CheckCheat(&cheat_powerup[6], ev->data1))
      {
        plyr->message = STSTR_BEHOLD;
      }
      else if (cht_CheckCheat(&cheat_choppers, ev->data1))
      {
        plyr->weaponowned[wp_chainsaw] = true;
        plyr->powers[pw_invulnerability] = true;
        plyr->message = STSTR_CHOPPERS;
      }
      else if (cht_CheckCheat(&cheat_mypos, ev->data1))
      {
        static char     buf[ST_MSGWIDTH];
        sprintf(buf, "ang=0x%x;x,y=(0x%x,0x%x)",
                players[consoleplayer].mo->angle,
                players[consoleplayer].mo->x,
                players[consoleplayer].mo->y);
        plyr->message = buf;
      }
    }

    if (cht_CheckCheat(&cheat_clev, ev->data1))
    {
      char              buf[3];
      int               epsd;
      int               map;

      cht_GetParam(&cheat_clev, buf);

      if (gamemode == commercial)
      {
        epsd = 0;
        map = (buf[0] - '0')*10 + buf[1] - '0';
      }
      else
      {
        epsd = buf[0] - '0';
        map = buf[1] - '0';
      }

      if (epsd < 1)
        return false;

      if (map < 1)
        return false;

      if ((gamemode == retail)
          && ((epsd > 4) || (map > 9)))
        return false;

      if ((gamemode == registered)
          && ((epsd > 3) || (map > 9)))
        return false;

      if ((gamemode == shareware)
          && ((epsd > 1) || (map > 9)))
        return false;

      if ((gamemode == commercial)
        && (( epsd > 1) || (map > 34)))
        return false;

      plyr->message = STSTR_CLEV;
      G_DeferedInitNew(gameskill, epsd, map);
    }
  }
  return false;
}



int ST_calcPainOffset(void)
{
    int         health;
    static int  lastcalc;
    static int  oldhealth = -1;

    health = plyr->health > 100 ? 100 : plyr->health;

    if (health != oldhealth)
    {
        lastcalc = ST_FACESTRIDE * (((100 - health) * ST_NUMPAINFACES) / 101);
        oldhealth = health;
    }
    return lastcalc;
}


void ST_updateFaceWidget(void)
{
    int         i;
    angle_t     badguyangle;
    angle_t     diffang;
    static int  lastattackdown = -1;
    static int  priority = 0;
    boolean     doevilgrin;

    if (priority < 10)
    {
        if (!plyr->health)
        {
            priority = 9;
            st_faceindex = ST_DEADFACE;
            st_facecount = 1;
        }
    }

    if (priority < 9)
    {
        if (plyr->bonuscount)
        {
            doevilgrin = false;

            for (i=0;i<NUMWEAPONS;i++)
            {
                if (oldweaponsowned[i] != plyr->weaponowned[i])
                {
                    doevilgrin = true;
                    oldweaponsowned[i] = plyr->weaponowned[i];
                }
            }
            if (doevilgrin)
            {
                priority = 8;
                st_facecount = ST_EVILGRINCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_EVILGRINOFFSET;
            }
        }

    }

    if (priority < 8)
    {
        if (plyr->damagecount
            && plyr->attacker
            && plyr->attacker != plyr->mo)
        {
            priority = 7;

            if (plyr->health - st_oldhealth > ST_MUCHPAIN)
            {
                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
            }
            else
            {
                badguyangle = R_PointToAngle2(plyr->mo->x,
                                              plyr->mo->y,
                                              plyr->attacker->x,
                                              plyr->attacker->y);

                if (badguyangle > plyr->mo->angle)
                {
                    diffang = badguyangle - plyr->mo->angle;
                    i = diffang > ANG180;
                }
                else
                {
                    diffang = plyr->mo->angle - badguyangle;
                    i = diffang <= ANG180;
                }


                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset();

                if (diffang < ANG45)
                {
                    st_faceindex += ST_RAMPAGEOFFSET;
                }
                else if (i)
                {
                    st_faceindex += ST_TURNOFFSET;
                }
                else
                {
                    st_faceindex += ST_TURNOFFSET+1;
                }
            }
        }
    }

    if (priority < 7)
    {
        if (plyr->damagecount)
        {
            if (plyr->health - st_oldhealth > ST_MUCHPAIN)
            {
                priority = 7;
                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
            }
            else
            {
                priority = 6;
                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
            }

        }

    }

    if (priority < 6)
    {
        if (plyr->attackdown)
        {
            if (lastattackdown==-1)
                lastattackdown = ST_RAMPAGEDELAY;
            else if (!--lastattackdown)
            {
                priority = 5;
                st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
                st_facecount = 1;
                lastattackdown = 1;
            }
        }
        else
            lastattackdown = -1;

    }

    if (priority < 5)
    {
        if ((plyr->cheats & CF_GODMODE)
            || plyr->powers[pw_invulnerability])
        {
            priority = 4;

            st_faceindex = ST_GODFACE;
            st_facecount = 1;

        }

    }

    if (!st_facecount)
    {
        st_faceindex = ST_calcPainOffset() + (st_randomnumber % 3);
        st_facecount = ST_STRAIGHTFACECOUNT;
        priority = 0;
    }

    st_facecount--;

}

void ST_updateWidgets(void)
{
    static int  largeammo = 1994;
    int         i;

    if (weaponinfo[plyr->readyweapon].ammo == am_noammo)
        w_ready.num = &largeammo;
    else
        w_ready.num = &plyr->ammo[weaponinfo[plyr->readyweapon].ammo];
    w_ready.data = plyr->readyweapon;


    for (i=0;i<3;i++)
    {
        keyboxes[i] = plyr->cards[i] ? i : -1;

        if (plyr->cards[i+3])
            keyboxes[i] = i+3;
    }

    ST_updateFaceWidget();

    st_notdeathmatch = !deathmatch;

    st_armson = st_statusbaron && !deathmatch;

    st_fragson = deathmatch && st_statusbaron;
    st_fragscount = 0;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (i != consoleplayer)
            st_fragscount += plyr->frags[i];
        else
            st_fragscount -= plyr->frags[i];
    }

    if (!--st_msgcounter)
        st_chat = st_oldchat;

}

void ST_Ticker (void)
{

    st_clock++;
    st_randomnumber = M_Random();
    ST_updateWidgets();
    st_oldhealth = plyr->health;

}

static int st_palette = 0;

void ST_doPaletteStuff(void)
{

    int         palette;
    byte*       pal;
    int         cnt;
    int         bzc;

    cnt = plyr->damagecount;

    if (plyr->powers[pw_strength])
    {
        bzc = 12 - (plyr->powers[pw_strength]>>6);

        if (bzc > cnt)
            cnt = bzc;
    }

    if (cnt)
    {
        palette = (cnt+7)>>3;

        if (palette >= NUMREDPALS)
            palette = NUMREDPALS-1;

        palette += STARTREDPALS;
    }

    else if (plyr->bonuscount)
    {
        palette = (plyr->bonuscount+7)>>3;

        if (palette >= NUMBONUSPALS)
            palette = NUMBONUSPALS-1;

        palette += STARTBONUSPALS;
    }

    else if ( plyr->powers[pw_ironfeet] > 4*32
              || plyr->powers[pw_ironfeet]&8)
        palette = RADIATIONPAL;
    else
        palette = 0;

    if (palette != st_palette)
    {
        st_palette = palette;
        pal = (byte *) W_CacheLumpNum (lu_palette, PU_CACHE)+palette*768;
        I_SetPalette (pal);
    }

}

void ST_drawWidgets(boolean refresh)
{
    int         i;

    st_armson = st_statusbaron && !deathmatch;

    st_fragson = deathmatch && st_statusbaron;

    STlib_updateNum(&w_ready, refresh);

    for (i=0;i<4;i++)
    {
        STlib_updateNum(&w_ammo[i], refresh);
        STlib_updateNum(&w_maxammo[i], refresh);
    }

    STlib_updatePercent(&w_health, refresh);
    STlib_updatePercent(&w_armor, refresh);

    STlib_updateBinIcon(&w_armsbg, refresh);

    for (i=0;i<6;i++)
        STlib_updateMultIcon(&w_arms[i], refresh);

    STlib_updateMultIcon(&w_faces, refresh);

    for (i=0;i<3;i++)
        STlib_updateMultIcon(&w_keyboxes[i], refresh);

    STlib_updateNum(&w_frags, refresh);

}

void ST_doRefresh(void)
{

    st_firsttime = false;

    ST_refreshBackground();

    ST_drawWidgets(true);

}

void ST_diffDraw(void)
{
    ST_drawWidgets(false);
}

void ST_Drawer (boolean fullscreen, boolean refresh)
{

    st_statusbaron = (!fullscreen) || automapactive;
    st_firsttime = st_firsttime || refresh;

    ST_doPaletteStuff();

    if (st_firsttime) ST_doRefresh();
    else ST_diffDraw();

}

void ST_loadGraphics(void)
{

    int         i;
    int         j;
    int         facenum;

    char        namebuf[9];

    for (i=0;i<10;i++)
    {
        namebuf[0] = 'S'; namebuf[1] = 'T'; namebuf[2] = 'T'; namebuf[3] = 'N';
        namebuf[4] = 'U'; namebuf[5] = 'M'; namebuf[6] = '0' + i; namebuf[7] = '\0';
        tallnum[i] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);

        namebuf[0] = 'S'; namebuf[1] = 'T'; namebuf[2] = 'Y'; namebuf[3] = 'S';
        namebuf[4] = 'N'; namebuf[5] = 'U'; namebuf[6] = 'M'; namebuf[7] = '0' + i; namebuf[8] = '\0';
        shortnum[i] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);
    }

    tallpercent = (patch_t *) W_CacheLumpName("STTPRCNT", PU_STATIC);

    for (i=0;i<NUMCARDS;i++)
    {
        namebuf[0] = 'S'; namebuf[1] = 'T'; namebuf[2] = 'K'; namebuf[3] = 'E';
        namebuf[4] = 'Y'; namebuf[5] = 'S'; namebuf[6] = '0' + i; namebuf[7] = '\0';
        keys[i] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);
    }

    armsbg = (patch_t *) W_CacheLumpName("STARMS", PU_STATIC);

    for (i=0;i<6;i++)
    {
        namebuf[0] = 'S'; namebuf[1] = 'T'; namebuf[2] = 'G'; namebuf[3] = 'N';
        namebuf[4] = 'U'; namebuf[5] = 'M'; namebuf[6] = '0' + (i + 2); namebuf[7] = '\0';

        arms[i][0] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);

        arms[i][1] = shortnum[i+2];
    }

    namebuf[0] = 'S'; namebuf[1] = 'T'; namebuf[2] = 'F'; namebuf[3] = 'B';
    if (consoleplayer < 0 || consoleplayer > 9)
        namebuf[4] = '0';
    else
        namebuf[4] = '0' + consoleplayer;
    namebuf[5] = '\0';
    faceback = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);

    sbar = (patch_t *) W_CacheLumpName("STBAR", PU_STATIC);

    facenum = 0;
    for (i=0;i<ST_NUMPAINFACES;i++)
    {
        for (j=0;j<ST_NUMSTRAIGHTFACES;j++)
        {
            namebuf[0] = 'S'; namebuf[1] = 'T'; namebuf[2] = 'F'; namebuf[3] = 'S';
            namebuf[4] = 'T'; namebuf[5] = '0' + i; namebuf[6] = '0' + j; namebuf[7] = '\0';
            faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
        }
        namebuf[0] = 'S'; namebuf[1] = 'T'; namebuf[2] = 'F'; namebuf[3] = 'T';
        namebuf[4] = 'R'; namebuf[5] = '0' + i; namebuf[6] = '0'; namebuf[7] = '\0';
        faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
        namebuf[0] = 'S'; namebuf[1] = 'T'; namebuf[2] = 'F'; namebuf[3] = 'T';
        namebuf[4] = 'L'; namebuf[5] = '0' + i; namebuf[6] = '0'; namebuf[7] = '\0';
        faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
        namebuf[0] = 'S'; namebuf[1] = 'T'; namebuf[2] = 'F'; namebuf[3] = 'O';
        namebuf[4] = 'U'; namebuf[5] = 'C'; namebuf[6] = 'H'; namebuf[7] = '0' + i; namebuf[8] = '\0';
        faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
        namebuf[0] = 'S'; namebuf[1] = 'T'; namebuf[2] = 'F'; namebuf[3] = 'E';
        namebuf[4] = 'V'; namebuf[5] = 'L'; namebuf[6] = '0' + i; namebuf[7] = '\0';
        faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
        namebuf[0] = 'S'; namebuf[1] = 'T'; namebuf[2] = 'F'; namebuf[3] = 'K';
        namebuf[4] = 'I'; namebuf[5] = 'L'; namebuf[6] = 'L'; namebuf[7] = '0' + i; namebuf[8] = '\0';
        faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
    }
    faces[facenum++] = W_CacheLumpName("STFGOD0", PU_STATIC);
    faces[facenum++] = W_CacheLumpName("STFDEAD0", PU_STATIC);

}

void ST_loadData(void)
{
    lu_palette = W_GetNumForName ("PLAYPAL");
    ST_loadGraphics();
}

void ST_unloadGraphics(void)
{

    int i;

    for (i=0;i<10;i++)
    {
        Z_ChangeTag(tallnum[i], PU_CACHE);
        Z_ChangeTag(shortnum[i], PU_CACHE);
    }
    Z_ChangeTag(tallpercent, PU_CACHE);

    Z_ChangeTag(armsbg, PU_CACHE);

    for (i=0;i<6;i++)
        Z_ChangeTag(arms[i][0], PU_CACHE);

    for (i=0;i<NUMCARDS;i++)
        Z_ChangeTag(keys[i], PU_CACHE);

    Z_ChangeTag(sbar, PU_CACHE);
    Z_ChangeTag(faceback, PU_CACHE);

    for (i=0;i<ST_NUMFACES;i++)
        Z_ChangeTag(faces[i], PU_CACHE);



}

void ST_unloadData(void)
{
    ST_unloadGraphics();
}

void ST_initData(void)
{

    int         i;

    st_firsttime = true;
    plyr = &players[consoleplayer];

    st_clock = 0;
    st_chatstate = StartChatState;
    st_gamestate = FirstPersonState;

    st_statusbaron = true;
    st_oldchat = st_chat = false;
    st_cursoron = false;

    st_faceindex = 0;
    st_palette = -1;

    st_oldhealth = -1;

    for (i=0;i<NUMWEAPONS;i++)
        oldweaponsowned[i] = plyr->weaponowned[i];

    for (i=0;i<3;i++)
        keyboxes[i] = -1;

    STlib_init();

}



void ST_createWidgets(void)
{

    int i;

    STlib_initNum(&w_ready,
                  ST_AMMOX,
                  ST_AMMOY,
                  tallnum,
                  &plyr->ammo[weaponinfo[plyr->readyweapon].ammo],
                  &st_statusbaron,
                  ST_AMMOWIDTH );

    w_ready.data = plyr->readyweapon;

    STlib_initPercent(&w_health,
                      ST_HEALTHX,
                      ST_HEALTHY,
                      tallnum,
                      &plyr->health,
                      &st_statusbaron,
                      tallpercent);

    STlib_initBinIcon(&w_armsbg,
                      ST_ARMSBGX,
                      ST_ARMSBGY,
                      armsbg,
                      &st_notdeathmatch,
                      &st_statusbaron);

    for(i=0;i<6;i++)
    {
        STlib_initMultIcon(&w_arms[i],
                           ST_ARMSX+(i%3)*ST_ARMSXSPACE,
                           ST_ARMSY+(i/3)*ST_ARMSYSPACE,
                           arms[i], (int *) &plyr->weaponowned[i+1],
                           &st_armson);
    }

    STlib_initNum(&w_frags,
                  ST_FRAGSX,
                  ST_FRAGSY,
                  tallnum,
                  &st_fragscount,
                  &st_fragson,
                  ST_FRAGSWIDTH);

    STlib_initMultIcon(&w_faces,
                       ST_FACESX,
                       ST_FACESY,
                       faces,
                       &st_faceindex,
                       &st_statusbaron);

    STlib_initPercent(&w_armor,
                      ST_ARMORX,
                      ST_ARMORY,
                      tallnum,
                      &plyr->armorpoints,
                      &st_statusbaron, tallpercent);

    STlib_initMultIcon(&w_keyboxes[0],
                       ST_KEY0X,
                       ST_KEY0Y,
                       keys,
                       &keyboxes[0],
                       &st_statusbaron);

    STlib_initMultIcon(&w_keyboxes[1],
                       ST_KEY1X,
                       ST_KEY1Y,
                       keys,
                       &keyboxes[1],
                       &st_statusbaron);

    STlib_initMultIcon(&w_keyboxes[2],
                       ST_KEY2X,
                       ST_KEY2Y,
                       keys,
                       &keyboxes[2],
                       &st_statusbaron);

    STlib_initNum(&w_ammo[0],
                  ST_AMMO0X,
                  ST_AMMO0Y,
                  shortnum,
                  &plyr->ammo[0],
                  &st_statusbaron,
                  ST_AMMO0WIDTH);

    STlib_initNum(&w_ammo[1],
                  ST_AMMO1X,
                  ST_AMMO1Y,
                  shortnum,
                  &plyr->ammo[1],
                  &st_statusbaron,
                  ST_AMMO1WIDTH);

    STlib_initNum(&w_ammo[2],
                  ST_AMMO2X,
                  ST_AMMO2Y,
                  shortnum,
                  &plyr->ammo[2],
                  &st_statusbaron,
                  ST_AMMO2WIDTH);

    STlib_initNum(&w_ammo[3],
                  ST_AMMO3X,
                  ST_AMMO3Y,
                  shortnum,
                  &plyr->ammo[3],
                  &st_statusbaron,
                  ST_AMMO3WIDTH);

    STlib_initNum(&w_maxammo[0],
                  ST_MAXAMMO0X,
                  ST_MAXAMMO0Y,
                  shortnum,
                  &plyr->maxammo[0],
                  &st_statusbaron,
                  ST_MAXAMMO0WIDTH);

    STlib_initNum(&w_maxammo[1],
                  ST_MAXAMMO1X,
                  ST_MAXAMMO1Y,
                  shortnum,
                  &plyr->maxammo[1],
                  &st_statusbaron,
                  ST_MAXAMMO1WIDTH);

    STlib_initNum(&w_maxammo[2],
                  ST_MAXAMMO2X,
                  ST_MAXAMMO2Y,
                  shortnum,
                  &plyr->maxammo[2],
                  &st_statusbaron,
                  ST_MAXAMMO2WIDTH);

    STlib_initNum(&w_maxammo[3],
                  ST_MAXAMMO3X,
                  ST_MAXAMMO3Y,
                  shortnum,
                  &plyr->maxammo[3],
                  &st_statusbaron,
                  ST_MAXAMMO3WIDTH);

}

static boolean  st_stopped = true;


void ST_Start (void)
{

    if (!st_stopped)
        ST_Stop();

    ST_initData();
    ST_createWidgets();
    st_stopped = false;

}

void ST_Stop (void)
{
    if (st_stopped)
        return;

    I_SetPalette (W_CacheLumpNum (lu_palette, PU_CACHE));

    st_stopped = true;
}

void ST_Init (void)
{
    veryfirsttime = 0;
    ST_loadData();
    screens[4] = (byte *) Z_Malloc(ST_WIDTH*ST_HEIGHT, PU_STATIC, 0);
}
