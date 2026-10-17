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
//      The status bar widget code.
//
//-----------------------------------------------------------------------------

#ifndef __STLIB__
#define __STLIB__


#include "r_defs.h"


#define BG 4
#define FG 0





typedef struct
{
    int         x;
    int         y;

    int width;

    int         oldnum;

    int*        num;

    boolean*    on;

    patch_t**   p;

    int data;

} st_number_t;



typedef struct
{
    st_number_t         n;

    patch_t*            p;

} st_percent_t;



typedef struct
{
    int                 x;
    int                 y;

    int                 oldinum;

    int*                inum;

    boolean*            on;

    patch_t**           p;

    int                 data;

} st_multicon_t;





typedef struct
{
    int                 x;
    int                 y;

    int                 oldval;

    boolean*            val;

    boolean*            on;


    patch_t*            p;
    int                 data;

} st_binicon_t;




void STlib_init(void);



void
STlib_initNum
( st_number_t*          n,
  int                   x,
  int                   y,
  patch_t**             pl,
  int*                  num,
  boolean*              on,
  int                   width );

void
STlib_updateNum
( st_number_t*          n,
  boolean               refresh );


void
STlib_initPercent
( st_percent_t*         p,
  int                   x,
  int                   y,
  patch_t**             pl,
  int*                  num,
  boolean*              on,
  patch_t*              percent );


void
STlib_updatePercent
( st_percent_t*         per,
  int                   refresh );


void
STlib_initMultIcon
( st_multicon_t*        mi,
  int                   x,
  int                   y,
  patch_t**             il,
  int*                  inum,
  boolean*              on );


void
STlib_updateMultIcon
( st_multicon_t*        mi,
  boolean               refresh );


void
STlib_initBinIcon
( st_binicon_t*         b,
  int                   x,
  int                   y,
  patch_t*              i,
  boolean*              val,
  boolean*              on );

void
STlib_updateBinIcon
( st_binicon_t*         bi,
  boolean               refresh );

#endif
