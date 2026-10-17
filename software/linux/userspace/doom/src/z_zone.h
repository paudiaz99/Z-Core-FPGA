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
//      Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
//      Remark: this was the only stuff that, according
//       to John Carmack, might have been useful for
//       Quake.
//
//---------------------------------------------------------------------



#ifndef __Z_ZONE__
#define __Z_ZONE__

#include <stdio.h>

#define PU_STATIC               1
#define PU_SOUND                2
#define PU_MUSIC                3
#define PU_DAVE         4
#define PU_LEVEL                50
#define PU_LEVSPEC              51
#define PU_PURGELEVEL   100
#define PU_CACHE                101


void    Z_Init (void);
void*   Z_Malloc (int size, int tag, void *ptr);
void    Z_Free (void *ptr);
void    Z_FreeTags (int lowtag, int hightag);
void    Z_DumpHeap (int lowtag, int hightag);
void    Z_FileDumpHeap (FILE *f);
void    Z_CheckHeap (void);
void    Z_ChangeTag2 (void *ptr, int tag);
int     Z_FreeMemory (void);


typedef struct memblock_s
{
    int                 size;
    void**              user;
    int                 tag;
    int                 id;
    struct memblock_s*  next;
    struct memblock_s*  prev;
} memblock_t;

#define Z_ChangeTag(p,t) do { \
      memblock_t *_zct_b = (memblock_t *)( (byte *)(p) - sizeof(memblock_t)); \
      volatile int _zct_id = _zct_b->id; \
      if (_zct_id != 0x1d4a11) \
          I_Error("Z_CT at " __FILE__ ":%i id=0x%x",__LINE__,_zct_id); \
      _zct_b->tag = (t); \
} while (0)



#endif
