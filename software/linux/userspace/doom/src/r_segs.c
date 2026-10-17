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
//      All the clipping: columns, horizontal spans, sky columns.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: r_segs.c,v 1.3 1997/01/29 20:10:19 b1 Exp $";


#include <stdlib.h>

#include "i_system.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_sky.h"



boolean         segtextured;

boolean         markfloor;
boolean         markceiling;

boolean         maskedtexture;
int             toptexture;
int             bottomtexture;
int             midtexture;


angle_t         rw_normalangle;
int             rw_angle1;

int             rw_x;
int             rw_stopx;
angle_t         rw_centerangle;
fixed_t         rw_offset;
fixed_t         rw_distance;
fixed_t         rw_scale;
fixed_t         rw_scalestep;
fixed_t         rw_midtexturemid;
fixed_t         rw_toptexturemid;
fixed_t         rw_bottomtexturemid;

int             worldtop;
int             worldbottom;
int             worldhigh;
int             worldlow;

fixed_t         pixhigh;
fixed_t         pixlow;
fixed_t         pixhighstep;
fixed_t         pixlowstep;

fixed_t         topfrac;
fixed_t         topstep;

fixed_t         bottomfrac;
fixed_t         bottomstep;


lighttable_t**  walllights;

short*          maskedtexturecol;



void
R_RenderMaskedSegRange
( drawseg_t*    ds,
  int           x1,
  int           x2 )
{
    unsigned    index;
    column_t*   col;
    int         lightnum;
    int         texnum;

    curline = ds->curline;
    frontsector = curline->frontsector;
    backsector = curline->backsector;
    texnum = texturetranslation[curline->sidedef->midtexture];

    lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight;

    if (curline->v1->y == curline->v2->y)
        lightnum--;
    else if (curline->v1->x == curline->v2->x)
        lightnum++;

    if (lightnum < 0)
        walllights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        walllights = scalelight[LIGHTLEVELS-1];
    else
        walllights = scalelight[lightnum];

    maskedtexturecol = ds->maskedtexturecol;

    rw_scalestep = ds->scalestep;
    spryscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;
    mfloorclip = ds->sprbottomclip;
    mceilingclip = ds->sprtopclip;

    if (curline->linedef->flags & ML_DONTPEGBOTTOM)
    {
        dc_texturemid = frontsector->floorheight > backsector->floorheight
            ? frontsector->floorheight : backsector->floorheight;
        dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
    }
    else
    {
        dc_texturemid =frontsector->ceilingheight<backsector->ceilingheight
            ? frontsector->ceilingheight : backsector->ceilingheight;
        dc_texturemid = dc_texturemid - viewz;
    }
    dc_texturemid += curline->sidedef->rowoffset;

    if (fixedcolormap)
        dc_colormap = fixedcolormap;

    for (dc_x = x1 ; dc_x <= x2 ; dc_x++)
    {
        if (maskedtexturecol[dc_x] != MAXSHORT)
        {
            if (!fixedcolormap)
            {
                index = spryscale>>LIGHTSCALESHIFT;

                if (index >=  MAXLIGHTSCALE )
                    index = MAXLIGHTSCALE-1;

                dc_colormap = walllights[index];
            }

            sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
            dc_iscale = 0xffffffffu / (unsigned)spryscale;

            col = (column_t *)(
                (byte *)R_GetColumn(texnum,maskedtexturecol[dc_x]) -3);

            R_DrawMaskedColumn (col);
            maskedtexturecol[dc_x] = MAXSHORT;
        }
        spryscale += rw_scalestep;
    }

}




#define HEIGHTBITS              12
#define HEIGHTUNIT              (1<<HEIGHTBITS)

void R_RenderSegLoop (void)
{
    angle_t             angle;
    unsigned            index;
    int                 yl;
    int                 yh;
    int                 mid;
    fixed_t             texturecolumn;
    int                 top;
    int                 bottom;


    for ( ; rw_x < rw_stopx ; rw_x++)
    {
        yl = (topfrac+HEIGHTUNIT-1)>>HEIGHTBITS;

        if (yl < ceilingclip[rw_x]+1)
            yl = ceilingclip[rw_x]+1;

        if (markceiling)
        {
            top = ceilingclip[rw_x]+1;
            bottom = yl-1;

            if (bottom >= floorclip[rw_x])
                bottom = floorclip[rw_x]-1;

            if (top <= bottom)
            {
                ceilingplane->top[rw_x] = top;
                ceilingplane->bottom[rw_x] = bottom;
            }
        }

        yh = bottomfrac>>HEIGHTBITS;

        if (yh >= floorclip[rw_x])
            yh = floorclip[rw_x]-1;

        if (markfloor)
        {
            top = yh+1;
            bottom = floorclip[rw_x]-1;
            if (top <= ceilingclip[rw_x])
                top = ceilingclip[rw_x]+1;
            if (top <= bottom)
            {
                floorplane->top[rw_x] = top;
                floorplane->bottom[rw_x] = bottom;
            }
        }

        if (segtextured)
        {
            angle = (rw_centerangle + xtoviewangle[rw_x])>>ANGLETOFINESHIFT;
            texturecolumn = rw_offset-FixedMul(finetangent[angle],rw_distance);
            texturecolumn >>= FRACBITS;
            index = rw_scale>>LIGHTSCALESHIFT;

            if (index >=  MAXLIGHTSCALE )
                index = MAXLIGHTSCALE-1;

            dc_colormap = walllights[index];
            dc_x = rw_x;
            dc_iscale = 0xffffffffu / (unsigned)rw_scale;
        }

        if (midtexture)
        {
            dc_yl = yl;
            dc_yh = yh;
            dc_texturemid = rw_midtexturemid;
            dc_source = R_GetColumn(midtexture,texturecolumn);
            colfunc ();
            ceilingclip[rw_x] = viewheight;
            floorclip[rw_x] = -1;
        }
        else
        {
            if (toptexture)
            {
                mid = pixhigh>>HEIGHTBITS;
                pixhigh += pixhighstep;

                if (mid >= floorclip[rw_x])
                    mid = floorclip[rw_x]-1;

                if (mid >= yl)
                {
                    dc_yl = yl;
                    dc_yh = mid;
                    dc_texturemid = rw_toptexturemid;
                    dc_source = R_GetColumn(toptexture,texturecolumn);
                    colfunc ();
                    ceilingclip[rw_x] = mid;
                }
                else
                    ceilingclip[rw_x] = yl-1;
            }
            else
            {
                if (markceiling)
                    ceilingclip[rw_x] = yl-1;
            }

            if (bottomtexture)
            {
                mid = (pixlow+HEIGHTUNIT-1)>>HEIGHTBITS;
                pixlow += pixlowstep;

                if (mid <= ceilingclip[rw_x])
                    mid = ceilingclip[rw_x]+1;

                if (mid <= yh)
                {
                    dc_yl = mid;
                    dc_yh = yh;
                    dc_texturemid = rw_bottomtexturemid;
                    dc_source = R_GetColumn(bottomtexture,
                                            texturecolumn);
                    colfunc ();
                    floorclip[rw_x] = mid;
                }
                else
                    floorclip[rw_x] = yh+1;
            }
            else
            {
                if (markfloor)
                    floorclip[rw_x] = yh+1;
            }

            if (maskedtexture)
            {
                maskedtexturecol[rw_x] = texturecolumn;
            }
        }

        rw_scale += rw_scalestep;
        topfrac += topstep;
        bottomfrac += bottomstep;
    }
}




void
R_StoreWallRange
( int   start,
  int   stop )
{
    fixed_t             hyp;
    fixed_t             sineval;
    angle_t             distangle, offsetangle;
    fixed_t             vtop;
    int                 lightnum;

    if (ds_p == &drawsegs[MAXDRAWSEGS])
        return;

#ifdef RANGECHECK
    if (start >=viewwidth || start > stop)
        I_Error ("Bad R_RenderWallRange: %i to %i", start , stop);
#endif

    sidedef = curline->sidedef;
    linedef = curline->linedef;

    linedef->flags |= ML_MAPPED;

    rw_normalangle = curline->angle + ANG90;
    offsetangle = abs(rw_normalangle-rw_angle1);

    if (offsetangle > ANG90)
        offsetangle = ANG90;

    distangle = ANG90 - offsetangle;
    hyp = R_PointToDist (curline->v1->x, curline->v1->y);
    sineval = finesine[distangle>>ANGLETOFINESHIFT];
    rw_distance = FixedMul (hyp, sineval);


    ds_p->x1 = rw_x = start;
    ds_p->x2 = stop;
    ds_p->curline = curline;
    rw_stopx = stop+1;

    ds_p->scale1 = rw_scale =
        R_ScaleFromGlobalAngle (viewangle + xtoviewangle[start]);

    if (stop > start )
    {
        ds_p->scale2 = R_ScaleFromGlobalAngle (viewangle + xtoviewangle[stop]);
        ds_p->scalestep = rw_scalestep =
            (ds_p->scale2 - rw_scale) / (stop-start);
    }
    else
    {
#if 0
        if (rw_distance < FRACUNIT/2)
        {
            fixed_t             trx,try;
            fixed_t             gxt,gyt;

            trx = curline->v1->x - viewx;
            try = curline->v1->y - viewy;

            gxt = FixedMul(trx,viewcos);
            gyt = -FixedMul(try,viewsin);
            ds_p->scale1 = FixedDiv(projection, gxt-gyt)<<detailshift;
        }
#endif
        ds_p->scale2 = ds_p->scale1;
    }

    worldtop = frontsector->ceilingheight - viewz;
    worldbottom = frontsector->floorheight - viewz;

    midtexture = toptexture = bottomtexture = maskedtexture = 0;
    ds_p->maskedtexturecol = NULL;

    if (!backsector)
    {
        midtexture = texturetranslation[sidedef->midtexture];
        markfloor = markceiling = true;
        if (linedef->flags & ML_DONTPEGBOTTOM)
        {
            vtop = frontsector->floorheight +
                textureheight[sidedef->midtexture];
            rw_midtexturemid = vtop - viewz;
        }
        else
        {
            rw_midtexturemid = worldtop;
        }
        rw_midtexturemid += sidedef->rowoffset;

        ds_p->silhouette = SIL_BOTH;
        ds_p->sprtopclip = screenheightarray;
        ds_p->sprbottomclip = negonearray;
        ds_p->bsilheight = MAXINT;
        ds_p->tsilheight = MININT;
    }
    else
    {
        ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
        ds_p->silhouette = 0;

        if (frontsector->floorheight > backsector->floorheight)
        {
            ds_p->silhouette = SIL_BOTTOM;
            ds_p->bsilheight = frontsector->floorheight;
        }
        else if (backsector->floorheight > viewz)
        {
            ds_p->silhouette = SIL_BOTTOM;
            ds_p->bsilheight = MAXINT;
        }

        if (frontsector->ceilingheight < backsector->ceilingheight)
        {
            ds_p->silhouette |= SIL_TOP;
            ds_p->tsilheight = frontsector->ceilingheight;
        }
        else if (backsector->ceilingheight < viewz)
        {
            ds_p->silhouette |= SIL_TOP;
            ds_p->tsilheight = MININT;
        }

        if (backsector->ceilingheight <= frontsector->floorheight)
        {
            ds_p->sprbottomclip = negonearray;
            ds_p->bsilheight = MAXINT;
            ds_p->silhouette |= SIL_BOTTOM;
        }

        if (backsector->floorheight >= frontsector->ceilingheight)
        {
            ds_p->sprtopclip = screenheightarray;
            ds_p->tsilheight = MININT;
            ds_p->silhouette |= SIL_TOP;
        }

        worldhigh = backsector->ceilingheight - viewz;
        worldlow = backsector->floorheight - viewz;

        if (frontsector->ceilingpic == skyflatnum
            && backsector->ceilingpic == skyflatnum)
        {
            worldtop = worldhigh;
        }


        if (worldlow != worldbottom
            || backsector->floorpic != frontsector->floorpic
            || backsector->lightlevel != frontsector->lightlevel)
        {
            markfloor = true;
        }
        else
        {
            markfloor = false;
        }


        if (worldhigh != worldtop
            || backsector->ceilingpic != frontsector->ceilingpic
            || backsector->lightlevel != frontsector->lightlevel)
        {
            markceiling = true;
        }
        else
        {
            markceiling = false;
        }

        if (backsector->ceilingheight <= frontsector->floorheight
            || backsector->floorheight >= frontsector->ceilingheight)
        {
            markceiling = markfloor = true;
        }


        if (worldhigh < worldtop)
        {
            toptexture = texturetranslation[sidedef->toptexture];
            if (linedef->flags & ML_DONTPEGTOP)
            {
                rw_toptexturemid = worldtop;
            }
            else
            {
                vtop =
                    backsector->ceilingheight
                    + textureheight[sidedef->toptexture];

                rw_toptexturemid = vtop - viewz;
            }
        }
        if (worldlow > worldbottom)
        {
            bottomtexture = texturetranslation[sidedef->bottomtexture];

            if (linedef->flags & ML_DONTPEGBOTTOM )
            {
                rw_bottomtexturemid = worldtop;
            }
            else
                rw_bottomtexturemid = worldlow;
        }
        rw_toptexturemid += sidedef->rowoffset;
        rw_bottomtexturemid += sidedef->rowoffset;

        if (sidedef->midtexture)
        {
            maskedtexture = true;
            ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
            lastopening += rw_stopx - rw_x;
        }
    }

    segtextured = midtexture | toptexture | bottomtexture | maskedtexture;

    if (segtextured)
    {
        offsetangle = rw_normalangle-rw_angle1;

        if (offsetangle > ANG180)
            offsetangle = -offsetangle;

        if (offsetangle > ANG90)
            offsetangle = ANG90;

        sineval = finesine[offsetangle >>ANGLETOFINESHIFT];
        rw_offset = FixedMul (hyp, sineval);

        if (rw_normalangle-rw_angle1 < ANG180)
            rw_offset = -rw_offset;

        rw_offset += sidedef->textureoffset + curline->offset;
        rw_centerangle = ANG90 + viewangle - rw_normalangle;

        if (!fixedcolormap)
        {
            lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight;

            if (curline->v1->y == curline->v2->y)
                lightnum--;
            else if (curline->v1->x == curline->v2->x)
                lightnum++;

            if (lightnum < 0)
                walllights = scalelight[0];
            else if (lightnum >= LIGHTLEVELS)
                walllights = scalelight[LIGHTLEVELS-1];
            else
                walllights = scalelight[lightnum];
        }
    }



    if (frontsector->floorheight >= viewz)
    {
        markfloor = false;
    }

    if (frontsector->ceilingheight <= viewz
        && frontsector->ceilingpic != skyflatnum)
    {
        markceiling = false;
    }


    worldtop >>= 4;
    worldbottom >>= 4;

    topstep = -FixedMul (rw_scalestep, worldtop);
    topfrac = (centeryfrac>>4) - FixedMul (worldtop, rw_scale);

    bottomstep = -FixedMul (rw_scalestep,worldbottom);
    bottomfrac = (centeryfrac>>4) - FixedMul (worldbottom, rw_scale);

    if (backsector)
    {
        worldhigh >>= 4;
        worldlow >>= 4;

        if (worldhigh < worldtop)
        {
            pixhigh = (centeryfrac>>4) - FixedMul (worldhigh, rw_scale);
            pixhighstep = -FixedMul (rw_scalestep,worldhigh);
        }

        if (worldlow > worldbottom)
        {
            pixlow = (centeryfrac>>4) - FixedMul (worldlow, rw_scale);
            pixlowstep = -FixedMul (rw_scalestep,worldlow);
        }
    }

    if (markceiling)
        ceilingplane = R_CheckPlane (ceilingplane, rw_x, rw_stopx-1);

    if (markfloor)
        floorplane = R_CheckPlane (floorplane, rw_x, rw_stopx-1);

    R_RenderSegLoop ();


    if ( ((ds_p->silhouette & SIL_TOP) || maskedtexture)
         && !ds_p->sprtopclip)
    {
        memcpy (lastopening, ceilingclip+start, 2*(rw_stopx-start));
        ds_p->sprtopclip = lastopening - start;
        lastopening += rw_stopx - start;
    }

    if ( ((ds_p->silhouette & SIL_BOTTOM) || maskedtexture)
         && !ds_p->sprbottomclip)
    {
        memcpy (lastopening, floorclip+start, 2*(rw_stopx-start));
        ds_p->sprbottomclip = lastopening - start;
        lastopening += rw_stopx - start;
    }

    if (maskedtexture && !(ds_p->silhouette&SIL_TOP))
    {
        ds_p->silhouette |= SIL_TOP;
        ds_p->tsilheight = MININT;
    }
    if (maskedtexture && !(ds_p->silhouette&SIL_BOTTOM))
    {
        ds_p->silhouette |= SIL_BOTTOM;
        ds_p->bsilheight = MAXINT;
    }
    ds_p++;
}

