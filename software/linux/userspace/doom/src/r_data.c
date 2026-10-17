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
// Revision 1.3  1997/01/29 20:10
// DESCRIPTION:
//      Preparation of data for rendering,
//      generation of lookups, caching, retrieval by name.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: r_data.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";


#include  <alloca.h>
#include  <string.h>

#include "i_system.h"
#include "z_zone.h"

#include "m_swap.h"

#include "w_wad.h"

#include "doomdef.h"
#include "r_local.h"
#include "p_local.h"

#include "doomstat.h"
#include "r_sky.h"

#ifdef ZCORE_DOOM
#include "riscv/console.h"
#endif

#include "r_data.h"




typedef struct
{
    short       originx;
    short       originy;
    short       patch;
    short       stepdir;
    short       colormap;
} mappatch_t;


typedef struct
{
    char                name[8];
    boolean             masked;
    short               width;
    short               height;
    void                **columndirectory;
    short               patchcount;
    mappatch_t  patches[1];
} maptexture_t;


typedef struct
{
    int         originx;
    int         originy;
    int         patch;
} texpatch_t;


typedef struct
{
    char        name[8];
    short       width;
    short       height;

    short       patchcount;
    texpatch_t  patches[1];

} texture_t;



int             firstflat;
int             lastflat;
int             numflats;

int             firstpatch;
int             lastpatch;
int             numpatches;

int             firstspritelump;
int             lastspritelump;
int             numspritelumps;

int             numtextures;
texture_t**     textures;


int*                    texturewidthmask;
fixed_t*                textureheight;
int*                    texturecompositesize;
short**                 texturecolumnlump;
unsigned short**        texturecolumnofs;
byte**                  texturecomposite;

int*            flattranslation;
int*            texturetranslation;

fixed_t*        spritewidth;
fixed_t*        spriteoffset;
fixed_t*        spritetopoffset;

lighttable_t    *colormaps;





void
R_DrawColumnInCache
( column_t*     patch,
  byte*         cache,
  int           originy,
  int           cacheheight )
{
    int         count;
    int         position;
    byte*       source;

    while (patch->topdelta != 0xff)
    {
        source = (byte *)patch + 3;
        count = patch->length;
        position = originy + patch->topdelta;

        if (position < 0)
        {
            count += position;
            position = 0;
        }

        if (position + count > cacheheight)
            count = cacheheight - position;

        if (count > 0)
            memcpy (cache + position, source, count);

        patch = (column_t *)(  (byte *)patch + patch->length + 4);
    }
}



void R_GenerateComposite (int texnum)
{
    byte*               block;
    texture_t*          texture;
    texpatch_t*         patch;
    patch_t*            realpatch;
    int                 x;
    int                 x1;
    int                 x2;
    int                 i;
    column_t*           patchcol;
    short*              collump;
    unsigned short*     colofs;

    texture = textures[texnum];

    block = Z_Malloc (texturecompositesize[texnum],
                      PU_STATIC,
                      &texturecomposite[texnum]);

    collump = texturecolumnlump[texnum];
    colofs = texturecolumnofs[texnum];

    patch = texture->patches;

    for (i=0 , patch = texture->patches;
         i<texture->patchcount;
         i++, patch++)
    {
        realpatch = W_CacheLumpNum (patch->patch, PU_CACHE);
        x1 = patch->originx;
        x2 = x1 + SHORT(realpatch->width);

        if (x1<0)
            x = 0;
        else
            x = x1;

        if (x2 > texture->width)
            x2 = texture->width;

        for ( ; x<x2 ; x++)
        {
            if (collump[x] >= 0)
                continue;

            patchcol = (column_t *)((byte *)realpatch
                                    + LONG(realpatch->columnofs[x-x1]));
            R_DrawColumnInCache (patchcol,
                                 block + colofs[x],
                                 patch->originy,
                                 texture->height);
        }

    }

    Z_ChangeTag (block, PU_CACHE);
}



void R_GenerateLookup (int texnum)
{
    texture_t*          texture;
    byte*               patchcount;
    texpatch_t*         patch;
    patch_t*            realpatch;
    int                 x;
    int                 x1;
    int                 x2;
    int                 i;
    short*              collump;
    unsigned short*     colofs;

    texture = textures[texnum];

    texturecomposite[texnum] = 0;

    texturecompositesize[texnum] = 0;
    collump = texturecolumnlump[texnum];
    colofs = texturecolumnofs[texnum];

    patchcount = (byte *)alloca (texture->width);
    memset (patchcount, 0, texture->width);

    patch = texture->patches;

    for (i=0 , patch = texture->patches;
         i<texture->patchcount;
         i++, patch++)
    {
        realpatch = W_CacheLumpNum (patch->patch, PU_CACHE);
        x1 = patch->originx;
        x2 = x1 + SHORT(realpatch->width);

        if (x1 < 0)
            x = 0;
        else
            x = x1;

        if (x2 > texture->width)
            x2 = texture->width;
        for ( ; x<x2 ; x++)
        {
            patchcount[x]++;
            collump[x] = patch->patch;
            colofs[x] = LONG(realpatch->columnofs[x-x1])+3;
        }
    }

    for (x=0 ; x<texture->width ; x++)
    {
        if (!patchcount[x])
        {
            printf ("R_GenerateLookup: column without a patch (%s)\n",
                    texture->name);
            return;
        }

        if (patchcount[x] > 1)
        {
            collump[x] = -1;
            colofs[x] = texturecompositesize[texnum];

            if (texturecompositesize[texnum] > 0x10000-texture->height)
            {
                I_Error ("R_GenerateLookup: texture %i is >64k",
                         texnum);
            }

            texturecompositesize[texnum] += texture->height;
        }
    }
}




byte*
R_GetColumn
( int           tex,
  int           col )
{
    int         lump;
    int         ofs;

    col &= texturewidthmask[tex];
    lump = texturecolumnlump[tex][col];
    ofs = texturecolumnofs[tex][col];

    if (lump > 0)
        return (byte *)W_CacheLumpNum(lump,PU_CACHE)+ofs;

    if (!texturecomposite[tex])
        R_GenerateComposite (tex);

    return texturecomposite[tex] + ofs;
}




void R_InitTextures (void)
{
#ifdef ZCORE_DOOM
    console_puts ("[UART] R_InitTextures: enter\r\n");
#endif
    maptexture_t*       mtexture;
    texture_t*          texture;
    mappatch_t*         mpatch;
    texpatch_t*         patch;

    int                 i;
    int                 j;

    int*                maptex;
    int*                maptex2;
    int*                maptex1;

    char                name[9];
    char*               names;
    char*               name_p;

    int*                patchlookup;

    int                 totalwidth;
    int                 nummappatches;
    int                 offset;
    int                 maxoff;
    int                 maxoff2;
    int                 numtextures1;
    int                 numtextures2;

    int*                directory;

    int                 temp1;
    int                 temp2;
    int                 temp3;


    name[8] = 0;
    names = W_CacheLumpName ("PNAMES", PU_STATIC);
    {
        int nraw;
        memcpy (&nraw, names, sizeof (nraw));
        nummappatches = LONG (nraw);
    }
    name_p = names+4;
    patchlookup = alloca (nummappatches*sizeof(*patchlookup));

#ifdef ZCORE_DOOM
    console_printf ("[UART] R_InitTextures: nummappatches=%d names=0x%08x\r\n",
                    nummappatches, (unsigned)(unsigned long)names);
#endif
    for (i=0 ; i<nummappatches ; i++)
    {
        strncpy (name,name_p+i*8, 8);
        patchlookup[i] = W_CheckNumForName (name);
#ifdef ZCORE_DOOM
        if ((i & 0x3F) == 0) {
            console_printf ("[UART] R_InitTextures: patch %d/%d -> %d\r\n",
                            i, nummappatches, patchlookup[i]);
        }
#endif
    }
#ifdef ZCORE_DOOM
    console_printf ("[UART] R_InitTextures: PNAMES done, patches=%d (0x%08x)\r\n",
                    nummappatches, (unsigned)nummappatches);
#endif
    Z_Free (names);

    maptex = maptex1 = W_CacheLumpName ("TEXTURE1", PU_STATIC);
    memcpy (&numtextures1, maptex, sizeof (numtextures1));
    numtextures1 = LONG (numtextures1);
    maxoff = W_LumpLength (W_GetNumForName ("TEXTURE1"));
    directory = maptex+1;

    if (W_CheckNumForName ("TEXTURE2") != -1)
    {
        maptex2 = W_CacheLumpName ("TEXTURE2", PU_STATIC);
        memcpy (&numtextures2, maptex2, sizeof (numtextures2));
        numtextures2 = LONG (numtextures2);
        maxoff2 = W_LumpLength (W_GetNumForName ("TEXTURE2"));
    }
    else
    {
        maptex2 = NULL;
        numtextures2 = 0;
        maxoff2 = 0;
    }
    numtextures = numtextures1 + numtextures2;

    textures = Z_Malloc (numtextures*4, PU_STATIC, 0);
    texturecolumnlump = Z_Malloc (numtextures*4, PU_STATIC, 0);
    texturecolumnofs = Z_Malloc (numtextures*4, PU_STATIC, 0);
    texturecomposite = Z_Malloc (numtextures*4, PU_STATIC, 0);
    texturecompositesize = Z_Malloc (numtextures*4, PU_STATIC, 0);
    texturewidthmask = Z_Malloc (numtextures*4, PU_STATIC, 0);
    textureheight = Z_Malloc (numtextures*4, PU_STATIC, 0);

    totalwidth = 0;

    temp1 = W_GetNumForName ("S_START");
    temp2 = W_GetNumForName ("S_END") - 1;
    temp3 = ((temp2-temp1+63)/64) + ((numtextures+63)/64);
    printf("[");
    for (i = 0; i < temp3; i++)
        printf(" ");
    printf("         ]");
    for (i = 0; i < temp3; i++)
        printf("\x8");
    printf("\x8\x8\x8\x8\x8\x8\x8\x8\x8\x8");

    for (i=0 ; i<numtextures ; i++, directory++)
    {
        if (!(i&63))
            printf (".");

#ifdef ZCORE_DOOM
        if (i < 8) {
            int tex_off;
            memcpy(&tex_off, directory, sizeof(tex_off));
            console_printf("[DIAG] tex[%d] off=%d\r\n", i, LONG(tex_off));
        }
#endif

        if (i == numtextures1)
        {
            maptex = maptex2;
            maxoff = maxoff2;
            directory = maptex+1;
        }

        memcpy (&offset, directory, sizeof (offset));
        offset = LONG (offset);

        if (offset > maxoff)
            I_Error ("R_InitTextures: bad texture directory");

        {
        unsigned       tex_pc;
        unsigned short wh_raw;

        {
            byte        *src = (byte *)maptex + offset;
            int          room = (int)(((byte *)maptex + maxoff) - src);
            unsigned     pc;
            size_t       mtsz;
            size_t       txextra;
            int          alloc_sz;
            unsigned short raw16;

            enum {
                MT_OFF_WIDTH      = 12,
                MT_OFF_HEIGHT     = 14,
                MT_OFF_PATCHCOUNT = 20,
                MT_HEADER_BYTES   = 22,
                MT_PATCH_BYTES    = (int) sizeof (mappatch_t)
            };

            if (room < MT_HEADER_BYTES)
                I_Error ("R_InitTextures: texture past lump end");

            memcpy (&raw16, src + MT_OFF_PATCHCOUNT, sizeof (raw16));
            pc = (unsigned) SHORT ((short) raw16);
            if (pc < 1u || pc > 2048u)
                I_Error ("R_InitTextures: bad patchcount %u", pc);

            mtsz = (size_t) MT_HEADER_BYTES + (size_t) pc * (size_t) MT_PATCH_BYTES;
            if ((size_t) room < mtsz)
                I_Error ("R_InitTextures: truncated texture entry");

            mtexture = (maptexture_t *) Z_Malloc ((int)mtsz, PU_STATIC, 0);
            memcpy (mtexture, src, mtsz);

            txextra = (size_t)(pc - 1u) * sizeof (texpatch_t);
            if (txextra > (size_t)(INT_MAX - (int)sizeof (texture_t)))
                I_Error ("R_InitTextures: texture alloc overflow");
            alloc_sz = (int)(sizeof (texture_t) + txextra);
            texture = textures[i] = Z_Malloc (alloc_sz, PU_STATIC, 0);
            tex_pc = pc;
        }

        memcpy (&wh_raw, (byte *) mtexture + 12, sizeof (wh_raw));
        texture->width = SHORT ((short) wh_raw);
        memcpy (&wh_raw, (byte *) mtexture + 14, sizeof (wh_raw));
        texture->height = SHORT ((short) wh_raw);
        texture->patchcount = (short) tex_pc;

        memcpy (texture->name, mtexture->name, sizeof (texture->name));
        mpatch = (mappatch_t *) ((byte *) mtexture + 22);
        patch = &texture->patches[0];

        for (j=0 ; j<texture->patchcount ; j++, mpatch++, patch++)
        {
            int pidx;
            char tn[9];
            memcpy (tn, texture->name, 8);
            tn[8] = '\0';
            patch->originx = SHORT(mpatch->originx);
            patch->originy = SHORT(mpatch->originy);
            pidx = (int) SHORT (mpatch->patch);
            if (pidx < 0 || pidx >= nummappatches)
            {
                printf ("R_InitTextures: bad PNAMES index %d in texture %s\n",
                        pidx, tn);
                patch->patch = 0;
            }
            else
            {
                patch->patch = patchlookup[pidx];
            }
            if (patch->patch < 0 || patch->patch >= numlumps)
            {
                printf ("R_InitTextures: Missing patch in texture %s\n",
                         tn);
                patch->patch = 0;
            }
        }
        Z_Free (mtexture);
        texturecolumnlump[i] = Z_Malloc (texture->width*2, PU_STATIC,0);
        texturecolumnofs[i] = Z_Malloc (texture->width*2, PU_STATIC,0);

        j = 1;
        while (j*2 <= texture->width)
            j<<=1;

        texturewidthmask[i] = j-1;
        textureheight[i] = texture->height<<FRACBITS;

        totalwidth += texture->width;
        }
    }

    Z_Free (maptex1);
    if (maptex2)
        Z_Free (maptex2);

#ifdef ZCORE_DOOM
    console_printf ("[UART] R_InitTextures: leave numtextures=%d\r\n",
                    numtextures);
#endif

    for (i=0 ; i<numtextures ; i++)
        R_GenerateLookup (i);

    texturetranslation = Z_Malloc ((numtextures+1)*4, PU_STATIC, 0);

    for (i=0 ; i<numtextures ; i++)
        texturetranslation[i] = i;
}



void R_InitFlats (void)
{
    int         i;

    firstflat = W_GetNumForName ("F_START") + 1;
    lastflat = W_GetNumForName ("F_END") - 1;
    numflats = lastflat - firstflat + 1;

    flattranslation = Z_Malloc ((numflats+1)*4, PU_STATIC, 0);

    for (i=0 ; i<numflats ; i++)
        flattranslation[i] = i;
}


void R_InitSpriteLumps (void)
{
    int         i;
    patch_t     *patch;

    firstspritelump = W_GetNumForName ("S_START") + 1;
    lastspritelump = W_GetNumForName ("S_END") - 1;

    numspritelumps = lastspritelump - firstspritelump + 1;
    spritewidth = Z_Malloc (numspritelumps*4, PU_STATIC, 0);
    spriteoffset = Z_Malloc (numspritelumps*4, PU_STATIC, 0);
    spritetopoffset = Z_Malloc (numspritelumps*4, PU_STATIC, 0);

    for (i=0 ; i< numspritelumps ; i++)
    {
        if (!(i&63))
            printf (".");

        patch = W_CacheLumpNum (firstspritelump+i, PU_CACHE);
        spritewidth[i] = SHORT(patch->width)<<FRACBITS;
        spriteoffset[i] = SHORT(patch->leftoffset)<<FRACBITS;
        spritetopoffset[i] = SHORT(patch->topoffset)<<FRACBITS;
    }
}



void R_InitColormaps (void)
{
    int lump, length;

    lump = W_GetNumForName("COLORMAP");
    length = W_LumpLength (lump) + 255;
    colormaps = Z_Malloc (length, PU_STATIC, 0);
    colormaps = (byte *)( ((int)colormaps + 255)&~0xff);
    W_ReadLump (lump,colormaps);
}



void R_InitData (void)
{
#ifdef ZCORE_DOOM
    console_puts ("[UART] R_InitData: enter\r\n");
#endif
    R_InitTextures ();
    printf ("\nInitTextures");
#ifdef ZCORE_DOOM
    console_puts ("[UART] R_InitData: after InitTextures\r\n");
#endif
    R_InitFlats ();
    printf ("\nInitFlats");
#ifdef ZCORE_DOOM
    console_puts ("[UART] R_InitData: after InitFlats\r\n");
#endif
    R_InitSpriteLumps ();
    printf ("\nInitSprites");
#ifdef ZCORE_DOOM
    console_puts ("[UART] R_InitData: after InitSprites\r\n");
#endif
    R_InitColormaps ();
    printf ("\nInitColormaps");
#ifdef ZCORE_DOOM
    console_puts ("[UART] R_InitData: leave\r\n");
#endif
}



int R_FlatNumForName (char* name)
{
    int         i;
    char        namet[9];

    i = W_CheckNumForName (name);

    if (i == -1)
    {
        namet[8] = 0;
        memcpy (namet, name,8);
        I_Error ("R_FlatNumForName: %s not found",namet);
    }
    return i - firstflat;
}




int     R_CheckTextureNumForName (char *name)
{
    int         i;

    if (name[0] == '-')
        return 0;

    for (i=0 ; i<numtextures ; i++)
        if (!strncasecmp (textures[i]->name, name, 8) )
            return i;

    return -1;
}



int     R_TextureNumForName (char* name)
{
    int         i;

    i = R_CheckTextureNumForName (name);

    if (i==-1)
    {
        I_Error ("R_TextureNumForName: %s not found",
                 name);
    }
    return i;
}




int             flatmemory;
int             texturememory;
int             spritememory;

void R_PrecacheLevel (void)
{
    char*               flatpresent;
    char*               texturepresent;
    char*               spritepresent;

    int                 i;
    int                 j;
    int                 k;
    int                 lump;

    texture_t*          texture;
    thinker_t*          th;
    spriteframe_t*      sf;

    if (demoplayback)
        return;

    flatpresent = alloca(numflats);
    memset (flatpresent,0,numflats);

    for (i=0 ; i<numsectors ; i++)
    {
        flatpresent[sectors[i].floorpic] = 1;
        flatpresent[sectors[i].ceilingpic] = 1;
    }

    flatmemory = 0;

    for (i=0 ; i<numflats ; i++)
    {
        if (flatpresent[i])
        {
            lump = firstflat + i;
            flatmemory += lumpinfo[lump].size;
            W_CacheLumpNum(lump, PU_CACHE);
        }
    }

    texturepresent = alloca(numtextures);
    memset (texturepresent,0, numtextures);

    for (i=0 ; i<numsides ; i++)
    {
        texturepresent[sides[i].toptexture] = 1;
        texturepresent[sides[i].midtexture] = 1;
        texturepresent[sides[i].bottomtexture] = 1;
    }

    texturepresent[skytexture] = 1;

    texturememory = 0;
    for (i=0 ; i<numtextures ; i++)
    {
        if (!texturepresent[i])
            continue;

        texture = textures[i];

        for (j=0 ; j<texture->patchcount ; j++)
        {
            lump = texture->patches[j].patch;
            texturememory += lumpinfo[lump].size;
            W_CacheLumpNum(lump , PU_CACHE);
        }
    }

    spritepresent = alloca(numsprites);
    memset (spritepresent,0, numsprites);

    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
        if (th->function.acp1 == (actionf_p1)P_MobjThinker)
            spritepresent[((mobj_t *)th)->sprite] = 1;
    }

    spritememory = 0;
    for (i=0 ; i<numsprites ; i++)
    {
        if (!spritepresent[i])
            continue;

        for (j=0 ; j<sprites[i].numframes ; j++)
        {
            sf = &sprites[i].spriteframes[j];
            for (k=0 ; k<8 ; k++)
            {
                lump = firstspritelump + sf->lump[k];
                spritememory += lumpinfo[lump].size;
                W_CacheLumpNum(lump , PU_CACHE);
            }
        }
    }
}




