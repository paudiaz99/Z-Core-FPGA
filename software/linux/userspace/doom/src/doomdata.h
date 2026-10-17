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
//  all external data is defined here
//  most of the data is loaded into different structures at run time
//  some internal structures shared by many modules are here
//
//-----------------------------------------------------------------------------

#ifndef __DOOMDATA__
#define __DOOMDATA__

#include "doomtype.h"

#include "doomdef.h"




enum
{
  ML_LABEL,
  ML_THINGS,
  ML_LINEDEFS,
  ML_SIDEDEFS,
  ML_VERTEXES,
  ML_SEGS,
  ML_SSECTORS,
  ML_NODES,
  ML_SECTORS,
  ML_REJECT,
  ML_BLOCKMAP
};


typedef struct
{
  short         x;
  short         y;
} mapvertex_t;


typedef struct
{
  short         textureoffset;
  short         rowoffset;
  char          toptexture[8];
  char          bottomtexture[8];
  char          midtexture[8];
  short         sector;
} mapsidedef_t;



typedef struct
{
  short         v1;
  short         v2;
  short         flags;
  short         special;
  short         tag;
  short         sidenum[2];
} maplinedef_t;



#define ML_BLOCKING             1

#define ML_BLOCKMONSTERS        2

#define ML_TWOSIDED             4


#define ML_DONTPEGTOP           8

#define ML_DONTPEGBOTTOM        16

#define ML_SECRET               32

#define ML_SOUNDBLOCK           64

#define ML_DONTDRAW             128

#define ML_MAPPED               256




typedef struct
{
  short         floorheight;
  short         ceilingheight;
  char          floorpic[8];
  char          ceilingpic[8];
  short         lightlevel;
  short         special;
  short         tag;
} mapsector_t;

typedef struct
{
  short         numsegs;
  short         firstseg;
} mapsubsector_t;


typedef struct
{
  short         v1;
  short         v2;
  short         angle;
  short         linedef;
  short         side;
  short         offset;
} mapseg_t;




#define NF_SUBSECTOR    0x8000

typedef struct
{
  short         x;
  short         y;
  short         dx;
  short         dy;

  short         bbox[2][4];

  unsigned short        children[2];

} mapnode_t;




typedef struct
{
    short               x;
    short               y;
    short               angle;
    short               type;
    short               options;
} mapthing_t;





#endif

