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
//      DOOM strings, by language.
//
//-----------------------------------------------------------------------------


#ifndef __DSTRINGS__
#define __DSTRINGS__



#ifdef FRENCH
#include "d_french.h"
#else
#include "d_englsh.h"
#endif

#define SAVEGAMENAME    "doomsav"


#define DEVMAPS "devmaps"
#define DEVDATA "devdata"



#define NUM_QUITMESSAGES   22

extern char* endmsg[];


#endif
