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
// DESCRIPTION:  none
//      Implements special effects:
//      Texture animation, height or lighting changes
//       according to adjacent sectors, respective
//       utility functions, etc.
//
//-----------------------------------------------------------------------------


#ifndef __P_SPEC__
#define __P_SPEC__


extern  boolean levelTimer;
extern  int     levelTimeCount;


#define MO_TELEPORTMAN          14


void    P_InitPicAnims (void);

void    P_SpawnSpecials (void);

void    P_UpdateSpecials (void);

boolean
P_UseSpecialLine
( mobj_t*       thing,
  line_t*       line,
  int           side );

void
P_ShootSpecialLine
( mobj_t*       thing,
  line_t*       line );

void
P_CrossSpecialLine
( int           linenum,
  int           side,
  mobj_t*       thing );

void    P_PlayerInSpecialSector (player_t* player);

int
twoSided
( int           sector,
  int           line );

sector_t*
getSector
( int           currentSector,
  int           line,
  int           side );

side_t*
getSide
( int           currentSector,
  int           line,
  int           side );

fixed_t P_FindLowestFloorSurrounding(sector_t* sec);
fixed_t P_FindHighestFloorSurrounding(sector_t* sec);

fixed_t
P_FindNextHighestFloor
( sector_t*     sec,
  int           currentheight );

fixed_t P_FindLowestCeilingSurrounding(sector_t* sec);
fixed_t P_FindHighestCeilingSurrounding(sector_t* sec);

int
P_FindSectorFromLineTag
( line_t*       line,
  int           start );

int
P_FindMinSurroundingLight
( sector_t*     sector,
  int           max );

sector_t*
getNextSector
( line_t*       line,
  sector_t*     sec );


int EV_DoDonut(line_t* line);



typedef struct
{
    thinker_t   thinker;
    sector_t*   sector;
    int         count;
    int         maxlight;
    int         minlight;

} fireflicker_t;



typedef struct
{
    thinker_t   thinker;
    sector_t*   sector;
    int         count;
    int         maxlight;
    int         minlight;
    int         maxtime;
    int         mintime;

} lightflash_t;



typedef struct
{
    thinker_t   thinker;
    sector_t*   sector;
    int         count;
    int         minlight;
    int         maxlight;
    int         darktime;
    int         brighttime;

} strobe_t;




typedef struct
{
    thinker_t   thinker;
    sector_t*   sector;
    int         minlight;
    int         maxlight;
    int         direction;

} glow_t;


#define GLOWSPEED                       8
#define STROBEBRIGHT            5
#define FASTDARK                        15
#define SLOWDARK                        35

void    P_SpawnFireFlicker (sector_t* sector);
void    T_LightFlash (lightflash_t* flash);
void    P_SpawnLightFlash (sector_t* sector);
void    T_StrobeFlash (strobe_t* flash);

void
P_SpawnStrobeFlash
( sector_t*     sector,
  int           fastOrSlow,
  int           inSync );

void    EV_StartLightStrobing(line_t* line);
void    EV_TurnTagLightsOff(line_t* line);

void
EV_LightTurnOn
( line_t*       line,
  int           bright );

void    T_Glow(glow_t* g);
void    P_SpawnGlowingLight(sector_t* sector);




typedef struct
{
    char        name1[9];
    char        name2[9];
    short       episode;

} switchlist_t;


typedef enum
{
    top,
    middle,
    bottom

} bwhere_e;


typedef struct
{
    line_t*     line;
    bwhere_e    where;
    int         btexture;
    int         btimer;
    mobj_t*     soundorg;

} button_t;




#define MAXSWITCHES             50

#define MAXBUTTONS              16

#define BUTTONTIME      35

extern button_t buttonlist[MAXBUTTONS];

void
P_ChangeSwitchTexture
( line_t*       line,
  int           useAgain );

void P_InitSwitchList(void);


typedef enum
{
    up,
    down,
    waiting,
    in_stasis

} plat_e;



typedef enum
{
    perpetualRaise,
    downWaitUpStay,
    raiseAndChange,
    raiseToNearestAndChange,
    blazeDWUS

} plattype_e;



typedef struct
{
    thinker_t   thinker;
    sector_t*   sector;
    fixed_t     speed;
    fixed_t     low;
    fixed_t     high;
    int         wait;
    int         count;
    plat_e      status;
    plat_e      oldstatus;
    boolean     crush;
    int         tag;
    plattype_e  type;

} plat_t;



#define PLATWAIT                3
#define PLATSPEED               FRACUNIT
#define MAXPLATS                30


extern plat_t*  activeplats[MAXPLATS];

void    T_PlatRaise(plat_t*     plat);

int
EV_DoPlat
( line_t*       line,
  plattype_e    type,
  int           amount );

void    P_AddActivePlat(plat_t* plat);
void    P_RemoveActivePlat(plat_t* plat);
void    EV_StopPlat(line_t* line);
void    P_ActivateInStasis(int tag);


typedef enum
{
    normal,
    close30ThenOpen,
    close,
    open,
    raiseIn5Mins,
    blazeRaise,
    blazeOpen,
    blazeClose

} vldoor_e;



typedef struct
{
    thinker_t   thinker;
    vldoor_e    type;
    sector_t*   sector;
    fixed_t     topheight;
    fixed_t     speed;

    int             direction;

    int             topwait;
    int             topcountdown;

} vldoor_t;



#define VDOORSPEED              FRACUNIT*2
#define VDOORWAIT               150

void
EV_VerticalDoor
( line_t*       line,
  mobj_t*       thing );

int
EV_DoDoor
( line_t*       line,
  vldoor_e      type );

int
EV_DoLockedDoor
( line_t*       line,
  vldoor_e      type,
  mobj_t*       thing );

void    T_VerticalDoor (vldoor_t* door);
void    P_SpawnDoorCloseIn30 (sector_t* sec);

void
P_SpawnDoorRaiseIn5Mins
( sector_t*     sec,
  int           secnum );



#if 0
typedef enum
{
    sd_opening,
    sd_waiting,
    sd_closing

} sd_e;



typedef enum
{
    sdt_openOnly,
    sdt_closeOnly,
    sdt_openAndClose

} sdt_e;




typedef struct
{
    thinker_t   thinker;
    sdt_e       type;
    line_t*     line;
    int         frame;
    int         whichDoorIndex;
    int         timer;
    sector_t*   frontsector;
    sector_t*   backsector;
    sd_e         status;

} slidedoor_t;



typedef struct
{
    char        frontFrame1[9];
    char        frontFrame2[9];
    char        frontFrame3[9];
    char        frontFrame4[9];
    char        backFrame1[9];
    char        backFrame2[9];
    char        backFrame3[9];
    char        backFrame4[9];

} slidename_t;



typedef struct
{
    int             frontFrames[4];
    int             backFrames[4];

} slideframe_t;



#define SNUMFRAMES              4

#define SDOORWAIT               35*3
#define SWAITTICS               4

#define MAXSLIDEDOORS   5

void P_InitSlidingDoorFrames(void);

void
EV_SlidingDoor
( line_t*       line,
  mobj_t*       thing );
#endif



typedef enum
{
    lowerToFloor,
    raiseToHighest,
    lowerAndCrush,
    crushAndRaise,
    fastCrushAndRaise,
    silentCrushAndRaise

} ceiling_e;



typedef struct
{
    thinker_t   thinker;
    ceiling_e   type;
    sector_t*   sector;
    fixed_t     bottomheight;
    fixed_t     topheight;
    fixed_t     speed;
    boolean     crush;

    int         direction;

    int         tag;
    int         olddirection;

} ceiling_t;





#define CEILSPEED               FRACUNIT
#define CEILWAIT                150
#define MAXCEILINGS             30

extern ceiling_t*       activeceilings[MAXCEILINGS];

int
EV_DoCeiling
( line_t*       line,
  ceiling_e     type );

void    T_MoveCeiling (ceiling_t* ceiling);
void    P_AddActiveCeiling(ceiling_t* c);
void    P_RemoveActiveCeiling(ceiling_t* c);
int     EV_CeilingCrushStop(line_t* line);
void    P_ActivateInStasisCeiling(line_t* line);


typedef enum
{
    lowerFloor,

    lowerFloorToLowest,

    turboLower,

    raiseFloor,

    raiseFloorToNearest,

    raiseToTexture,

    lowerAndChange,

    raiseFloor24,
    raiseFloor24AndChange,
    raiseFloorCrush,

    raiseFloorTurbo,
    donutRaise,
    raiseFloor512

} floor_e;




typedef enum
{
    build8,
    turbo16

} stair_e;



typedef struct
{
    thinker_t   thinker;
    floor_e     type;
    boolean     crush;
    sector_t*   sector;
    int         direction;
    int         newspecial;
    short       texture;
    fixed_t     floordestheight;
    fixed_t     speed;

} floormove_t;



#define FLOORSPEED              FRACUNIT

typedef enum
{
    ok,
    crushed,
    pastdest

} result_e;

result_e
T_MovePlane
( sector_t*     sector,
  fixed_t       speed,
  fixed_t       dest,
  boolean       crush,
  int           floorOrCeiling,
  int           direction );

int
EV_BuildStairs
( line_t*       line,
  stair_e       type );

int
EV_DoFloor
( line_t*       line,
  floor_e       floortype );

void T_MoveFloor( floormove_t* floor);

int
EV_Teleport
( line_t*       line,
  int           side,
  mobj_t*       thing );

#endif
