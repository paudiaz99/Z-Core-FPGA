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
//      Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: w_wad.c,v 1.5 1997/02/03 16:47:57 b1 Exp $";


#ifdef NORMALUNIX
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <alloca.h>
#define O_BINARY                0
#endif

#include "doomtype.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"

#ifdef __GNUG__
#pragma implementation "w_wad.h"
#endif
#include "w_wad.h"

#ifdef ZCORE_DOOM
#include "riscv/console.h"
#endif




lumpinfo_t*             lumpinfo;
int                     numlumps;

void**                  lumpcache;


#define strcmpi strcasecmp


char* strupr (char* str)
{
    char *s = str;
    while (*s) {
        *s = (char) toupper ((unsigned char) *s);
        s++;
    }
    return str;
}


static void
W_ReadFull (int handle, void *buf, int nbytes)
{
    unsigned char *p = (unsigned char *) buf;
    int            got = 0;

    while (got < nbytes)
    {
        ssize_t n = read (handle, p + got, (size_t)(nbytes - got));
        if (n <= 0)
            I_Error ("W_ReadFull: got %i need %i (last n=%i)",
                     got, nbytes, (int) n);
        got += (int) n;
    }
}

int filelength (int handle)
{
    struct stat fileinfo;

    if (fstat (handle,&fileinfo) == -1)
        I_Error ("Error fstating");

    return fileinfo.st_size;
}


void
ExtractFileBase
( char*         path,
  char*         dest )
{
    char*       src;
    int         length;

    src = path + strlen(path) - 1;

    while (src != path
           && *(src-1) != '\\'
           && *(src-1) != '/')
    {
        src--;
    }

    memset (dest,0,8);
    length = 0;

    while (*src && *src != '.')
    {
        if (++length == 9)
            I_Error ("Filename base of %s >8 chars",path);

        *dest++ = toupper((int)*src++);
    }
}







int                     reloadlump;
char*                   reloadname;


void W_AddFile (char *filename)
{
    wadinfo_t           header;
    lumpinfo_t*         lump_p;
    unsigned            i;
    int                 handle;
    int                 length;
    int                 startlump;
    filelump_t*         fileinfo;
    filelump_t*         dirbuf = NULL;
    filelump_t          singleinfo;
    int                 storehandle;


    if (filename[0] == '~')
    {
        filename++;
        reloadname = filename;
        reloadlump = numlumps;
    }

    if ( (handle = open (filename,O_RDONLY | O_BINARY)) == -1)
    {
        printf (" couldn't open %s\n",filename);
        return;
    }

    printf (" adding %s\n",filename);
    startlump = numlumps;

    if (strcmpi (filename+strlen(filename)-3 , "wad" ) )
    {
        fileinfo = &singleinfo;
        singleinfo.filepos = 0;
        singleinfo.size = filelength(handle);
        ExtractFileBase (filename, singleinfo.name);
        numlumps++;
    }
    else
    {
        unsigned char     rawhdr[12];
        ssize_t           nread;

        nread = read (handle, rawhdr, sizeof (rawhdr));
        if (nread != (ssize_t) sizeof (rawhdr))
            I_Error ("Wad file %s: header read got %i bytes, need 12",
                     filename, (int) nread);

        if (memcmp (rawhdr, "IWAD", 4) && memcmp (rawhdr, "PWAD", 4))
            I_Error ("Wad file %s doesn't have IWAD or PWAD id\n", filename);

        memcpy (header.identification, rawhdr, 4);
        memcpy (&header.numlumps, rawhdr + 4, sizeof (header.numlumps));
        memcpy (&header.infotableofs, rawhdr + 8, sizeof (header.infotableofs));
        header.numlumps = LONG(header.numlumps);
        header.infotableofs = LONG(header.infotableofs);
#ifdef ZCORE_DOOM
        console_printf ("[UART] WAD hdr: %c%c%c%c lumps=%d dirofs=0x%x\r\n",
                        rawhdr[0], rawhdr[1], rawhdr[2], rawhdr[3],
                        header.numlumps, header.infotableofs);
#endif
        if (header.numlumps < 1 || header.numlumps > 65536)
            I_Error ("W_AddFile: absurd lump count %i", header.numlumps);
        length = header.numlumps * (int) sizeof (filelump_t);
        if (length / (int) sizeof (filelump_t) != header.numlumps)
            I_Error ("W_AddFile: lump directory size overflow");
        dirbuf = (filelump_t *) Z_Malloc (length, PU_STATIC, 0);
        fileinfo = dirbuf;
        if (lseek (handle, header.infotableofs, SEEK_SET) == (off_t) -1)
            I_Error ("W_AddFile: lseek dir");
        W_ReadFull (handle, fileinfo, length);
        numlumps += header.numlumps;
    }


    {
        lumpinfo_t *oldinfo = lumpinfo;
        lumpinfo = Z_Malloc (numlumps*sizeof(lumpinfo_t), PU_STATIC, NULL);
        if (oldinfo)
        {
            memcpy (lumpinfo, oldinfo, startlump*sizeof(lumpinfo_t));
            Z_Free (oldinfo);
        }
    }

    lump_p = &lumpinfo[startlump];

    storehandle = reloadname ? -1 : handle;

    for (i=startlump ; i<numlumps ; i++, lump_p++, fileinfo++)
    {
        lump_p->handle = storehandle;
        lump_p->position = LONG(fileinfo->filepos);
        lump_p->size = LONG(fileinfo->size);
        memcpy (lump_p->name, fileinfo->name, sizeof (lump_p->name));
    }

    if (dirbuf)
        Z_Free (dirbuf);

#ifdef ZCORE_DOOM
    console_printf ("[UART] W_AddFile done: %s numlumps=%d\r\n",
                    filename, numlumps);
#endif

    if (reloadname)
        close (handle);
}




void W_Reload (void)
{
    wadinfo_t           header;
    int                 lumpcount;
    lumpinfo_t*         lump_p;
    unsigned            i;
    int                 handle;
    int                 length;
    filelump_t*         fileinfo;

    if (!reloadname)
        return;

    if ( (handle = open (reloadname,O_RDONLY | O_BINARY)) == -1)
        I_Error ("W_Reload: couldn't open %s",reloadname);

    {
        unsigned char rawhdr[12];
        ssize_t       nread = read (handle, rawhdr, sizeof (rawhdr));
        if (nread != (ssize_t) sizeof (rawhdr))
            I_Error ("W_Reload: header read got %i bytes", (int) nread);
        memcpy (header.identification, rawhdr, 4);
        memcpy (&header.numlumps, rawhdr + 4, sizeof (header.numlumps));
        memcpy (&header.infotableofs, rawhdr + 8, sizeof (header.infotableofs));
    }
    lumpcount = LONG(header.numlumps);
    header.infotableofs = LONG(header.infotableofs);
    length = lumpcount * (int) sizeof (filelump_t);
    {
        filelump_t *dir = (filelump_t *) Z_Malloc (length, PU_STATIC, 0);

        fileinfo = dir;
        if (lseek (handle, header.infotableofs, SEEK_SET) == (off_t) -1)
            I_Error ("W_Reload: lseek dir");
        W_ReadFull (handle, fileinfo, length);

        lump_p = &lumpinfo[reloadlump];

        for (i=reloadlump ;
             i<reloadlump+lumpcount ;
             i++, lump_p++, fileinfo++)
        {
            if (lumpcache[i])
                Z_Free (lumpcache[i]);

            lump_p->position = LONG(fileinfo->filepos);
            lump_p->size = LONG(fileinfo->size);
        }

        Z_Free (dir);
    }

    close (handle);
}



void W_InitMultipleFiles (char** filenames)
{
    int         size;

    numlumps = 0;

    lumpinfo = NULL;

    for ( ; *filenames ; filenames++)
        W_AddFile (*filenames);

    if (!numlumps)
        I_Error ("W_InitFiles: no files found");

    size = numlumps * sizeof(*lumpcache);
    lumpcache = Z_Malloc (size, PU_STATIC, NULL);

    memset (lumpcache,0, size);
}




void W_InitFile (char* filename)
{
    char*       names[2];

    names[0] = filename;
    names[1] = NULL;
    W_InitMultipleFiles (names);
}



int W_NumLumps (void)
{
    return numlumps;
}




int W_CheckNumForName (char* name)
{
    char        name8[9];
    lumpinfo_t* lump_p;

    memset (name8, 0, sizeof (name8));
    strncpy (name8, name, 8);
    strupr (name8);

    lump_p = lumpinfo + numlumps;

    while (lump_p-- != lumpinfo)
    {
        if (!memcmp (lump_p->name, name8, 8))
            return lump_p - lumpinfo;
    }

    return -1;
}




int W_GetNumForName (char* name)
{
    int i;

    i = W_CheckNumForName (name);

    if (i == -1)
      I_Error ("W_GetNumForName: %s not found!", name);

    return i;
}


int W_LumpLength (int lump)
{
    if (lump >= numlumps)
        I_Error ("W_LumpLength: %i >= numlumps",lump);

    return lumpinfo[lump].size;
}



void
W_ReadLump
( int           lump,
  void*         dest )
{
    int         c;
    lumpinfo_t* l;
    int         handle;

    if (lump >= numlumps)
        I_Error ("W_ReadLump: %i >= numlumps",lump);

    l = lumpinfo+lump;


    if (l->handle == -1)
    {
        if ( (handle = open (reloadname,O_RDONLY | O_BINARY)) == -1)
            I_Error ("W_ReadLump: couldn't open %s",reloadname);
    }
    else
        handle = l->handle;

    lseek (handle, l->position, SEEK_SET);
    c = read (handle, dest, l->size);

    if (c < l->size)
        I_Error ("W_ReadLump: only read %i of %i on lump %i",
                 c,l->size,lump);

    if (l->handle == -1)
        close (handle);

}




void*
W_CacheLumpNum
( int           lump,
  int           tag )
{
    if ((unsigned)lump >= numlumps)
        I_Error ("W_CacheLumpNum: %i >= numlumps",lump);

    if (lumpcache[lump])
    {
        memblock_t *blk = (memblock_t *)((byte *)lumpcache[lump]
                                          - sizeof(memblock_t));
        volatile int blk_id = blk->id;
        if (blk_id == 0x1d4a11)
        {
            blk->tag = tag;
        }
        else
        {
#ifdef ZCORE_DOOM
            console_printf("[DIAG] stale lump=%d id=0x%08x\r\n",
                           lump, (unsigned)blk_id);
#endif
            lumpcache[lump] = NULL;
        }
    }

    if (!lumpcache[lump])
    {
        Z_Malloc (W_LumpLength (lump), tag, &lumpcache[lump]);
        W_ReadLump (lump, lumpcache[lump]);
    }

    return lumpcache[lump];
}



void*
W_CacheLumpName
( char*         name,
  int           tag )
{
    return W_CacheLumpNum (W_GetNumForName(name), tag);
}


int             info[2500][10];
int             profilecount;

void W_Profile (void)
{
    int         i;
    memblock_t* block;
    void*       ptr;
    char        ch;
    FILE*       f;
    int         j;
    char        name[9];


    for (i=0 ; i<numlumps ; i++)
    {
        ptr = lumpcache[i];
        if (!ptr)
        {
            ch = ' ';
            continue;
        }
        else
        {
            block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));
            if (block->tag < PU_PURGELEVEL)
                ch = 'S';
            else
                ch = 'P';
        }
        info[i][profilecount] = ch;
    }
    profilecount++;

    f = fopen ("waddump.txt","w");
    name[8] = 0;

    for (i=0 ; i<numlumps ; i++)
    {
        memcpy (name,lumpinfo[i].name,8);

        for (j=0 ; j<8 ; j++)
            if (!name[j])
                break;

        for ( ; j<8 ; j++)
            name[j] = ' ';

        fprintf (f,"%s ",name);

        for (j=0 ; j<profilecount ; j++)
            fprintf (f,"    %c",info[i][j]);

        fprintf (f,"\n");
    }
    fclose (f);
}


