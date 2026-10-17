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
//      LineOfSight/Visibility checks, uses REJECT Lookup Table.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: p_sight.c,v 1.3 1997/01/28 22:08:28 b1 Exp $";


#include "doomdef.h"

#include "i_system.h"
#include "p_local.h"

#include "r_state.h"

fixed_t         sightzstart;
fixed_t         topslope;
fixed_t         bottomslope;

divline_t       strace;
fixed_t         t2x;
fixed_t         t2y;

int             sightcounts[2];


int
P_DivlineSide
( fixed_t       x,
  fixed_t       y,
  divline_t*    node )
{
    fixed_t     dx;
    fixed_t     dy;
    fixed_t     left;
    fixed_t     right;

    if (!node->dx)
    {
        if (x==node->x)
            return 2;

        if (x <= node->x)
            return node->dy > 0;

        return node->dy < 0;
    }

    if (!node->dy)
    {
        if (x==node->y)
            return 2;

        if (y <= node->y)
            return node->dx < 0;

        return node->dx > 0;
    }

    dx = (x - node->x);
    dy = (y - node->y);

    left =  (node->dy>>FRACBITS) * (dx>>FRACBITS);
    right = (dy>>FRACBITS) * (node->dx>>FRACBITS);

    if (right < left)
        return 0;

    if (left == right)
        return 2;
    return 1;
}


fixed_t
P_InterceptVector2
( divline_t*    v2,
  divline_t*    v1 )
{
    fixed_t     frac;
    fixed_t     num;
    fixed_t     den;

    den = FixedMul (v1->dy>>8,v2->dx) - FixedMul(v1->dx>>8,v2->dy);

    if (den == 0)
        return 0;

    num = FixedMul ( (v1->x - v2->x)>>8 ,v1->dy) +
        FixedMul ( (v2->y - v1->y)>>8 , v1->dx);
    frac = FixedDiv (num , den);

    return frac;
}

boolean P_CrossSubsector (int num)
{
    seg_t*              seg;
    line_t*             line;
    int                 s1;
    int                 s2;
    int                 count;
    subsector_t*        sub;
    sector_t*           front;
    sector_t*           back;
    fixed_t             opentop;
    fixed_t             openbottom;
    divline_t           divl;
    vertex_t*           v1;
    vertex_t*           v2;
    fixed_t             frac;
    fixed_t             slope;

#ifdef RANGECHECK
    if (num>=numsubsectors)
        I_Error ("P_CrossSubsector: ss %i with numss = %i",
                 num,
                 numsubsectors);
#endif

    sub = &subsectors[num];

    count = sub->numlines;
    seg = &segs[sub->firstline];

    for ( ; count ; seg++, count--)
    {
        line = seg->linedef;

        if (line->validcount == validcount)
            continue;

        line->validcount = validcount;

        v1 = line->v1;
        v2 = line->v2;
        s1 = P_DivlineSide (v1->x,v1->y, &strace);
        s2 = P_DivlineSide (v2->x, v2->y, &strace);

        if (s1 == s2)
            continue;

        divl.x = v1->x;
        divl.y = v1->y;
        divl.dx = v2->x - v1->x;
        divl.dy = v2->y - v1->y;
        s1 = P_DivlineSide (strace.x, strace.y, &divl);
        s2 = P_DivlineSide (t2x, t2y, &divl);

        if (s1 == s2)
            continue;

        if ( !(line->flags & ML_TWOSIDED) )
            return false;

        front = seg->frontsector;
        back = seg->backsector;

        if (front->floorheight == back->floorheight
            && front->ceilingheight == back->ceilingheight)
            continue;

        if (front->ceilingheight < back->ceilingheight)
            opentop = front->ceilingheight;
        else
            opentop = back->ceilingheight;

        if (front->floorheight > back->floorheight)
            openbottom = front->floorheight;
        else
            openbottom = back->floorheight;

        if (openbottom >= opentop)
            return false;

        frac = P_InterceptVector2 (&strace, &divl);

        if (front->floorheight != back->floorheight)
        {
            slope = FixedDiv (openbottom - sightzstart , frac);
            if (slope > bottomslope)
                bottomslope = slope;
        }

        if (front->ceilingheight != back->ceilingheight)
        {
            slope = FixedDiv (opentop - sightzstart , frac);
            if (slope < topslope)
                topslope = slope;
        }

        if (topslope <= bottomslope)
            return false;
    }
    return true;
}



boolean P_CrossBSPNode (int bspnum)
{
    node_t*     bsp;
    int         side;

    if (bspnum & NF_SUBSECTOR)
    {
        if (bspnum == -1)
            return P_CrossSubsector (0);
        else
            return P_CrossSubsector (bspnum&(~NF_SUBSECTOR));
    }

    bsp = &nodes[bspnum];

    side = P_DivlineSide (strace.x, strace.y, (divline_t *)bsp);
    if (side == 2)
        side = 0;

    if (!P_CrossBSPNode (bsp->children[side]) )
        return false;

    if (side == P_DivlineSide (t2x, t2y,(divline_t *)bsp))
    {
        return true;
    }

    return P_CrossBSPNode (bsp->children[side^1]);
}


boolean
P_CheckSight
( mobj_t*       t1,
  mobj_t*       t2 )
{
    int         s1;
    int         s2;
    int         pnum;
    int         bytenum;
    int         bitnum;


    s1 = (t1->subsector->sector - sectors);
    s2 = (t2->subsector->sector - sectors);
    pnum = s1*numsectors + s2;
    bytenum = pnum>>3;
    bitnum = 1 << (pnum&7);

    if (rejectmatrix[bytenum]&bitnum)
    {
        sightcounts[0]++;

        return false;
    }

    sightcounts[1]++;

    validcount++;

    sightzstart = t1->z + t1->height - (t1->height>>2);
    topslope = (t2->z+t2->height) - sightzstart;
    bottomslope = (t2->z) - sightzstart;

    strace.x = t1->x;
    strace.y = t1->y;
    t2x = t2->x;
    t2y = t2->y;
    strace.dx = t2->x - t1->x;
    strace.dy = t2->y - t1->y;

    return P_CrossBSPNode (numnodes-1);
}


