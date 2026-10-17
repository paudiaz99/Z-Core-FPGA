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
//  Sprite animation.
//
//-----------------------------------------------------------------------------


#ifndef __P_PSPR__
#define __P_PSPR__

#include "m_fixed.h"
#include "tables.h"


#include "info.h"

#ifdef __GNUG__
#pragma interface
#endif


#define FF_FULLBRIGHT   0x8000
#define FF_FRAMEMASK    0x7fff



typedef enum
{
    ps_weapon,
    ps_flash,
    NUMPSPRITES

} psprnum_t;

typedef struct
{
    state_t*    state;
    int         tics;
    fixed_t     sx;
    fixed_t     sy;

} pspdef_t;

#endif
