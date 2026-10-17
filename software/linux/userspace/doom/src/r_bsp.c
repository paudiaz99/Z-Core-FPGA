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
//      BSP traversal, handling of LineSegs for rendering.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: r_bsp.c,v 1.4 1997/02/03 22:45:12 b1 Exp $";


#include "doomdef.h"

#include "m_bbox.h"

#include "i_system.h"

#include "r_main.h"
#include "r_plane.h"
#include "r_things.h"

#include "doomstat.h"
#include "r_state.h"




seg_t*          curline;
side_t*         sidedef;
line_t*         linedef;
sector_t*       frontsector;
sector_t*       backsector;

drawseg_t       drawsegs[MAXDRAWSEGS];
drawseg_t*      ds_p;


void
R_StoreWallRange
( int   start,
  int   stop );




void R_ClearDrawSegs (void)
{
    ds_p = drawsegs;
}



typedef struct
{
    int first;
    int last;

} cliprange_t;


#define MAXSEGS         32

cliprange_t*    newend;
cliprange_t     solidsegs[MAXSEGS];




void
R_ClipSolidWallSegment
( int                   first,
  int                   last )
{
    cliprange_t*        next;
    cliprange_t*        start;

    start = solidsegs;
    while (start->last < first-1)
        start++;

    if (first < start->first)
    {
        if (last < start->first-1)
        {
            R_StoreWallRange (first, last);
            next = newend;
            newend++;

            while (next != start)
            {
                *next = *(next-1);
                next--;
            }
            next->first = first;
            next->last = last;
            return;
        }

        R_StoreWallRange (first, start->first - 1);
        start->first = first;
    }

    if (last <= start->last)
        return;

    next = start;
    while (last >= (next+1)->first-1)
    {
        R_StoreWallRange (next->last + 1, (next+1)->first - 1);
        next++;

        if (last <= next->last)
        {
            start->last = next->last;
            goto crunch;
        }
    }

    R_StoreWallRange (next->last + 1, last);
    start->last = last;

  crunch:
    if (next == start)
    {
        return;
    }


    while (next++ != newend)
    {
        *++start = *next;
    }

    newend = start+1;
}



void
R_ClipPassWallSegment
( int   first,
  int   last )
{
    cliprange_t*        start;

    start = solidsegs;
    while (start->last < first-1)
        start++;

    if (first < start->first)
    {
        if (last < start->first-1)
        {
            R_StoreWallRange (first, last);
            return;
        }

        R_StoreWallRange (first, start->first - 1);
    }

    if (last <= start->last)
        return;

    while (last >= (start+1)->first-1)
    {
        R_StoreWallRange (start->last + 1, (start+1)->first - 1);
        start++;

        if (last <= start->last)
            return;
    }

    R_StoreWallRange (start->last + 1, last);
}



void R_ClearClipSegs (void)
{
    solidsegs[0].first = -0x7fffffff;
    solidsegs[0].last = -1;
    solidsegs[1].first = viewwidth;
    solidsegs[1].last = 0x7fffffff;
    newend = solidsegs+2;
}

void R_AddLine (seg_t*  line)
{
    int                 x1;
    int                 x2;
    angle_t             angle1;
    angle_t             angle2;
    angle_t             span;
    angle_t             tspan;

    curline = line;

    angle1 = R_PointToAngle (line->v1->x, line->v1->y);
    angle2 = R_PointToAngle (line->v2->x, line->v2->y);

    span = angle1 - angle2;

    if (span >= ANG180)
        return;

    rw_angle1 = angle1;
    angle1 -= viewangle;
    angle2 -= viewangle;

    tspan = angle1 + clipangle;
    if (tspan > 2*clipangle)
    {
        tspan -= 2*clipangle;

        if (tspan >= span)
            return;

        angle1 = clipangle;
    }
    tspan = clipangle - angle2;
    if (tspan > 2*clipangle)
    {
        tspan -= 2*clipangle;

        if (tspan >= span)
            return;
        angle2 = -clipangle;
    }

    angle1 = (angle1+ANG90)>>ANGLETOFINESHIFT;
    angle2 = (angle2+ANG90)>>ANGLETOFINESHIFT;
    x1 = viewangletox[angle1];
    x2 = viewangletox[angle2];

    if (x1 == x2)
        return;

    backsector = line->backsector;

    if (!backsector)
        goto clipsolid;

    if (backsector->ceilingheight <= frontsector->floorheight
        || backsector->floorheight >= frontsector->ceilingheight)
        goto clipsolid;

    if (backsector->ceilingheight != frontsector->ceilingheight
        || backsector->floorheight != frontsector->floorheight)
        goto clippass;

    if (backsector->ceilingpic == frontsector->ceilingpic
        && backsector->floorpic == frontsector->floorpic
        && backsector->lightlevel == frontsector->lightlevel
        && curline->sidedef->midtexture == 0)
    {
        return;
    }


  clippass:
    R_ClipPassWallSegment (x1, x2-1);
    return;

  clipsolid:
    R_ClipSolidWallSegment (x1, x2-1);
}


int     checkcoord[12][4] =
{
    {3,0,2,1},
    {3,0,2,0},
    {3,1,2,0},
    {0},
    {2,0,2,1},
    {0,0,0,0},
    {3,1,3,0},
    {0},
    {2,0,3,1},
    {2,1,3,1},
    {2,1,3,0}
};


boolean R_CheckBBox (fixed_t*   bspcoord)
{
    int                 boxx;
    int                 boxy;
    int                 boxpos;

    fixed_t             x1;
    fixed_t             y1;
    fixed_t             x2;
    fixed_t             y2;

    angle_t             angle1;
    angle_t             angle2;
    angle_t             span;
    angle_t             tspan;

    cliprange_t*        start;

    int                 sx1;
    int                 sx2;

    if (viewx <= bspcoord[BOXLEFT])
        boxx = 0;
    else if (viewx < bspcoord[BOXRIGHT])
        boxx = 1;
    else
        boxx = 2;

    if (viewy >= bspcoord[BOXTOP])
        boxy = 0;
    else if (viewy > bspcoord[BOXBOTTOM])
        boxy = 1;
    else
        boxy = 2;

    boxpos = (boxy<<2)+boxx;
    if (boxpos == 5)
        return true;

    x1 = bspcoord[checkcoord[boxpos][0]];
    y1 = bspcoord[checkcoord[boxpos][1]];
    x2 = bspcoord[checkcoord[boxpos][2]];
    y2 = bspcoord[checkcoord[boxpos][3]];

    angle1 = R_PointToAngle (x1, y1) - viewangle;
    angle2 = R_PointToAngle (x2, y2) - viewangle;

    span = angle1 - angle2;

    if (span >= ANG180)
        return true;

    tspan = angle1 + clipangle;

    if (tspan > 2*clipangle)
    {
        tspan -= 2*clipangle;

        if (tspan >= span)
            return false;

        angle1 = clipangle;
    }
    tspan = clipangle - angle2;
    if (tspan > 2*clipangle)
    {
        tspan -= 2*clipangle;

        if (tspan >= span)
            return false;

        angle2 = -clipangle;
    }


    angle1 = (angle1+ANG90)>>ANGLETOFINESHIFT;
    angle2 = (angle2+ANG90)>>ANGLETOFINESHIFT;
    sx1 = viewangletox[angle1];
    sx2 = viewangletox[angle2];

    if (sx1 == sx2)
        return false;
    sx2--;

    start = solidsegs;
    while (start->last < sx2)
        start++;

    if (sx1 >= start->first
        && sx2 <= start->last)
    {
        return false;
    }

    return true;
}



void R_Subsector (int num)
{
    int                 count;
    seg_t*              line;
    subsector_t*        sub;

#ifdef RANGECHECK
    if (num>=numsubsectors)
        I_Error ("R_Subsector: ss %i with numss = %i",
                 num,
                 numsubsectors);
#endif

    sscount++;
    sub = &subsectors[num];
    frontsector = sub->sector;
    count = sub->numlines;
    line = &segs[sub->firstline];

    if (frontsector->floorheight < viewz)
    {
        floorplane = R_FindPlane (frontsector->floorheight,
                                  frontsector->floorpic,
                                  frontsector->lightlevel);
    }
    else
        floorplane = NULL;

    if (frontsector->ceilingheight > viewz
        || frontsector->ceilingpic == skyflatnum)
    {
        ceilingplane = R_FindPlane (frontsector->ceilingheight,
                                    frontsector->ceilingpic,
                                    frontsector->lightlevel);
    }
    else
        ceilingplane = NULL;

    R_AddSprites (frontsector);

    while (count--)
    {
        R_AddLine (line);
        line++;
    }
}




void R_RenderBSPNode (int bspnum)
{
    node_t*     bsp;
    int         side;

    if (bspnum & NF_SUBSECTOR)
    {
        if (bspnum == -1)
            R_Subsector (0);
        else
            R_Subsector (bspnum&(~NF_SUBSECTOR));
        return;
    }

    bsp = &nodes[bspnum];

    side = R_PointOnSide (viewx, viewy, bsp);

    R_RenderBSPNode (bsp->children[side]);

    if (R_CheckBBox (bsp->bbox[side^1]))
        R_RenderBSPNode (bsp->children[side^1]);
}


