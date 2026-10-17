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
//      Refresh/rendering module, shared data struct definitions.
//
//-----------------------------------------------------------------------------


#ifndef __R_DEFS__
#define __R_DEFS__


#include "doomdef.h"

#include "m_fixed.h"

#include "d_think.h"
#include "p_mobj.h"



#ifdef __GNUG__
#pragma interface
#endif



#define SIL_NONE                0
#define SIL_BOTTOM              1
#define SIL_TOP                 2
#define SIL_BOTH                3

#define MAXDRAWSEGS             256






typedef struct
{
    fixed_t     x;
    fixed_t     y;

} vertex_t;


struct line_s;

typedef struct
{
    thinker_t           thinker;
    fixed_t             x;
    fixed_t             y;
    fixed_t             z;

} degenmobj_t;

typedef struct
{
    fixed_t     floorheight;
    fixed_t     ceilingheight;
    short       floorpic;
    short       ceilingpic;
    short       lightlevel;
    short       special;
    short       tag;

    int         soundtraversed;

    mobj_t*     soundtarget;

    int         blockbox[4];

    degenmobj_t soundorg;

    int         validcount;

    mobj_t*     thinglist;

    void*       specialdata;

    int                 linecount;
    struct line_s**     lines;

} sector_t;





typedef struct
{
    fixed_t     textureoffset;

    fixed_t     rowoffset;

    short       toptexture;
    short       bottomtexture;
    short       midtexture;

    sector_t*   sector;

} side_t;



typedef enum
{
    ST_HORIZONTAL,
    ST_VERTICAL,
    ST_POSITIVE,
    ST_NEGATIVE

} slopetype_t;



typedef struct line_s
{
    vertex_t*   v1;
    vertex_t*   v2;

    fixed_t     dx;
    fixed_t     dy;

    short       flags;
    short       special;
    short       tag;

    short       sidenum[2];

    fixed_t     bbox[4];

    slopetype_t slopetype;

    sector_t*   frontsector;
    sector_t*   backsector;

    int         validcount;

    void*       specialdata;
} line_t;




typedef struct subsector_s
{
    sector_t*   sector;
    short       numlines;
    short       firstline;

} subsector_t;



typedef struct
{
    vertex_t*   v1;
    vertex_t*   v2;

    fixed_t     offset;

    angle_t     angle;

    side_t*     sidedef;
    line_t*     linedef;

    sector_t*   frontsector;
    sector_t*   backsector;

} seg_t;



typedef struct
{
    fixed_t     x;
    fixed_t     y;
    fixed_t     dx;
    fixed_t     dy;

    fixed_t     bbox[2][4];

    unsigned short children[2];

} node_t;




typedef struct
{
    byte                topdelta;
    byte                length;
} post_t;

typedef post_t  column_t;









typedef byte    lighttable_t;




typedef struct drawseg_s
{
    seg_t*              curline;
    int                 x1;
    int                 x2;

    fixed_t             scale1;
    fixed_t             scale2;
    fixed_t             scalestep;

    int                 silhouette;

    fixed_t             bsilheight;

    fixed_t             tsilheight;

    short*              sprtopclip;
    short*              sprbottomclip;
    short*              maskedtexturecol;

} drawseg_t;



typedef struct
{
    short               width;
    short               height;
    short               leftoffset;
    short               topoffset;
    int                 columnofs[8];
} patch_t;







typedef struct vissprite_s
{
    struct vissprite_s* prev;
    struct vissprite_s* next;

    int                 x1;
    int                 x2;

    fixed_t             gx;
    fixed_t             gy;

    fixed_t             gz;
    fixed_t             gzt;

    fixed_t             startfrac;

    fixed_t             scale;

    fixed_t             xiscale;

    fixed_t             texturemid;
    int                 patch;

    lighttable_t*       colormap;

    int                 mobjflags;

} vissprite_t;


typedef struct
{
    boolean     rotate;

    short       lump[8];

    byte        flip[8];

} spriteframe_t;



typedef struct
{
    int                 numframes;
    spriteframe_t*      spriteframes;

} spritedef_t;



typedef struct
{
  fixed_t               height;
  int                   picnum;
  int                   lightlevel;
  int                   minx;
  int                   maxx;


  byte          pad1;
  byte          top[SCREENWIDTH];
  byte          pad2;
  byte          pad3;
  byte          bottom[SCREENWIDTH];
  byte          pad4;

} visplane_t;




#endif
