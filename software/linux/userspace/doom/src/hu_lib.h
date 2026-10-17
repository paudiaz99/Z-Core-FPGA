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
//
//-----------------------------------------------------------------------------

#ifndef __HULIB__
#define __HULIB__

#include "r_defs.h"


#define BG                      1
#define FG                      0

#define HU_CHARERASE    KEY_BACKSPACE

#define HU_MAXLINES             4
#define HU_MAXLINELENGTH        80


typedef struct
{
    int         x;
    int         y;

    patch_t**   f;
    int         sc;
    char        l[HU_MAXLINELENGTH+1];
    int         len;

    int         needsupdate;

} hu_textline_t;



typedef struct
{
    hu_textline_t       l[HU_MAXLINES];
    int                 h;
    int                 cl;

    boolean*            on;
    boolean             laston;

} hu_stext_t;



typedef struct
{
    hu_textline_t       l;

    int                 lm;

    boolean*            on;
    boolean             laston;

} hu_itext_t;



void HUlib_init(void);


void    HUlib_clearTextLine(hu_textline_t *t);

void    HUlib_initTextLine(hu_textline_t *t, int x, int y, patch_t **f, int sc);

boolean HUlib_addCharToTextLine(hu_textline_t *t, char ch);

boolean HUlib_delCharFromTextLine(hu_textline_t *t);

void    HUlib_drawTextLine(hu_textline_t *l, boolean drawcursor);

void    HUlib_eraseTextLine(hu_textline_t *l);



void
HUlib_initSText
( hu_stext_t*   s,
  int           x,
  int           y,
  int           h,
  patch_t**     font,
  int           startchar,
  boolean*      on );

void HUlib_addLineToSText(hu_stext_t* s);

void
HUlib_addMessageToSText
( hu_stext_t*   s,
  char*         prefix,
  char*         msg );

void HUlib_drawSText(hu_stext_t* s);

void HUlib_eraseSText(hu_stext_t* s);

void
HUlib_initIText
( hu_itext_t*   it,
  int           x,
  int           y,
  patch_t**     font,
  int           startchar,
  boolean*      on );

void HUlib_delCharFromIText(hu_itext_t* it);

void HUlib_eraseLineFromIText(hu_itext_t* it);

void HUlib_resetIText(hu_itext_t* it);

void
HUlib_addPrefixToIText
( hu_itext_t*   it,
  char*         str );

boolean
HUlib_keyInIText
( hu_itext_t*   it,
  unsigned char ch );

void HUlib_drawIText(hu_itext_t* it);

void HUlib_eraseIText(hu_itext_t* it);

#endif
