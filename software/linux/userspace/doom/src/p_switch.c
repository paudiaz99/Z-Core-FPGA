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
// $Log:$
//
// DESCRIPTION:
//      Switches, buttons. Two-state animation. Exits.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: p_switch.c,v 1.3 1997/01/28 22:08:29 b1 Exp $";


#include "i_system.h"
#include "doomdef.h"
#include "p_local.h"

#include "g_game.h"

#include "s_sound.h"

#include "sounds.h"

#include "doomstat.h"
#include "r_state.h"


switchlist_t alphSwitchList[] =
{
    {"SW1BRCOM",        "SW2BRCOM",     1},
    {"SW1BRN1", "SW2BRN1",      1},
    {"SW1BRN2", "SW2BRN2",      1},
    {"SW1BRNGN",        "SW2BRNGN",     1},
    {"SW1BROWN",        "SW2BROWN",     1},
    {"SW1COMM", "SW2COMM",      1},
    {"SW1COMP", "SW2COMP",      1},
    {"SW1DIRT", "SW2DIRT",      1},
    {"SW1EXIT", "SW2EXIT",      1},
    {"SW1GRAY", "SW2GRAY",      1},
    {"SW1GRAY1",        "SW2GRAY1",     1},
    {"SW1METAL",        "SW2METAL",     1},
    {"SW1PIPE", "SW2PIPE",      1},
    {"SW1SLAD", "SW2SLAD",      1},
    {"SW1STARG",        "SW2STARG",     1},
    {"SW1STON1",        "SW2STON1",     1},
    {"SW1STON2",        "SW2STON2",     1},
    {"SW1STONE",        "SW2STONE",     1},
    {"SW1STRTN",        "SW2STRTN",     1},

    {"SW1BLUE", "SW2BLUE",      2},
    {"SW1CMT",          "SW2CMT",       2},
    {"SW1GARG", "SW2GARG",      2},
    {"SW1GSTON",        "SW2GSTON",     2},
    {"SW1HOT",          "SW2HOT",       2},
    {"SW1LION", "SW2LION",      2},
    {"SW1SATYR",        "SW2SATYR",     2},
    {"SW1SKIN", "SW2SKIN",      2},
    {"SW1VINE", "SW2VINE",      2},
    {"SW1WOOD", "SW2WOOD",      2},

    {"SW1PANEL",        "SW2PANEL",     3},
    {"SW1ROCK", "SW2ROCK",      3},
    {"SW1MET2", "SW2MET2",      3},
    {"SW1WDMET",        "SW2WDMET",     3},
    {"SW1BRIK", "SW2BRIK",      3},
    {"SW1MOD1", "SW2MOD1",      3},
    {"SW1ZIM",          "SW2ZIM",       3},
    {"SW1STON6",        "SW2STON6",     3},
    {"SW1TEK",          "SW2TEK",       3},
    {"SW1MARB", "SW2MARB",      3},
    {"SW1SKULL",        "SW2SKULL",     3},

    {"\0",              "\0",           0}
};

int             switchlist[MAXSWITCHES * 2];
int             numswitches;
button_t        buttonlist[MAXBUTTONS];

void P_InitSwitchList(void)
{
    int         i;
    int         index;
    int         episode;

    episode = 1;

    if (gamemode == registered)
        episode = 2;
    else
        if ( gamemode == commercial )
            episode = 3;

    for (index = 0,i = 0;i < MAXSWITCHES;i++)
    {
        if (!alphSwitchList[i].episode)
        {
            numswitches = index/2;
            switchlist[index] = -1;
            break;
        }

        if (alphSwitchList[i].episode <= episode)
        {
#if 0
            int         value;

            if (R_CheckTextureNumForName(alphSwitchList[i].name1) < 0)
            {
                I_Error("Can't find switch texture '%s'!",
                        alphSwitchList[i].name1);
                continue;
            }

            value = R_TextureNumForName(alphSwitchList[i].name1);
#endif
            switchlist[index++] = R_TextureNumForName(alphSwitchList[i].name1);
            switchlist[index++] = R_TextureNumForName(alphSwitchList[i].name2);
        }
    }
}


void
P_StartButton
( line_t*       line,
  bwhere_e      w,
  int           texture,
  int           time )
{
    int         i;

    for (i = 0;i < MAXBUTTONS;i++)
    {
        if (buttonlist[i].btimer
            && buttonlist[i].line == line)
        {

            return;
        }
    }



    for (i = 0;i < MAXBUTTONS;i++)
    {
        if (!buttonlist[i].btimer)
        {
            buttonlist[i].line = line;
            buttonlist[i].where = w;
            buttonlist[i].btexture = texture;
            buttonlist[i].btimer = time;
            buttonlist[i].soundorg = (mobj_t *)&line->frontsector->soundorg;
            return;
        }
    }

    I_Error("P_StartButton: no button slots left!");
}





void
P_ChangeSwitchTexture
( line_t*       line,
  int           useAgain )
{
    int     texTop;
    int     texMid;
    int     texBot;
    int     i;
    int     sound;

    if (!useAgain)
        line->special = 0;

    texTop = sides[line->sidenum[0]].toptexture;
    texMid = sides[line->sidenum[0]].midtexture;
    texBot = sides[line->sidenum[0]].bottomtexture;

    sound = sfx_swtchn;

    if (line->special == 11)
        sound = sfx_swtchx;

    for (i = 0;i < numswitches*2;i++)
    {
        if (switchlist[i] == texTop)
        {
            S_StartSound(buttonlist->soundorg,sound);
            sides[line->sidenum[0]].toptexture = switchlist[i^1];

            if (useAgain)
                P_StartButton(line,top,switchlist[i],BUTTONTIME);

            return;
        }
        else
        {
            if (switchlist[i] == texMid)
            {
                S_StartSound(buttonlist->soundorg,sound);
                sides[line->sidenum[0]].midtexture = switchlist[i^1];

                if (useAgain)
                    P_StartButton(line, middle,switchlist[i],BUTTONTIME);

                return;
            }
            else
            {
                if (switchlist[i] == texBot)
                {
                    S_StartSound(buttonlist->soundorg,sound);
                    sides[line->sidenum[0]].bottomtexture = switchlist[i^1];

                    if (useAgain)
                        P_StartButton(line, bottom,switchlist[i],BUTTONTIME);

                    return;
                }
            }
        }
    }
}






boolean
P_UseSpecialLine
( mobj_t*       thing,
  line_t*       line,
  int           side )
{

    if (side)
    {
        switch(line->special)
        {
          case 124:
            break;

          default:
            return false;
            break;
        }
    }


    if (!thing->player)
    {
        if (line->flags & ML_SECRET)
            return false;

        switch(line->special)
        {
          case 1:
          case 32:
          case 33:
          case 34:
            break;

          default:
            return false;
            break;
        }
    }


    switch (line->special)
    {
      case 1:
      case 26:
      case 27:
      case 28:

      case 31:
      case 32:
      case 33:
      case 34:

      case 117:
      case 118:
        EV_VerticalDoor (line, thing);
        break;


      case 7:
        if (EV_BuildStairs(line,build8))
            P_ChangeSwitchTexture(line,0);
        break;

      case 9:
        if (EV_DoDonut(line))
            P_ChangeSwitchTexture(line,0);
        break;

      case 11:
        P_ChangeSwitchTexture(line,0);
        G_ExitLevel ();
        break;

      case 14:
        if (EV_DoPlat(line,raiseAndChange,32))
            P_ChangeSwitchTexture(line,0);
        break;

      case 15:
        if (EV_DoPlat(line,raiseAndChange,24))
            P_ChangeSwitchTexture(line,0);
        break;

      case 18:
        if (EV_DoFloor(line, raiseFloorToNearest))
            P_ChangeSwitchTexture(line,0);
        break;

      case 20:
        if (EV_DoPlat(line,raiseToNearestAndChange,0))
            P_ChangeSwitchTexture(line,0);
        break;

      case 21:
        if (EV_DoPlat(line,downWaitUpStay,0))
            P_ChangeSwitchTexture(line,0);
        break;

      case 23:
        if (EV_DoFloor(line,lowerFloorToLowest))
            P_ChangeSwitchTexture(line,0);
        break;

      case 29:
        if (EV_DoDoor(line,normal))
            P_ChangeSwitchTexture(line,0);
        break;

      case 41:
        if (EV_DoCeiling(line,lowerToFloor))
            P_ChangeSwitchTexture(line,0);
        break;

      case 71:
        if (EV_DoFloor(line,turboLower))
            P_ChangeSwitchTexture(line,0);
        break;

      case 49:
        if (EV_DoCeiling(line,crushAndRaise))
            P_ChangeSwitchTexture(line,0);
        break;

      case 50:
        if (EV_DoDoor(line,close))
            P_ChangeSwitchTexture(line,0);
        break;

      case 51:
        P_ChangeSwitchTexture(line,0);
        G_SecretExitLevel ();
        break;

      case 55:
        if (EV_DoFloor(line,raiseFloorCrush))
            P_ChangeSwitchTexture(line,0);
        break;

      case 101:
        if (EV_DoFloor(line,raiseFloor))
            P_ChangeSwitchTexture(line,0);
        break;

      case 102:
        if (EV_DoFloor(line,lowerFloor))
            P_ChangeSwitchTexture(line,0);
        break;

      case 103:
        if (EV_DoDoor(line,open))
            P_ChangeSwitchTexture(line,0);
        break;

      case 111:
        if (EV_DoDoor (line,blazeRaise))
            P_ChangeSwitchTexture(line,0);
        break;

      case 112:
        if (EV_DoDoor (line,blazeOpen))
            P_ChangeSwitchTexture(line,0);
        break;

      case 113:
        if (EV_DoDoor (line,blazeClose))
            P_ChangeSwitchTexture(line,0);
        break;

      case 122:
        if (EV_DoPlat(line,blazeDWUS,0))
            P_ChangeSwitchTexture(line,0);
        break;

      case 127:
        if (EV_BuildStairs(line,turbo16))
            P_ChangeSwitchTexture(line,0);
        break;

      case 131:
        if (EV_DoFloor(line,raiseFloorTurbo))
            P_ChangeSwitchTexture(line,0);
        break;

      case 133:
      case 135:
      case 137:
        if (EV_DoLockedDoor (line,blazeOpen,thing))
            P_ChangeSwitchTexture(line,0);
        break;

      case 140:
        if (EV_DoFloor(line,raiseFloor512))
            P_ChangeSwitchTexture(line,0);
        break;

      case 42:
        if (EV_DoDoor(line,close))
            P_ChangeSwitchTexture(line,1);
        break;

      case 43:
        if (EV_DoCeiling(line,lowerToFloor))
            P_ChangeSwitchTexture(line,1);
        break;

      case 45:
        if (EV_DoFloor(line,lowerFloor))
            P_ChangeSwitchTexture(line,1);
        break;

      case 60:
        if (EV_DoFloor(line,lowerFloorToLowest))
            P_ChangeSwitchTexture(line,1);
        break;

      case 61:
        if (EV_DoDoor(line,open))
            P_ChangeSwitchTexture(line,1);
        break;

      case 62:
        if (EV_DoPlat(line,downWaitUpStay,1))
            P_ChangeSwitchTexture(line,1);
        break;

      case 63:
        if (EV_DoDoor(line,normal))
            P_ChangeSwitchTexture(line,1);
        break;

      case 64:
        if (EV_DoFloor(line,raiseFloor))
            P_ChangeSwitchTexture(line,1);
        break;

      case 66:
        if (EV_DoPlat(line,raiseAndChange,24))
            P_ChangeSwitchTexture(line,1);
        break;

      case 67:
        if (EV_DoPlat(line,raiseAndChange,32))
            P_ChangeSwitchTexture(line,1);
        break;

      case 65:
        if (EV_DoFloor(line,raiseFloorCrush))
            P_ChangeSwitchTexture(line,1);
        break;

      case 68:
        if (EV_DoPlat(line,raiseToNearestAndChange,0))
            P_ChangeSwitchTexture(line,1);
        break;

      case 69:
        if (EV_DoFloor(line, raiseFloorToNearest))
            P_ChangeSwitchTexture(line,1);
        break;

      case 70:
        if (EV_DoFloor(line,turboLower))
            P_ChangeSwitchTexture(line,1);
        break;

      case 114:
        if (EV_DoDoor (line,blazeRaise))
            P_ChangeSwitchTexture(line,1);
        break;

      case 115:
        if (EV_DoDoor (line,blazeOpen))
            P_ChangeSwitchTexture(line,1);
        break;

      case 116:
        if (EV_DoDoor (line,blazeClose))
            P_ChangeSwitchTexture(line,1);
        break;

      case 123:
        if (EV_DoPlat(line,blazeDWUS,0))
            P_ChangeSwitchTexture(line,1);
        break;

      case 132:
        if (EV_DoFloor(line,raiseFloorTurbo))
            P_ChangeSwitchTexture(line,1);
        break;

      case 99:
      case 134:
      case 136:
        if (EV_DoLockedDoor (line,blazeOpen,thing))
            P_ChangeSwitchTexture(line,1);
        break;

      case 138:
        EV_LightTurnOn(line,255);
        P_ChangeSwitchTexture(line,1);
        break;

      case 139:
        EV_LightTurnOn(line,35);
        P_ChangeSwitchTexture(line,1);
        break;

    }

    return true;
}

