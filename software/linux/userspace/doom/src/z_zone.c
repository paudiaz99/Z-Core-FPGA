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
//      Zone Memory Allocation. Neat.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: z_zone.c,v 1.4 1997/02/03 16:47:58 b1 Exp $";


#include <stdint.h>

#include "z_zone.h"
#include "i_system.h"
#include "doomdef.h"

#ifdef ZCORE_DOOM
#include "riscv/console.h"
#endif



#define ZONEID  0x1d4a11


typedef struct
{
    int         size;

    memblock_t  blocklist;

    memblock_t* rover;

} memzone_t;



memzone_t*      mainzone;



void Z_ClearZone (memzone_t* zone)
{
    memblock_t*         block;

    zone->blocklist.next =
        zone->blocklist.prev =
        block = (memblock_t *)( (byte *)zone + sizeof(memzone_t) );

    zone->blocklist.user = (void *)zone;
    zone->blocklist.tag = PU_STATIC;
    zone->rover = block;

    block->prev = block->next = &zone->blocklist;

    block->user = NULL;

    block->size = zone->size - sizeof(memzone_t);
}



void Z_Init (void)
{
    memblock_t* block;
    int         size;

    mainzone = (memzone_t *)I_ZoneBase (&size);
    if (!mainzone || size <= sizeof(memzone_t) + sizeof(memblock_t))
        I_Error ("Z_Init: no usable zone memory");
    mainzone->size = size;

    mainzone->blocklist.next =
        mainzone->blocklist.prev =
        block = (memblock_t *)( (byte *)mainzone + sizeof(memzone_t) );

    mainzone->blocklist.user = (void *)mainzone;
    mainzone->blocklist.tag = PU_STATIC;
    mainzone->rover = block;

    block->prev = block->next = &mainzone->blocklist;

    block->user = NULL;

    block->size = mainzone->size - sizeof(memzone_t);

    printf("Z_Init: zone=%p size=%d block=%p bsize=%d\n",
           (void*)mainzone, mainzone->size, (void*)block, block->size);
#ifdef ZCORE_DOOM
    console_printf ("[UART] Z_Init: zone=0x%08x size=%d free_blk=%d\r\n",
                    (unsigned)(size_t)mainzone, mainzone->size, block->size);
#endif
}


void Z_Free (void* ptr)
{
    memblock_t*         block;
    memblock_t*         other;

    block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));

    if (block->id != ZONEID)
        I_Error ("Z_Free: freed a pointer without ZONEID");

    if (block->user > (void **)0x100)
    {

#ifdef ZCORE_DOOM
        if (block->tag >= PU_PURGELEVEL) {
            console_printf("[DIAG] Z_Free purge: ptr=0x%08x user=0x%08x "
                           "tag=%d *user_before=0x%08x\r\n",
                           (unsigned)(size_t)((byte *)block + sizeof(memblock_t)),
                           (unsigned)(size_t)block->user,
                           block->tag,
                           (unsigned)(size_t)(*(void **)block->user));
        }
#endif

        *block->user = 0;
    }

    block->user = NULL;
    block->tag = 0;
    block->id = 0;

    other = block->prev;

    if (!other->user)
    {
        other->size += block->size;
        other->next = block->next;
        other->next->prev = other;

        if (block == mainzone->rover)
            mainzone->rover = other;

        block = other;
    }

    other = block->next;
    if (!other->user)
    {
        block->size += other->size;
        block->next = other->next;
        block->next->prev = block;

        if (other == mainzone->rover)
            mainzone->rover = block;
    }
}



#define MINFRAGMENT             64


void*
Z_Malloc
( int           size,
  int           tag,
  void*         user )
{
    int         extra;
    memblock_t* start;
    memblock_t* rover;
    memblock_t* newblock;
    memblock_t* base;

    size = (size + 3) & ~3;


    size += sizeof(memblock_t);

    base = mainzone->rover;

    if (!base->prev->user)
        base = base->prev;

    rover = base;
    start = base->prev;

    do
    {
        if (rover == start)
        {
            printf("Z_Malloc FAIL: zone=%p size=%d free=%d rover=%p\n",
                   (void*)mainzone, mainzone->size, Z_FreeMemory(), (void*)mainzone->rover);
            {
                memblock_t *blk = mainzone->blocklist.next;
                int n = 0;
                while (blk != &mainzone->blocklist && n < 8) {
                    printf("  blk[%d] %p sz=%d user=%p tag=%d\n",
                           n, (void*)blk, blk->size, (void*)blk->user, blk->tag);
                    blk = blk->next; n++;
                }
            }
            I_Error ("Z_Malloc: failed on allocation of %i bytes", size);
        }

        if (rover->user)
        {
            if (rover->tag < PU_PURGELEVEL)
            {
                base = rover = rover->next;
            }
            else
            {

                base = base->prev;
                Z_Free ((byte *)rover+sizeof(memblock_t));
                base = base->next;
                rover = base->next;
            }
        }
        else
            rover = rover->next;
    } while (base->user || base->size < size);


    extra = base->size - size;

    if (extra >  MINFRAGMENT)
    {
        newblock = (memblock_t *) ((byte *)base + size );
        newblock->size = extra;

        newblock->user = NULL;
        newblock->tag = 0;
        newblock->prev = base;
        newblock->next = base->next;
        newblock->next->prev = newblock;

        base->next = newblock;
        base->size = size;
    }

    if (user)
    {
        base->user = user;
        *(void **)user = (void *) ((byte *)base + sizeof(memblock_t));
    }
    else
    {
        if (tag >= PU_PURGELEVEL)
        {
#ifdef ZCORE_DOOM
            console_printf ("[DIAG] Z_Malloc PURGABLE ERR: size=%d tag=%d user=0x%08x\r\n",
                            size, tag, (unsigned)(size_t)user);
#endif
            I_Error ("Z_Malloc: an owner is required for purgable blocks");
        }

        base->user = (void *)2;
    }
    base->tag = tag;

    mainzone->rover = base->next;

    base->id = ZONEID;

    return (void *) ((byte *)base + sizeof(memblock_t));
}



void
Z_FreeTags
( int           lowtag,
  int           hightag )
{
    memblock_t* block;
    memblock_t* next;

    for (block = mainzone->blocklist.next ;
         block != &mainzone->blocklist ;
         block = next)
    {
        if (((uintptr_t)block & 3u) != 0)
            I_Error ("Z_FreeTags: unaligned block %p", (void *)block);
        {
            byte *lo = (byte *)mainzone;
            byte *hi = lo + mainzone->size;
            if ((byte *)block < lo || (byte *)block + (int)sizeof(memblock_t) > hi)
                I_Error ("Z_FreeTags: block %p out of zone [%p..%p)",
                         (void *)block, (void *)lo, (void *)hi);
        }

        next = block->next;

        if (!block->user)
            continue;

        if (block->tag >= lowtag && block->tag <= hightag)
            Z_Free ( (byte *)block+sizeof(memblock_t));
    }
}



void
Z_DumpHeap
( int           lowtag,
  int           hightag )
{
    memblock_t* block;

    printf ("zone size: %i  location: %p\n",
            mainzone->size,mainzone);

    printf ("tag range: %i to %i\n",
            lowtag, hightag);

    for (block = mainzone->blocklist.next ; ; block = block->next)
    {
        if (block->tag >= lowtag && block->tag <= hightag)
            printf ("block:%p    size:%7i    user:%p    tag:%3i\n",
                    block, block->size, block->user, block->tag);

        if (block->next == &mainzone->blocklist)
        {
            break;
        }

        if ( (byte *)block + block->size != (byte *)block->next)
            printf ("ERROR: block size does not touch the next block\n");

        if ( block->next->prev != block)
            printf ("ERROR: next block doesn't have proper back link\n");

        if (!block->user && !block->next->user)
            printf ("ERROR: two consecutive free blocks\n");
    }
}


void Z_FileDumpHeap (FILE* f)
{
    memblock_t* block;

    fprintf (f,"zone size: %i  location: %p\n",mainzone->size,mainzone);

    for (block = mainzone->blocklist.next ; ; block = block->next)
    {
        fprintf (f,"block:%p    size:%7i    user:%p    tag:%3i\n",
                 block, block->size, block->user, block->tag);

        if (block->next == &mainzone->blocklist)
        {
            break;
        }

        if ( (byte *)block + block->size != (byte *)block->next)
            fprintf (f,"ERROR: block size does not touch the next block\n");

        if ( block->next->prev != block)
            fprintf (f,"ERROR: next block doesn't have proper back link\n");

        if (!block->user && !block->next->user)
            fprintf (f,"ERROR: two consecutive free blocks\n");
    }
}



void Z_CheckHeap (void)
{
    memblock_t* block;

    for (block = mainzone->blocklist.next ; ; block = block->next)
    {
        if (block->next == &mainzone->blocklist)
        {
            break;
        }

        if ( (byte *)block + block->size != (byte *)block->next)
            I_Error ("Z_CheckHeap: block size does not touch the next block\n");

        if ( block->next->prev != block)
            I_Error ("Z_CheckHeap: next block doesn't have proper back link\n");

        if (!block->user && !block->next->user)
            I_Error ("Z_CheckHeap: two consecutive free blocks\n");
    }
}




void
Z_ChangeTag2
( void*         ptr,
  int           tag )
{
    memblock_t* block;

    block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));

    if (block->id != ZONEID)
        I_Error ("Z_ChangeTag: freed a pointer without ZONEID");

    if (tag >= PU_PURGELEVEL && (unsigned)block->user < 0x100)
        I_Error ("Z_ChangeTag: an owner is required for purgable blocks");

    block->tag = tag;
}



int Z_FreeMemory (void)
{
    memblock_t*         block;
    int                 free;

    free = 0;

    for (block = mainzone->blocklist.next ;
         block != &mainzone->blocklist;
         block = block->next)
    {
        if (!block->user || block->tag >= PU_PURGELEVEL)
            free += block->size;
    }
    return free;
}

