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
//      Refresh of things, i.e. objects represented by sprites.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: r_things.c,v 1.5 1997/02/03 16:47:56 b1 Exp $";


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomdef.h"
#include "m_swap.h"

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

#include "doomstat.h"



#define MINZ                            (FRACUNIT*4)
#define BASEYCENTER                     100




typedef struct
{
    int         x1;
    int         x2;

    int         column;
    int         topclip;
    int         bottomclip;

} maskdraw_t;



fixed_t         pspritescale;
fixed_t         pspriteiscale;

lighttable_t**  spritelights;

short           negonearray[SCREENWIDTH];
short           screenheightarray[SCREENWIDTH];



spritedef_t*    sprites;
int             numsprites;

spriteframe_t   sprtemp[29];
int             maxframe;
char*           spritename;




void
R_InstallSpriteLump
( int           lump,
  unsigned      frame,
  unsigned      rotation,
  boolean       flipped )
{
    int         r;

    if (frame >= 29 || rotation > 8)
        I_Error("R_InstallSpriteLump: "
                "Bad frame characters in lump %i (%.8s, frame %u, rotation %u)",
                lump, lumpinfo[lump].name, frame, rotation);

    if ((int)frame > maxframe)
        maxframe = frame;

    if (rotation == 0)
    {
        if (sprtemp[frame].rotate == false)
            I_Error ("R_InitSprites: Sprite %s frame %c has "
                     "multip rot=0 lump", spritename, 'A'+frame);

        if (sprtemp[frame].rotate == true)
            I_Error ("R_InitSprites: Sprite %s frame %c has rotations "
                     "and a rot=0 lump", spritename, 'A'+frame);

        sprtemp[frame].rotate = false;
        for (r=0 ; r<8 ; r++)
        {
            sprtemp[frame].lump[r] = lump - firstspritelump;
            sprtemp[frame].flip[r] = (byte)flipped;
        }
        return;
    }

    if (sprtemp[frame].rotate == false)
        I_Error ("R_InitSprites: Sprite %s frame %c has rotations "
                 "and a rot=0 lump", spritename, 'A'+frame);

    sprtemp[frame].rotate = true;

    rotation--;
    if (sprtemp[frame].lump[rotation] != -1)
        I_Error ("R_InitSprites: Sprite %s : %c : %c "
                 "has two lumps mapped to it",
                 spritename, 'A'+frame, '1'+rotation);

    sprtemp[frame].lump[rotation] = lump - firstspritelump;
    sprtemp[frame].flip[rotation] = (byte)flipped;
}




void R_InitSpriteDefs (char** namelist)
{
    char**      check;
    int         i;
    int         l;
    int         intname;
    int         frame;
    int         rotation;
    int         start;
    int         end;
    int         patched;

    check = namelist;
    while (*check != NULL)
        check++;

    numsprites = check-namelist;

    if (!numsprites)
        return;

    sprites = Z_Malloc(numsprites *sizeof(*sprites), PU_STATIC, NULL);

    start = firstspritelump-1;
    end = lastspritelump+1;

    for (i=0 ; i<numsprites ; i++)
    {
        spritename = namelist[i];
        memset (sprtemp,-1, sizeof(sprtemp));

        maxframe = -1;
        memcpy (&intname, namelist[i], sizeof (intname));

        for (l=start+1 ; l<end ; l++)
        {
            int lump4;
            memcpy (&lump4, lumpinfo[l].name, sizeof (lump4));
            if (lump4 == intname)
            {
                frame = lumpinfo[l].name[4] - 'A';
                rotation = lumpinfo[l].name[5] - '0';

                if (modifiedgame)
                    patched = W_GetNumForName (lumpinfo[l].name);
                else
                    patched = l;

                R_InstallSpriteLump (patched, frame, rotation, false);

                if (lumpinfo[l].name[6])
                {
                    frame = lumpinfo[l].name[6] - 'A';
                    rotation = lumpinfo[l].name[7] - '0';
                    R_InstallSpriteLump (l, frame, rotation, true);
                }
            }
        }

        if (maxframe == -1)
        {
            sprites[i].numframes = 0;
            continue;
        }

        maxframe++;

        for (frame = 0 ; frame < maxframe ; frame++)
        {
            switch ((int)sprtemp[frame].rotate)
            {
              case -1:
                I_Error ("R_InitSprites: No patches found "
                         "for %s frame %c", namelist[i], frame+'A');
                break;

              case 0:
                break;

              case 1:
                for (rotation=0 ; rotation<8 ; rotation++)
                    if (sprtemp[frame].lump[rotation] == -1)
                        I_Error ("R_InitSprites: Sprite %s frame %c "
                                 "is missing rotations",
                                 namelist[i], frame+'A');
                break;
            }
        }

        sprites[i].numframes = maxframe;
        sprites[i].spriteframes =
            Z_Malloc (maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
        memcpy (sprites[i].spriteframes, sprtemp, maxframe*sizeof(spriteframe_t));
    }

}




vissprite_t     vissprites[MAXVISSPRITES];
vissprite_t*    vissprite_p;
int             newvissprite;



void R_InitSprites (char** namelist)
{
    int         i;

    for (i=0 ; i<SCREENWIDTH ; i++)
    {
        negonearray[i] = -1;
    }

    R_InitSpriteDefs (namelist);
}



void R_ClearSprites (void)
{
    vissprite_p = vissprites;
}


vissprite_t     overflowsprite;

vissprite_t* R_NewVisSprite (void)
{
    if (vissprite_p == &vissprites[MAXVISSPRITES])
        return &overflowsprite;

    vissprite_p++;
    return vissprite_p-1;
}



short*          mfloorclip;
short*          mceilingclip;

fixed_t         spryscale;
fixed_t         sprtopscreen;

void R_DrawMaskedColumn (column_t* column)
{
    int         topscreen;
    int         bottomscreen;
    fixed_t     basetexturemid;

    basetexturemid = dc_texturemid;

    for ( ; column->topdelta != 0xff ; )
    {
        topscreen = sprtopscreen + spryscale*column->topdelta;
        bottomscreen = topscreen + spryscale*column->length;

        dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
        dc_yh = (bottomscreen-1)>>FRACBITS;

        if (dc_yh >= mfloorclip[dc_x])
            dc_yh = mfloorclip[dc_x]-1;
        if (dc_yl <= mceilingclip[dc_x])
            dc_yl = mceilingclip[dc_x]+1;

        if (dc_yl <= dc_yh)
        {
            dc_source = (byte *)column + 3;
            dc_texturemid = basetexturemid - (column->topdelta<<FRACBITS);

            colfunc ();
        }
        column = (column_t *)(  (byte *)column + column->length + 4);
    }

    dc_texturemid = basetexturemid;
}



void
R_DrawVisSprite
( vissprite_t*          vis,
  int                   x1,
  int                   x2 )
{
    column_t*           column;
    int                 texturecolumn;
    fixed_t             frac;
    patch_t*            patch;


    patch = W_CacheLumpNum (vis->patch+firstspritelump, PU_CACHE);

    dc_colormap = vis->colormap;

    if (!dc_colormap)
    {
        colfunc = fuzzcolfunc;
    }
    else if (vis->mobjflags & MF_TRANSLATION)
    {
        colfunc = R_DrawTranslatedColumn;
        dc_translation = translationtables - 256 +
            ( (vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8) );
    }

    dc_iscale = abs(vis->xiscale)>>detailshift;
    dc_texturemid = vis->texturemid;
    frac = vis->startfrac;
    spryscale = vis->scale;
    sprtopscreen = centeryfrac - FixedMul(dc_texturemid,spryscale);

    for (dc_x=vis->x1 ; dc_x<=vis->x2 ; dc_x++, frac += vis->xiscale)
    {
        texturecolumn = frac>>FRACBITS;
#ifdef RANGECHECK
        if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
            I_Error ("R_DrawSpriteRange: bad texturecolumn");
#endif
        column = (column_t *) ((byte *)patch +
                               LONG(patch->columnofs[texturecolumn]));
        R_DrawMaskedColumn (column);
    }

    colfunc = basecolfunc;
}



void R_ProjectSprite (mobj_t* thing)
{
    fixed_t             tr_x;
    fixed_t             tr_y;

    fixed_t             gxt;
    fixed_t             gyt;

    fixed_t             tx;
    fixed_t             tz;

    fixed_t             xscale;

    int                 x1;
    int                 x2;

    spritedef_t*        sprdef;
    spriteframe_t*      sprframe;
    int                 lump;

    unsigned            rot;
    boolean             flip;

    int                 index;

    vissprite_t*        vis;

    angle_t             ang;
    fixed_t             iscale;

    tr_x = thing->x - viewx;
    tr_y = thing->y - viewy;

    gxt = FixedMul(tr_x,viewcos);
    gyt = -FixedMul(tr_y,viewsin);

    tz = gxt-gyt;

    if (tz < MINZ)
        return;

    xscale = FixedDiv(projection, tz);

    gxt = -FixedMul(tr_x,viewsin);
    gyt = FixedMul(tr_y,viewcos);
    tx = -(gyt+gxt);

    if (abs(tx)>(tz<<2))
        return;

#ifdef RANGECHECK
    if ((unsigned)thing->sprite >= numsprites)
        I_Error ("R_ProjectSprite: invalid sprite number %i ",
                 thing->sprite);
#endif
    sprdef = &sprites[thing->sprite];
#ifdef RANGECHECK
    if ( (thing->frame&FF_FRAMEMASK) >= sprdef->numframes )
        I_Error ("R_ProjectSprite: invalid sprite frame %i : %i ",
                 thing->sprite, thing->frame);
#endif
    sprframe = &sprdef->spriteframes[ thing->frame & FF_FRAMEMASK];

    if (sprframe->rotate)
    {
        ang = R_PointToAngle (thing->x, thing->y);
        rot = (ang-thing->angle+(unsigned)(ANG45/2)*9)>>29;
        lump = sprframe->lump[rot];
        flip = (boolean)sprframe->flip[rot];
    }
    else
    {
        lump = sprframe->lump[0];
        flip = (boolean)sprframe->flip[0];
    }

    tx -= spriteoffset[lump];
    x1 = (centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS;

    if (x1 > viewwidth)
        return;

    tx +=  spritewidth[lump];
    x2 = ((centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS) - 1;

    if (x2 < 0)
        return;

    vis = R_NewVisSprite ();
    vis->mobjflags = thing->flags;
    vis->scale = xscale<<detailshift;
    vis->gx = thing->x;
    vis->gy = thing->y;
    vis->gz = thing->z;
    vis->gzt = thing->z + spritetopoffset[lump];
    vis->texturemid = vis->gzt - viewz;
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
    iscale = FixedDiv (FRACUNIT, xscale);

    if (flip)
    {
        vis->startfrac = spritewidth[lump]-1;
        vis->xiscale = -iscale;
    }
    else
    {
        vis->startfrac = 0;
        vis->xiscale = iscale;
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale*(vis->x1-x1);
    vis->patch = lump;

    if (thing->flags & MF_SHADOW)
    {
        vis->colormap = NULL;
    }
    else if (fixedcolormap)
    {
        vis->colormap = fixedcolormap;
    }
    else if (thing->frame & FF_FULLBRIGHT)
    {
        vis->colormap = colormaps;
    }

    else
    {
        index = xscale>>(LIGHTSCALESHIFT-detailshift);

        if (index >= MAXLIGHTSCALE)
            index = MAXLIGHTSCALE-1;

        vis->colormap = spritelights[index];
    }
}




void R_AddSprites (sector_t* sec)
{
    mobj_t*             thing;
    int                 lightnum;

    if (sec->validcount == validcount)
        return;

    sec->validcount = validcount;

    lightnum = (sec->lightlevel >> LIGHTSEGSHIFT)+extralight;

    if (lightnum < 0)
        spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
    else
        spritelights = scalelight[lightnum];

    for (thing = sec->thinglist ; thing ; thing = thing->snext)
        R_ProjectSprite (thing);
}


void R_DrawPSprite (pspdef_t* psp)
{
    fixed_t             tx;
    int                 x1;
    int                 x2;
    spritedef_t*        sprdef;
    spriteframe_t*      sprframe;
    int                 lump;
    boolean             flip;
    vissprite_t*        vis;
    vissprite_t         avis;

#ifdef RANGECHECK
    if ( (unsigned)psp->state->sprite >= numsprites)
        I_Error ("R_ProjectSprite: invalid sprite number %i ",
                 psp->state->sprite);
#endif
    sprdef = &sprites[psp->state->sprite];
#ifdef RANGECHECK
    if ( (psp->state->frame & FF_FRAMEMASK)  >= sprdef->numframes)
        I_Error ("R_ProjectSprite: invalid sprite frame %i : %i ",
                 psp->state->sprite, psp->state->frame);
#endif
    sprframe = &sprdef->spriteframes[ psp->state->frame & FF_FRAMEMASK ];

    lump = sprframe->lump[0];
    flip = (boolean)sprframe->flip[0];

    tx = psp->sx-160*FRACUNIT;

    tx -= spriteoffset[lump];
    x1 = (centerxfrac + FixedMul (tx,pspritescale) ) >>FRACBITS;

    if (x1 > viewwidth)
        return;

    tx +=  spritewidth[lump];
    x2 = ((centerxfrac + FixedMul (tx, pspritescale) ) >>FRACBITS) - 1;

    if (x2 < 0)
        return;

    vis = &avis;
    vis->mobjflags = 0;
    vis->texturemid = (BASEYCENTER<<FRACBITS)+FRACUNIT/2-(psp->sy-spritetopoffset[lump]);
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
    vis->scale = pspritescale<<detailshift;

    if (flip)
    {
        vis->xiscale = -pspriteiscale;
        vis->startfrac = spritewidth[lump]-1;
    }
    else
    {
        vis->xiscale = pspriteiscale;
        vis->startfrac = 0;
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale*(vis->x1-x1);

    vis->patch = lump;

    if (viewplayer->powers[pw_invisibility] > 4*32
        || viewplayer->powers[pw_invisibility] & 8)
    {
        vis->colormap = NULL;
    }
    else if (fixedcolormap)
    {
        vis->colormap = fixedcolormap;
    }
    else if (psp->state->frame & FF_FULLBRIGHT)
    {
        vis->colormap = colormaps;
    }
    else
    {
        vis->colormap = spritelights[MAXLIGHTSCALE-1];
    }

    R_DrawVisSprite (vis, vis->x1, vis->x2);
}



void R_DrawPlayerSprites (void)
{
    int         i;
    int         lightnum;
    pspdef_t*   psp;

    lightnum =
        (viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT)
        +extralight;

    if (lightnum < 0)
        spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
    else
        spritelights = scalelight[lightnum];

    mfloorclip = screenheightarray;
    mceilingclip = negonearray;

    for (i=0, psp=viewplayer->psprites;
         i<NUMPSPRITES;
         i++,psp++)
    {
        if (psp->state)
            R_DrawPSprite (psp);
    }
}




vissprite_t     vsprsortedhead;

static vissprite_t r_vspr_unsorted;

void R_SortVisSprites (void)
{
    int                 i;
    int                 count;
    vissprite_t*        ds;
    vissprite_t*        best;
    vissprite_t*        unsorted;
    fixed_t             bestscale;

    unsorted = &r_vspr_unsorted;

    count = vissprite_p - vissprites;

    unsorted->next = unsorted->prev = unsorted;

    if (!count)
        return;

    for (ds=vissprites ; ds<vissprite_p ; ds++)
    {
        ds->next = ds+1;
        ds->prev = ds-1;
    }

    vissprites[0].prev = unsorted;
    unsorted->next = &vissprites[0];
    (vissprite_p-1)->next = unsorted;
    unsorted->prev = vissprite_p-1;

    vsprsortedhead.next = vsprsortedhead.prev = &vsprsortedhead;
    for (i=0 ; i<count ; i++)
    {
        bestscale = MAXINT;
        for (ds=unsorted->next ; ds != unsorted ; ds=ds->next)
        {
            if (ds->scale < bestscale)
            {
                bestscale = ds->scale;
                best = ds;
            }
        }
        best->next->prev = best->prev;
        best->prev->next = best->next;
        best->next = &vsprsortedhead;
        best->prev = vsprsortedhead.prev;
        vsprsortedhead.prev->next = best;
        vsprsortedhead.prev = best;
    }
}



void R_DrawSprite (vissprite_t* spr)
{
    drawseg_t*          ds;
    short               clipbot[SCREENWIDTH];
    short               cliptop[SCREENWIDTH];
    int                 x;
    int                 r1;
    int                 r2;
    fixed_t             scale;
    fixed_t             lowscale;
    int                 silhouette;

    for (x = spr->x1 ; x<=spr->x2 ; x++)
        clipbot[x] = cliptop[x] = -2;

    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)
    {
        if (ds->x1 > spr->x2
            || ds->x2 < spr->x1
            || (!ds->silhouette
                && !ds->maskedtexturecol) )
        {
            continue;
        }

        r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
        r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

        if (ds->scale1 > ds->scale2)
        {
            lowscale = ds->scale2;
            scale = ds->scale1;
        }
        else
        {
            lowscale = ds->scale1;
            scale = ds->scale2;
        }

        if (scale < spr->scale
            || ( lowscale < spr->scale
                 && !R_PointOnSegSide (spr->gx, spr->gy, ds->curline) ) )
        {
            if (ds->maskedtexturecol)
                R_RenderMaskedSegRange (ds, r1, r2);
            continue;
        }


        silhouette = ds->silhouette;

        if (spr->gz >= ds->bsilheight)
            silhouette &= ~SIL_BOTTOM;

        if (spr->gzt <= ds->tsilheight)
            silhouette &= ~SIL_TOP;

        if (silhouette == 1)
        {
            for (x=r1 ; x<=r2 ; x++)
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
        }
        else if (silhouette == 2)
        {
            for (x=r1 ; x<=r2 ; x++)
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
        }
        else if (silhouette == 3)
        {
            for (x=r1 ; x<=r2 ; x++)
            {
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
            }
        }

    }


    for (x = spr->x1 ; x<=spr->x2 ; x++)
    {
        if (clipbot[x] == -2)
            clipbot[x] = viewheight;

        if (cliptop[x] == -2)
            cliptop[x] = -1;
    }

    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawVisSprite (spr, spr->x1, spr->x2);
}




void R_DrawMasked (void)
{
    vissprite_t*        spr;
    drawseg_t*          ds;

    R_SortVisSprites ();

    if (vissprite_p > vissprites)
    {
        for (spr = vsprsortedhead.next ;
             spr != &vsprsortedhead ;
             spr=spr->next)
        {

            R_DrawSprite (spr);
        }
    }

    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)
        if (ds->maskedtexturecol)
            R_RenderMaskedSegRange (ds, ds->x1, ds->x2);

    if (!viewangleoffset)
        R_DrawPlayerSprites ();
}



