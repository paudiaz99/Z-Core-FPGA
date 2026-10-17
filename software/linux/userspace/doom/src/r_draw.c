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
//      The actual span/column drawing functions.
//      Here find the main potential for optimization,
//       e.g. inline assembly, different algorithms.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: r_draw.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";


#include "doomdef.h"

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

#include "v_video.h"

#include "doomstat.h"


#define MAXWIDTH                        1120
#define MAXHEIGHT                       832

#define SBARHEIGHT              32



byte*           viewimage;
int             viewwidth;
int             scaledviewwidth;
int             viewheight;
int             viewwindowx;
int             viewwindowy;

byte            translations[3][256];




lighttable_t*           dc_colormap;
int                     dc_x;
int                     dc_yl;
int                     dc_yh;
fixed_t                 dc_iscale;
fixed_t                 dc_texturemid;

byte*                   dc_source;

int                     dccount;

void R_DrawColumn (void)
{
    int                 count;
    byte*               dest;
    fixed_t             frac;
    fixed_t             fracstep;

    count = dc_yh - dc_yl;

    if (count < 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0
        || dc_yh >= SCREENHEIGHT)
        I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    dest = screens[0] + (viewwindowy + dc_yl) * SCREENWIDTH + (viewwindowx + dc_x);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    do
    {
        *dest = dc_colormap[dc_source[(frac>>FRACBITS)&127]];

        dest += SCREENWIDTH;
        frac += fracstep;

    } while (count--);
}



void R_DrawColumnLow (void)
{
    int                 count;
    byte*               dest;
    byte*               dest2;
    fixed_t             frac;
    fixed_t             fracstep;

    count = dc_yh - dc_yl;

    if (count < 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0
        || dc_yh >= SCREENHEIGHT)
    {

        I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
    }
#endif
    dc_x <<= 1;

    dest = screens[0] + (viewwindowy + dc_yl) * SCREENWIDTH + (viewwindowx + dc_x);
    dest2 = dest + 1;

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    do
    {
        *dest2 = *dest = dc_colormap[dc_source[(frac>>FRACBITS)&127]];
        dest += SCREENWIDTH;
        dest2 += SCREENWIDTH;
        frac += fracstep;

    } while (count--);
}


#define FUZZTABLE               50
#define FUZZOFF (SCREENWIDTH)


int     fuzzoffset[FUZZTABLE] =
{
    FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF
};

int     fuzzpos = 0;


void R_DrawFuzzColumn (void)
{
    int                 count;
    byte*               dest;
    fixed_t             frac;
    fixed_t             fracstep;

    if (!dc_yl)
        dc_yl = 1;

    if (dc_yh == viewheight-1)
        dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;

    if (count < 0)
        return;


#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0 || dc_yh >= SCREENHEIGHT)
    {
        I_Error ("R_DrawFuzzColumn: %i to %i at %i",
                 dc_yl, dc_yh, dc_x);
    }
#endif




    dest = screens[0] + (viewwindowy + dc_yl) * SCREENWIDTH + (viewwindowx + dc_x);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    do
    {
        *dest = colormaps[6*256+dest[fuzzoffset[fuzzpos]]];

        if (++fuzzpos == FUZZTABLE)
            fuzzpos = 0;

        dest += SCREENWIDTH;

        frac += fracstep;
    } while (count--);
}




byte*   dc_translation;
byte*   translationtables;

void R_DrawTranslatedColumn (void)
{
    int                 count;
    byte*               dest;
    fixed_t             frac;
    fixed_t             fracstep;

    count = dc_yh - dc_yl;
    if (count < 0)
        return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0
        || dc_yh >= SCREENHEIGHT)
    {
        I_Error ( "R_DrawColumn: %i to %i at %i",
                  dc_yl, dc_yh, dc_x);
    }

#endif






    dest = screens[0] + (viewwindowy + dc_yl) * SCREENWIDTH + (viewwindowx + dc_x);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    do
    {
        *dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];
        dest += SCREENWIDTH;

        frac += fracstep;
    } while (count--);
}




void R_InitTranslationTables (void)
{
    int         i;

    translationtables = Z_Malloc (256*3+255, PU_STATIC, 0);
    translationtables = (byte *)(( (int)translationtables + 255 )& ~255);

    for (i=0 ; i<256 ; i++)
    {
        if (i >= 0x70 && i<= 0x7f)
        {
            translationtables[i] = 0x60 + (i&0xf);
            translationtables [i+256] = 0x40 + (i&0xf);
            translationtables [i+512] = 0x20 + (i&0xf);
        }
        else
        {
            translationtables[i] = translationtables[i+256]
                = translationtables[i+512] = i;
        }
    }
}




int                     ds_y;
int                     ds_x1;
int                     ds_x2;

lighttable_t*           ds_colormap;

fixed_t                 ds_xfrac;
fixed_t                 ds_yfrac;
fixed_t                 ds_xstep;
fixed_t                 ds_ystep;

byte*                   ds_source;

int                     dscount;


void R_DrawSpan (void)
{
    fixed_t             xfrac;
    fixed_t             yfrac;
    byte*               dest;
    int                 count;
    int                 spot;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1
        || ds_x1<0
        || ds_x2>=SCREENWIDTH
        || (unsigned)ds_y>SCREENHEIGHT)
    {
        I_Error( "R_DrawSpan: %i to %i at %i",
                 ds_x1,ds_x2,ds_y);
    }
#endif


    xfrac = ds_xfrac;
    yfrac = ds_yfrac;

    dest = screens[0] + (viewwindowy + ds_y) * SCREENWIDTH + (viewwindowx + ds_x1);

    count = ds_x2 - ds_x1;

    do
    {
        spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);

        *dest++ = ds_colormap[ds_source[spot]];

        xfrac += ds_xstep;
        yfrac += ds_ystep;

    } while (count--);
}



void R_DrawSpanLow (void)
{
    fixed_t             xfrac;
    fixed_t             yfrac;
    byte*               dest;
    int                 count;
    int                 spot;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1
        || ds_x1<0
        || ds_x2>=SCREENWIDTH
        || (unsigned)ds_y>SCREENHEIGHT)
    {
        I_Error( "R_DrawSpan: %i to %i at %i",
                 ds_x1,ds_x2,ds_y);
    }
#endif

    xfrac = ds_xfrac;
    yfrac = ds_yfrac;

    ds_x1 <<= 1;
    ds_x2 <<= 1;

    dest = screens[0] + (viewwindowy + ds_y) * SCREENWIDTH + (viewwindowx + ds_x1);


    count = ds_x2 - ds_x1;
    do
    {
        spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);
        *dest++ = ds_colormap[ds_source[spot]];
        *dest++ = ds_colormap[ds_source[spot]];

        xfrac += ds_xstep;
        yfrac += ds_ystep;

    } while (count--);
}

void
R_InitBuffer
( int           width,
  int           height )
{
    viewwindowx = (SCREENWIDTH-width) >> 1;

    if (width == SCREENWIDTH)
        viewwindowy = 0;
    else
        viewwindowy = (SCREENHEIGHT-SBARHEIGHT-height) >> 1;
}




void R_FillBackScreen (void)
{
    byte*       src;
    byte*       dest;
    int         x;
    int         y;
    patch_t*    patch;

    char        name1[] = "FLOOR7_2";

    char        name2[] = "GRNROCK";

    char*       name;

    if (scaledviewwidth == 320)
        return;

    if ( gamemode == commercial)
        name = name2;
    else
        name = name1;

    src = W_CacheLumpName (name, PU_CACHE);
    dest = screens[1];

    for (y=0 ; y<SCREENHEIGHT-SBARHEIGHT ; y++)
    {
        for (x=0 ; x<SCREENWIDTH/64 ; x++)
        {
            memcpy (dest, src+((y&63)<<6), 64);
            dest += 64;
        }

        if (SCREENWIDTH&63)
        {
            memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63);
            dest += (SCREENWIDTH&63);
        }
    }

    patch = W_CacheLumpName ("brdr_t",PU_CACHE);

    for (x=0 ; x<scaledviewwidth ; x+=8)
        V_DrawPatch (viewwindowx+x,viewwindowy-8,1,patch);
    patch = W_CacheLumpName ("brdr_b",PU_CACHE);

    for (x=0 ; x<scaledviewwidth ; x+=8)
        V_DrawPatch (viewwindowx+x,viewwindowy+viewheight,1,patch);
    patch = W_CacheLumpName ("brdr_l",PU_CACHE);

    for (y=0 ; y<viewheight ; y+=8)
        V_DrawPatch (viewwindowx-8,viewwindowy+y,1,patch);
    patch = W_CacheLumpName ("brdr_r",PU_CACHE);

    for (y=0 ; y<viewheight ; y+=8)
        V_DrawPatch (viewwindowx+scaledviewwidth,viewwindowy+y,1,patch);


    V_DrawPatch (viewwindowx-8,
                 viewwindowy-8,
                 1,
                 W_CacheLumpName ("brdr_tl",PU_CACHE));

    V_DrawPatch (viewwindowx+scaledviewwidth,
                 viewwindowy-8,
                 1,
                 W_CacheLumpName ("brdr_tr",PU_CACHE));

    V_DrawPatch (viewwindowx-8,
                 viewwindowy+viewheight,
                 1,
                 W_CacheLumpName ("brdr_bl",PU_CACHE));

    V_DrawPatch (viewwindowx+scaledviewwidth,
                 viewwindowy+viewheight,
                 1,
                 W_CacheLumpName ("brdr_br",PU_CACHE));
}


void
R_VideoErase
( unsigned      ofs,
  int           count )
{
    memcpy (screens[0]+ofs, screens[1]+ofs, count);
}


void
V_MarkRect
( int           x,
  int           y,
  int           width,
  int           height );

void R_DrawViewBorder (void)
{
    int         top;
    int         side;
    int         ofs;
    int         i;

    if (scaledviewwidth == SCREENWIDTH)
        return;

    top = ((SCREENHEIGHT-SBARHEIGHT)-viewheight)/2;
    side = (SCREENWIDTH-scaledviewwidth)/2;

    R_VideoErase (0, top*SCREENWIDTH+side);

    ofs = (viewheight+top)*SCREENWIDTH-side;
    R_VideoErase (ofs, top*SCREENWIDTH+side);

    ofs = top*SCREENWIDTH + SCREENWIDTH-side;
    side <<= 1;

    for (i=1 ; i<viewheight ; i++)
    {
        R_VideoErase (ofs, side);
        ofs += SCREENWIDTH;
    }

    V_MarkRect (0,0,SCREENWIDTH, SCREENHEIGHT-SBARHEIGHT);
}


