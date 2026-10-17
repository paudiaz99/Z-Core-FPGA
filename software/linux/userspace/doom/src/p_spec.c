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
//      Implements special effects:
//      Texture animation, height or lighting changes
//       according to adjacent sectors, respective
//       utility functions, etc.
//      Line Tag handling. Line and Sector triggers.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: p_spec.c,v 1.6 1997/02/03 22:45:12 b1 Exp $";


#include <stdlib.h>

#include "doomdef.h"
#include "doomstat.h"

#include "i_system.h"
#include "z_zone.h"
#include "m_argv.h"
#include "m_random.h"
#include "w_wad.h"

#include "r_local.h"
#include "p_local.h"

#include "g_game.h"

#include "s_sound.h"

#include "r_state.h"

#include "sounds.h"


typedef struct
{
    boolean     istexture;
    int         picnum;
    int         basepic;
    int         numpics;
    int         speed;

} anim_t;

typedef struct
{
    boolean     istexture;
    char        endname[9];
    char        startname[9];
    int         speed;
} animdef_t;



#define MAXANIMS                32

extern anim_t   anims[MAXANIMS];
extern anim_t*  lastanim;


animdef_t               animdefs[] =
{
    {false,     "NUKAGE3",      "NUKAGE1",      8},
    {false,     "FWATER4",      "FWATER1",      8},
    {false,     "SWATER4",      "SWATER1",      8},
    {false,     "LAVA4",        "LAVA1",        8},
    {false,     "BLOOD3",       "BLOOD1",       8},

    {false,     "RROCK08",      "RROCK05",      8},
    {false,     "SLIME04",      "SLIME01",      8},
    {false,     "SLIME08",      "SLIME05",      8},
    {false,     "SLIME12",      "SLIME09",      8},

    {true,      "BLODGR4",      "BLODGR1",      8},
    {true,      "SLADRIP3",     "SLADRIP1",     8},

    {true,      "BLODRIP4",     "BLODRIP1",     8},
    {true,      "FIREWALL",     "FIREWALA",     8},
    {true,      "GSTFONT3",     "GSTFONT1",     8},
    {true,      "FIRELAVA",     "FIRELAV3",     8},
    {true,      "FIREMAG3",     "FIREMAG1",     8},
    {true,      "FIREBLU2",     "FIREBLU1",     8},
    {true,      "ROCKRED3",     "ROCKRED1",     8},

    {true,      "BFALL4",       "BFALL1",       8},
    {true,      "SFALL4",       "SFALL1",       8},
    {true,      "WFALL4",       "WFALL1",       8},
    {true,      "DBRAIN4",      "DBRAIN1",      8},

    {-1}
};

anim_t          anims[MAXANIMS];
anim_t*         lastanim;


#define MAXLINEANIMS            64

extern  short   numlinespecials;
extern  line_t* linespeciallist[MAXLINEANIMS];



void P_InitPicAnims (void)
{
    int         i;


    lastanim = anims;
    for (i=0 ; animdefs[i].istexture != -1 ; i++)
    {
        if (animdefs[i].istexture)
        {
            if (R_CheckTextureNumForName(animdefs[i].startname) == -1)
                continue;

            lastanim->picnum = R_TextureNumForName (animdefs[i].endname);
            lastanim->basepic = R_TextureNumForName (animdefs[i].startname);
        }
        else
        {
            if (W_CheckNumForName(animdefs[i].startname) == -1)
                continue;

            lastanim->picnum = R_FlatNumForName (animdefs[i].endname);
            lastanim->basepic = R_FlatNumForName (animdefs[i].startname);
        }

        lastanim->istexture = animdefs[i].istexture;
        lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;

        if (lastanim->numpics < 2)
            I_Error ("P_InitPicAnims: bad cycle from %s to %s",
                     animdefs[i].startname,
                     animdefs[i].endname);

        lastanim->speed = animdefs[i].speed;
        lastanim++;
    }

}






side_t*
getSide
( int           currentSector,
  int           line,
  int           side )
{
    return &sides[ (sectors[currentSector].lines[line])->sidenum[side] ];
}


sector_t*
getSector
( int           currentSector,
  int           line,
  int           side )
{
    return sides[ (sectors[currentSector].lines[line])->sidenum[side] ].sector;
}


int
twoSided
( int   sector,
  int   line )
{
    return (sectors[sector].lines[line])->flags & ML_TWOSIDED;
}




sector_t*
getNextSector
( line_t*       line,
  sector_t*     sec )
{
    if (!(line->flags & ML_TWOSIDED))
        return NULL;

    if (line->frontsector == sec)
        return line->backsector;

    return line->frontsector;
}



fixed_t P_FindLowestFloorSurrounding(sector_t* sec)
{
    int                 i;
    line_t*             check;
    sector_t*           other;
    fixed_t             floor = sec->floorheight;

    for (i=0 ;i < sec->linecount ; i++)
    {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;

        if (other->floorheight < floor)
            floor = other->floorheight;
    }
    return floor;
}



fixed_t P_FindHighestFloorSurrounding(sector_t *sec)
{
    int                 i;
    line_t*             check;
    sector_t*           other;
    fixed_t             floor = -500*FRACUNIT;

    for (i=0 ;i < sec->linecount ; i++)
    {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;

        if (other->floorheight > floor)
            floor = other->floorheight;
    }
    return floor;
}




#define MAX_ADJOINING_SECTORS           20

fixed_t
P_FindNextHighestFloor
( sector_t*     sec,
  int           currentheight )
{
    int                 i;
    int                 h;
    int                 min;
    line_t*             check;
    sector_t*           other;
    fixed_t             height = currentheight;


    fixed_t             heightlist[MAX_ADJOINING_SECTORS];

    for (i=0, h=0 ;i < sec->linecount ; i++)
    {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;

        if (other->floorheight > height)
            heightlist[h++] = other->floorheight;

        if ( h >= MAX_ADJOINING_SECTORS )
        {
            fprintf( stderr,
                     "Sector with more than 20 adjoining sectors\n" );
            break;
        }
    }

    if (!h)
        return currentheight;

    min = heightlist[0];

    for (i = 1;i < h;i++)
        if (heightlist[i] < min)
            min = heightlist[i];

    return min;
}


fixed_t
P_FindLowestCeilingSurrounding(sector_t* sec)
{
    int                 i;
    line_t*             check;
    sector_t*           other;
    fixed_t             height = MAXINT;

    for (i=0 ;i < sec->linecount ; i++)
    {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;

        if (other->ceilingheight < height)
            height = other->ceilingheight;
    }
    return height;
}


fixed_t P_FindHighestCeilingSurrounding(sector_t* sec)
{
    int         i;
    line_t*     check;
    sector_t*   other;
    fixed_t     height = 0;

    for (i=0 ;i < sec->linecount ; i++)
    {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;

        if (other->ceilingheight > height)
            height = other->ceilingheight;
    }
    return height;
}



int
P_FindSectorFromLineTag
( line_t*       line,
  int           start )
{
    int i;

    for (i=start+1;i<numsectors;i++)
        if (sectors[i].tag == line->tag)
            return i;

    return -1;
}




int
P_FindMinSurroundingLight
( sector_t*     sector,
  int           max )
{
    int         i;
    int         min;
    line_t*     line;
    sector_t*   check;

    min = max;
    for (i=0 ; i < sector->linecount ; i++)
    {
        line = sector->lines[i];
        check = getNextSector(line,sector);

        if (!check)
            continue;

        if (check->lightlevel < min)
            min = check->lightlevel;
    }
    return min;
}




void
P_CrossSpecialLine
( int           linenum,
  int           side,
  mobj_t*       thing )
{
    line_t*     line;
    int         ok;

    line = &lines[linenum];

    if (!thing->player)
    {
        switch(thing->type)
        {
          case MT_ROCKET:
          case MT_PLASMA:
          case MT_BFG:
          case MT_TROOPSHOT:
          case MT_HEADSHOT:
          case MT_BRUISERSHOT:
            return;
            break;

          default: break;
        }

        ok = 0;
        switch(line->special)
        {
          case 39:
          case 97:
          case 125:
          case 126:
          case 4:
          case 10:
          case 88:
            ok = 1;
            break;
        }
        if (!ok)
            return;
    }


    switch (line->special)
    {
      case 2:
        EV_DoDoor(line,open);
        line->special = 0;
        break;

      case 3:
        EV_DoDoor(line,close);
        line->special = 0;
        break;

      case 4:
        EV_DoDoor(line,normal);
        line->special = 0;
        break;

      case 5:
        EV_DoFloor(line,raiseFloor);
        line->special = 0;
        break;

      case 6:
        EV_DoCeiling(line,fastCrushAndRaise);
        line->special = 0;
        break;

      case 8:
        EV_BuildStairs(line,build8);
        line->special = 0;
        break;

      case 10:
        EV_DoPlat(line,downWaitUpStay,0);
        line->special = 0;
        break;

      case 12:
        EV_LightTurnOn(line,0);
        line->special = 0;
        break;

      case 13:
        EV_LightTurnOn(line,255);
        line->special = 0;
        break;

      case 16:
        EV_DoDoor(line,close30ThenOpen);
        line->special = 0;
        break;

      case 17:
        EV_StartLightStrobing(line);
        line->special = 0;
        break;

      case 19:
        EV_DoFloor(line,lowerFloor);
        line->special = 0;
        break;

      case 22:
        EV_DoPlat(line,raiseToNearestAndChange,0);
        line->special = 0;
        break;

      case 25:
        EV_DoCeiling(line,crushAndRaise);
        line->special = 0;
        break;

      case 30:
        EV_DoFloor(line,raiseToTexture);
        line->special = 0;
        break;

      case 35:
        EV_LightTurnOn(line,35);
        line->special = 0;
        break;

      case 36:
        EV_DoFloor(line,turboLower);
        line->special = 0;
        break;

      case 37:
        EV_DoFloor(line,lowerAndChange);
        line->special = 0;
        break;

      case 38:
        EV_DoFloor( line, lowerFloorToLowest );
        line->special = 0;
        break;

      case 39:
        EV_Teleport( line, side, thing );
        line->special = 0;
        break;

      case 40:
        EV_DoCeiling( line, raiseToHighest );
        EV_DoFloor( line, lowerFloorToLowest );
        line->special = 0;
        break;

      case 44:
        EV_DoCeiling( line, lowerAndCrush );
        line->special = 0;
        break;

      case 52:
        G_ExitLevel ();
        break;

      case 53:
        EV_DoPlat(line,perpetualRaise,0);
        line->special = 0;
        break;

      case 54:
        EV_StopPlat(line);
        line->special = 0;
        break;

      case 56:
        EV_DoFloor(line,raiseFloorCrush);
        line->special = 0;
        break;

      case 57:
        EV_CeilingCrushStop(line);
        line->special = 0;
        break;

      case 58:
        EV_DoFloor(line,raiseFloor24);
        line->special = 0;
        break;

      case 59:
        EV_DoFloor(line,raiseFloor24AndChange);
        line->special = 0;
        break;

      case 104:
        EV_TurnTagLightsOff(line);
        line->special = 0;
        break;

      case 108:
        EV_DoDoor (line,blazeRaise);
        line->special = 0;
        break;

      case 109:
        EV_DoDoor (line,blazeOpen);
        line->special = 0;
        break;

      case 100:
        EV_BuildStairs(line,turbo16);
        line->special = 0;
        break;

      case 110:
        EV_DoDoor (line,blazeClose);
        line->special = 0;
        break;

      case 119:
        EV_DoFloor(line,raiseFloorToNearest);
        line->special = 0;
        break;

      case 121:
        EV_DoPlat(line,blazeDWUS,0);
        line->special = 0;
        break;

      case 124:
        G_SecretExitLevel ();
        break;

      case 125:
        if (!thing->player)
        {
            EV_Teleport( line, side, thing );
            line->special = 0;
        }
        break;

      case 130:
        EV_DoFloor(line,raiseFloorTurbo);
        line->special = 0;
        break;

      case 141:
        EV_DoCeiling(line,silentCrushAndRaise);
        line->special = 0;
        break;

      case 72:
        EV_DoCeiling( line, lowerAndCrush );
        break;

      case 73:
        EV_DoCeiling(line,crushAndRaise);
        break;

      case 74:
        EV_CeilingCrushStop(line);
        break;

      case 75:
        EV_DoDoor(line,close);
        break;

      case 76:
        EV_DoDoor(line,close30ThenOpen);
        break;

      case 77:
        EV_DoCeiling(line,fastCrushAndRaise);
        break;

      case 79:
        EV_LightTurnOn(line,35);
        break;

      case 80:
        EV_LightTurnOn(line,0);
        break;

      case 81:
        EV_LightTurnOn(line,255);
        break;

      case 82:
        EV_DoFloor( line, lowerFloorToLowest );
        break;

      case 83:
        EV_DoFloor(line,lowerFloor);
        break;

      case 84:
        EV_DoFloor(line,lowerAndChange);
        break;

      case 86:
        EV_DoDoor(line,open);
        break;

      case 87:
        EV_DoPlat(line,perpetualRaise,0);
        break;

      case 88:
        EV_DoPlat(line,downWaitUpStay,0);
        break;

      case 89:
        EV_StopPlat(line);
        break;

      case 90:
        EV_DoDoor(line,normal);
        break;

      case 91:
        EV_DoFloor(line,raiseFloor);
        break;

      case 92:
        EV_DoFloor(line,raiseFloor24);
        break;

      case 93:
        EV_DoFloor(line,raiseFloor24AndChange);
        break;

      case 94:
        EV_DoFloor(line,raiseFloorCrush);
        break;

      case 95:
        EV_DoPlat(line,raiseToNearestAndChange,0);
        break;

      case 96:
        EV_DoFloor(line,raiseToTexture);
        break;

      case 97:
        EV_Teleport( line, side, thing );
        break;

      case 98:
        EV_DoFloor(line,turboLower);
        break;

      case 105:
        EV_DoDoor (line,blazeRaise);
        break;

      case 106:
        EV_DoDoor (line,blazeOpen);
        break;

      case 107:
        EV_DoDoor (line,blazeClose);
        break;

      case 120:
        EV_DoPlat(line,blazeDWUS,0);
        break;

      case 126:
        if (!thing->player)
            EV_Teleport( line, side, thing );
        break;

      case 128:
        EV_DoFloor(line,raiseFloorToNearest);
        break;

      case 129:
        EV_DoFloor(line,raiseFloorTurbo);
        break;
    }
}



void
P_ShootSpecialLine
( mobj_t*       thing,
  line_t*       line )
{
    int         ok;

    if (!thing->player)
    {
        ok = 0;
        switch(line->special)
        {
          case 46:
            ok = 1;
            break;
        }
        if (!ok)
            return;
    }

    switch(line->special)
    {
      case 24:
        EV_DoFloor(line,raiseFloor);
        P_ChangeSwitchTexture(line,0);
        break;

      case 46:
        EV_DoDoor(line,open);
        P_ChangeSwitchTexture(line,1);
        break;

      case 47:
        EV_DoPlat(line,raiseToNearestAndChange,0);
        P_ChangeSwitchTexture(line,0);
        break;
    }
}



void P_PlayerInSpecialSector (player_t* player)
{
    sector_t*   sector;

    sector = player->mo->subsector->sector;

    if (player->mo->z != sector->floorheight)
        return;

    switch (sector->special)
    {
      case 5:
        if (!player->powers[pw_ironfeet])
            if (!(leveltime&0x1f))
                P_DamageMobj (player->mo, NULL, NULL, 10);
        break;

      case 7:
        if (!player->powers[pw_ironfeet])
            if (!(leveltime&0x1f))
                P_DamageMobj (player->mo, NULL, NULL, 5);
        break;

      case 16:
      case 4:
        if (!player->powers[pw_ironfeet]
            || (P_Random()<5) )
        {
            if (!(leveltime&0x1f))
                P_DamageMobj (player->mo, NULL, NULL, 20);
        }
        break;

      case 9:
        player->secretcount++;
        sector->special = 0;
        break;

      case 11:
        player->cheats &= ~CF_GODMODE;

        if (!(leveltime&0x1f))
            P_DamageMobj (player->mo, NULL, NULL, 20);

        if (player->health <= 10)
            G_ExitLevel();
        break;

      default:
        I_Error ("P_PlayerInSpecialSector: "
                 "unknown special %i",
                 sector->special);
        break;
    };
}




boolean         levelTimer;
int             levelTimeCount;

void P_UpdateSpecials (void)
{
    anim_t*     anim;
    int         pic;
    int         i;
    line_t*     line;


    if (levelTimer == true)
    {
        levelTimeCount--;
        if (!levelTimeCount)
            G_ExitLevel();
    }

    for (anim = anims ; anim < lastanim ; anim++)
    {
        for (i=anim->basepic ; i<anim->basepic+anim->numpics ; i++)
        {
            pic = anim->basepic + ( (leveltime/anim->speed + i)%anim->numpics );
            if (anim->istexture)
                texturetranslation[i] = pic;
            else
                flattranslation[i] = pic;
        }
    }


    for (i = 0; i < numlinespecials; i++)
    {
        line = linespeciallist[i];
        switch(line->special)
        {
          case 48:
            sides[line->sidenum[0]].textureoffset += FRACUNIT;
            break;
        }
    }


    for (i = 0; i < MAXBUTTONS; i++)
        if (buttonlist[i].btimer)
        {
            buttonlist[i].btimer--;
            if (!buttonlist[i].btimer)
            {
                switch(buttonlist[i].where)
                {
                  case top:
                    sides[buttonlist[i].line->sidenum[0]].toptexture =
                        buttonlist[i].btexture;
                    break;

                  case middle:
                    sides[buttonlist[i].line->sidenum[0]].midtexture =
                        buttonlist[i].btexture;
                    break;

                  case bottom:
                    sides[buttonlist[i].line->sidenum[0]].bottomtexture =
                        buttonlist[i].btexture;
                    break;
                }
                S_StartSound((mobj_t *)&buttonlist[i].soundorg,sfx_swtchn);
                memset(&buttonlist[i],0,sizeof(button_t));
            }
        }

}



int EV_DoDonut(line_t*  line)
{
    sector_t*           s1;
    sector_t*           s2;
    sector_t*           s3;
    int                 secnum;
    int                 rtn;
    int                 i;
    floormove_t*        floor;

    secnum = -1;
    rtn = 0;
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        s1 = &sectors[secnum];

        if (s1->specialdata)
            continue;

        rtn = 1;
        s2 = getNextSector(s1->lines[0],s1);
        for (i = 0;i < s2->linecount;i++)
        {
            if ((!(s2->lines[i]->flags & ML_TWOSIDED)) ||
                (s2->lines[i]->backsector == s1))
                continue;
            s3 = s2->lines[i]->backsector;

            floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
            P_AddThinker (&floor->thinker);
            s2->specialdata = floor;
            floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
            floor->type = donutRaise;
            floor->crush = false;
            floor->direction = 1;
            floor->sector = s2;
            floor->speed = FLOORSPEED / 2;
            floor->texture = s3->floorpic;
            floor->newspecial = 0;
            floor->floordestheight = s3->floorheight;

            floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
            P_AddThinker (&floor->thinker);
            s1->specialdata = floor;
            floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
            floor->type = lowerFloor;
            floor->crush = false;
            floor->direction = -1;
            floor->sector = s1;
            floor->speed = FLOORSPEED / 2;
            floor->floordestheight = s3->floorheight;
            break;
        }
    }
    return rtn;
}




short           numlinespecials;
line_t*         linespeciallist[MAXLINEANIMS];


void P_SpawnSpecials (void)
{
    sector_t*   sector;
    int         i;

    levelTimer = false;

    i = M_CheckParm("-avg");
    if (i && deathmatch)
    {
        levelTimer = true;
        levelTimeCount = 20 * 60 * 35;
    }

    i = M_CheckParm("-timer");
    if (i && deathmatch)
    {
        int     time;
        time = atoi(myargv[i+1]) * 60 * 35;
        levelTimer = true;
        levelTimeCount = time;
    }

    sector = sectors;
    for (i=0 ; i<numsectors ; i++, sector++)
    {
        if (!sector->special)
            continue;

        switch (sector->special)
        {
          case 1:
            P_SpawnLightFlash (sector);
            break;

          case 2:
            P_SpawnStrobeFlash(sector,FASTDARK,0);
            break;

          case 3:
            P_SpawnStrobeFlash(sector,SLOWDARK,0);
            break;

          case 4:
            P_SpawnStrobeFlash(sector,FASTDARK,0);
            sector->special = 4;
            break;

          case 8:
            P_SpawnGlowingLight(sector);
            break;
          case 9:
            totalsecret++;
            break;

          case 10:
            P_SpawnDoorCloseIn30 (sector);
            break;

          case 12:
            P_SpawnStrobeFlash (sector, SLOWDARK, 1);
            break;

          case 13:
            P_SpawnStrobeFlash (sector, FASTDARK, 1);
            break;

          case 14:
            P_SpawnDoorRaiseIn5Mins (sector, i);
            break;

          case 17:
            P_SpawnFireFlicker(sector);
            break;
        }
    }


    numlinespecials = 0;
    for (i = 0;i < numlines; i++)
    {
        switch(lines[i].special)
        {
          case 48:
            linespeciallist[numlinespecials] = &lines[i];
            numlinespecials++;
            break;
        }
    }


    for (i = 0;i < MAXCEILINGS;i++)
        activeceilings[i] = NULL;

    for (i = 0;i < MAXPLATS;i++)
        activeplats[i] = NULL;

    for (i = 0;i < MAXBUTTONS;i++)
        memset(&buttonlist[i],0,sizeof(button_t));

}
