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
//      Movement, collision handling.
//      Shooting and aiming.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: p_map.c,v 1.5 1997/02/03 22:45:11 b1 Exp $";


#include <stdlib.h>

#include "m_bbox.h"
#include "m_random.h"
#include "i_system.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

#include "doomstat.h"
#include "r_state.h"
#include "sounds.h"


fixed_t         tmbbox[4];
mobj_t*         tmthing;
int             tmflags;
fixed_t         tmx;
fixed_t         tmy;


boolean         floatok;

fixed_t         tmfloorz;
fixed_t         tmceilingz;
fixed_t         tmdropoffz;

line_t*         ceilingline;

#define MAXSPECIALCROSS         8

line_t*         spechit[MAXSPECIALCROSS];
int             numspechit;




boolean PIT_StompThing (mobj_t* thing)
{
    fixed_t     blockdist;

    if (!(thing->flags & MF_SHOOTABLE) )
        return true;

    blockdist = thing->radius + tmthing->radius;

    if ( abs(thing->x - tmx) >= blockdist
         || abs(thing->y - tmy) >= blockdist )
    {
        return true;
    }

    if (thing == tmthing)
        return true;

    if ( !tmthing->player && gamemap != 30)
        return false;

    P_DamageMobj (thing, tmthing, tmthing, 10000);

    return true;
}


boolean
P_TeleportMove
( mobj_t*       thing,
  fixed_t       x,
  fixed_t       y )
{
    int                 xl;
    int                 xh;
    int                 yl;
    int                 yh;
    int                 bx;
    int                 by;

    subsector_t*        newsubsec;

    tmthing = thing;
    tmflags = thing->flags;

    tmx = x;
    tmy = y;

    tmbbox[BOXTOP] = y + tmthing->radius;
    tmbbox[BOXBOTTOM] = y - tmthing->radius;
    tmbbox[BOXRIGHT] = x + tmthing->radius;
    tmbbox[BOXLEFT] = x - tmthing->radius;

    newsubsec = R_PointInSubsector (x,y);
    ceilingline = NULL;

    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    validcount++;
    numspechit = 0;

    xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
        for (by=yl ; by<=yh ; by++)
            if (!P_BlockThingsIterator(bx,by,PIT_StompThing))
                return false;

    P_UnsetThingPosition (thing);

    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;
    thing->x = x;
    thing->y = y;

    P_SetThingPosition (thing);

    return true;
}




boolean PIT_CheckLine (line_t* ld)
{
    if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
        || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
        || tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
        || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP] )
        return true;

    if (P_BoxOnLineSide (tmbbox, ld) != -1)
        return true;



    if (!ld->backsector)
        return false;

    if (!(tmthing->flags & MF_MISSILE) )
    {
        if ( ld->flags & ML_BLOCKING )
            return false;

        if ( !tmthing->player && ld->flags & ML_BLOCKMONSTERS )
            return false;
    }

    P_LineOpening (ld);

    if (opentop < tmceilingz)
    {
        tmceilingz = opentop;
        ceilingline = ld;
    }

    if (openbottom > tmfloorz)
        tmfloorz = openbottom;

    if (lowfloor < tmdropoffz)
        tmdropoffz = lowfloor;

    if (ld->special)
    {
        spechit[numspechit] = ld;
        numspechit++;
    }

    return true;
}

boolean PIT_CheckThing (mobj_t* thing)
{
    fixed_t             blockdist;
    boolean             solid;
    int                 damage;

    if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE) ))
        return true;

    blockdist = thing->radius + tmthing->radius;

    if ( abs(thing->x - tmx) >= blockdist
         || abs(thing->y - tmy) >= blockdist )
    {
        return true;
    }

    if (thing == tmthing)
        return true;

    if (tmthing->flags & MF_SKULLFLY)
    {
        damage = ((P_Random()%8)+1)*tmthing->info->damage;

        P_DamageMobj (thing, tmthing, tmthing, damage);

        tmthing->flags &= ~MF_SKULLFLY;
        tmthing->momx = tmthing->momy = tmthing->momz = 0;

        P_SetMobjState (tmthing, tmthing->info->spawnstate);

        return false;
    }


    if (tmthing->flags & MF_MISSILE)
    {
        if (tmthing->z > thing->z + thing->height)
            return true;
        if (tmthing->z+tmthing->height < thing->z)
            return true;

        if (tmthing->target && (
            tmthing->target->type == thing->type ||
            (tmthing->target->type == MT_KNIGHT && thing->type == MT_BRUISER)||
            (tmthing->target->type == MT_BRUISER && thing->type == MT_KNIGHT) ) )
        {
            if (thing == tmthing->target)
                return true;

            if (thing->type != MT_PLAYER)
            {
                return false;
            }
        }

        if (! (thing->flags & MF_SHOOTABLE) )
        {
            return !(thing->flags & MF_SOLID);
        }

        damage = ((P_Random()%8)+1)*tmthing->info->damage;
        P_DamageMobj (thing, tmthing, tmthing->target, damage);

        return false;
    }

    if (thing->flags & MF_SPECIAL)
    {
        solid = thing->flags&MF_SOLID;
        if (tmflags&MF_PICKUP)
        {
            P_TouchSpecialThing (thing, tmthing);
        }
        return !solid;
    }

    return !(thing->flags & MF_SOLID);
}



boolean
P_CheckPosition
( mobj_t*       thing,
  fixed_t       x,
  fixed_t       y )
{
    int                 xl;
    int                 xh;
    int                 yl;
    int                 yh;
    int                 bx;
    int                 by;
    subsector_t*        newsubsec;

    tmthing = thing;
    tmflags = thing->flags;

    tmx = x;
    tmy = y;

    tmbbox[BOXTOP] = y + tmthing->radius;
    tmbbox[BOXBOTTOM] = y - tmthing->radius;
    tmbbox[BOXRIGHT] = x + tmthing->radius;
    tmbbox[BOXLEFT] = x - tmthing->radius;

    newsubsec = R_PointInSubsector (x,y);
    ceilingline = NULL;

    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    validcount++;
    numspechit = 0;

    if ( tmflags & MF_NOCLIP )
        return true;

    xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
        for (by=yl ; by<=yh ; by++)
            if (!P_BlockThingsIterator(bx,by,PIT_CheckThing))
                return false;

    xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
        for (by=yl ; by<=yh ; by++)
            if (!P_BlockLinesIterator (bx,by,PIT_CheckLine))
                return false;

    return true;
}


boolean
P_TryMove
( mobj_t*       thing,
  fixed_t       x,
  fixed_t       y )
{
    fixed_t     oldx;
    fixed_t     oldy;
    int         side;
    int         oldside;
    line_t*     ld;

    floatok = false;
    if (!P_CheckPosition (thing, x, y))
        return false;

    if ( !(thing->flags & MF_NOCLIP) )
    {
        if (tmceilingz - tmfloorz < thing->height)
            return false;

        floatok = true;

        if ( !(thing->flags&MF_TELEPORT)
             &&tmceilingz - thing->z < thing->height)
            return false;

        if ( !(thing->flags&MF_TELEPORT)
             && tmfloorz - thing->z > 24*FRACUNIT )
            return false;

        if ( !(thing->flags&(MF_DROPOFF|MF_FLOAT))
             && tmfloorz - tmdropoffz > 24*FRACUNIT )
            return false;
    }

    P_UnsetThingPosition (thing);

    oldx = thing->x;
    oldy = thing->y;
    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;
    thing->x = x;
    thing->y = y;

    P_SetThingPosition (thing);

    if (! (thing->flags&(MF_TELEPORT|MF_NOCLIP)) )
    {
        while (numspechit--)
        {
            ld = spechit[numspechit];
            side = P_PointOnLineSide (thing->x, thing->y, ld);
            oldside = P_PointOnLineSide (oldx, oldy, ld);
            if (side != oldside)
            {
                if (ld->special)
                    P_CrossSpecialLine (ld-lines, oldside, thing);
            }
        }
    }

    return true;
}


boolean P_ThingHeightClip (mobj_t* thing)
{
    boolean             onfloor;

    onfloor = (thing->z == thing->floorz);

    P_CheckPosition (thing, thing->x, thing->y);

    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;

    if (onfloor)
    {
        thing->z = thing->floorz;
    }
    else
    {
        if (thing->z+thing->height > thing->ceilingz)
            thing->z = thing->ceilingz - thing->height;
    }

    if (thing->ceilingz - thing->floorz < thing->height)
        return false;

    return true;
}



fixed_t         bestslidefrac;
fixed_t         secondslidefrac;

line_t*         bestslideline;
line_t*         secondslideline;

mobj_t*         slidemo;

fixed_t         tmxmove;
fixed_t         tmymove;



void P_HitSlideLine (line_t* ld)
{
    int                 side;

    angle_t             lineangle;
    angle_t             moveangle;
    angle_t             deltaangle;

    fixed_t             movelen;
    fixed_t             newlen;


    if (ld->slopetype == ST_HORIZONTAL)
    {
        tmymove = 0;
        return;
    }

    if (ld->slopetype == ST_VERTICAL)
    {
        tmxmove = 0;
        return;
    }

    side = P_PointOnLineSide (slidemo->x, slidemo->y, ld);

    lineangle = R_PointToAngle2 (0,0, ld->dx, ld->dy);

    if (side == 1)
        lineangle += ANG180;

    moveangle = R_PointToAngle2 (0,0, tmxmove, tmymove);
    deltaangle = moveangle-lineangle;

    if (deltaangle > ANG180)
        deltaangle += ANG180;

    lineangle >>= ANGLETOFINESHIFT;
    deltaangle >>= ANGLETOFINESHIFT;

    movelen = P_AproxDistance (tmxmove, tmymove);
    newlen = FixedMul (movelen, finecosine[deltaangle]);

    tmxmove = FixedMul (newlen, finecosine[lineangle]);
    tmymove = FixedMul (newlen, finesine[lineangle]);
}


boolean PTR_SlideTraverse (intercept_t* in)
{
    line_t*     li;

    if (!in->isaline)
        I_Error ("PTR_SlideTraverse: not a line?");

    li = in->d.line;

    if ( ! (li->flags & ML_TWOSIDED) )
    {
        if (P_PointOnLineSide (slidemo->x, slidemo->y, li))
        {
            return true;
        }
        goto isblocking;
    }

    P_LineOpening (li);

    if (openrange < slidemo->height)
        goto isblocking;

    if (opentop - slidemo->z < slidemo->height)
        goto isblocking;

    if (openbottom - slidemo->z > 24*FRACUNIT )
        goto isblocking;

    return true;

  isblocking:
    if (in->frac < bestslidefrac)
    {
        secondslidefrac = bestslidefrac;
        secondslideline = bestslideline;
        bestslidefrac = in->frac;
        bestslideline = li;
    }

    return false;
}



void P_SlideMove (mobj_t* mo)
{
    fixed_t             leadx;
    fixed_t             leady;
    fixed_t             trailx;
    fixed_t             traily;
    fixed_t             newx;
    fixed_t             newy;
    int                 hitcount;

    slidemo = mo;
    hitcount = 0;

  retry:
    if (++hitcount == 3)
        goto stairstep;


    if (mo->momx > 0)
    {
        leadx = mo->x + mo->radius;
        trailx = mo->x - mo->radius;
    }
    else
    {
        leadx = mo->x - mo->radius;
        trailx = mo->x + mo->radius;
    }

    if (mo->momy > 0)
    {
        leady = mo->y + mo->radius;
        traily = mo->y - mo->radius;
    }
    else
    {
        leady = mo->y - mo->radius;
        traily = mo->y + mo->radius;
    }

    bestslidefrac = FRACUNIT+1;

    P_PathTraverse ( leadx, leady, leadx+mo->momx, leady+mo->momy,
                     PT_ADDLINES, PTR_SlideTraverse );
    P_PathTraverse ( trailx, leady, trailx+mo->momx, leady+mo->momy,
                     PT_ADDLINES, PTR_SlideTraverse );
    P_PathTraverse ( leadx, traily, leadx+mo->momx, traily+mo->momy,
                     PT_ADDLINES, PTR_SlideTraverse );

    if (bestslidefrac == FRACUNIT+1)
    {
      stairstep:
        if (!P_TryMove (mo, mo->x, mo->y + mo->momy))
            P_TryMove (mo, mo->x + mo->momx, mo->y);
        return;
    }

    bestslidefrac -= 0x800;
    if (bestslidefrac > 0)
    {
        newx = FixedMul (mo->momx, bestslidefrac);
        newy = FixedMul (mo->momy, bestslidefrac);

        if (!P_TryMove (mo, mo->x+newx, mo->y+newy))
            goto stairstep;
    }

    bestslidefrac = FRACUNIT-(bestslidefrac+0x800);

    if (bestslidefrac > FRACUNIT)
        bestslidefrac = FRACUNIT;

    if (bestslidefrac <= 0)
        return;

    tmxmove = FixedMul (mo->momx, bestslidefrac);
    tmymove = FixedMul (mo->momy, bestslidefrac);

    P_HitSlideLine (bestslideline);

    mo->momx = tmxmove;
    mo->momy = tmymove;

    if (!P_TryMove (mo, mo->x+tmxmove, mo->y+tmymove))
    {
        goto retry;
    }
}


mobj_t*         linetarget;
mobj_t*         shootthing;

fixed_t         shootz;

int             la_damage;
fixed_t         attackrange;

fixed_t         aimslope;

extern fixed_t  topslope;
extern fixed_t  bottomslope;


boolean
PTR_AimTraverse (intercept_t* in)
{
    line_t*             li;
    mobj_t*             th;
    fixed_t             slope;
    fixed_t             thingtopslope;
    fixed_t             thingbottomslope;
    fixed_t             dist;

    if (in->isaline)
    {
        li = in->d.line;

        if ( !(li->flags & ML_TWOSIDED) )
            return false;

        P_LineOpening (li);

        if (openbottom >= opentop)
            return false;

        dist = FixedMul (attackrange, in->frac);

        if (li->frontsector->floorheight != li->backsector->floorheight)
        {
            slope = FixedDiv (openbottom - shootz , dist);
            if (slope > bottomslope)
                bottomslope = slope;
        }

        if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
        {
            slope = FixedDiv (opentop - shootz , dist);
            if (slope < topslope)
                topslope = slope;
        }

        if (topslope <= bottomslope)
            return false;

        return true;
    }

    th = in->d.thing;
    if (th == shootthing)
        return true;

    if (!(th->flags&MF_SHOOTABLE))
        return true;

    dist = FixedMul (attackrange, in->frac);
    thingtopslope = FixedDiv (th->z+th->height - shootz , dist);

    if (thingtopslope < bottomslope)
        return true;

    thingbottomslope = FixedDiv (th->z - shootz, dist);

    if (thingbottomslope > topslope)
        return true;

    if (thingtopslope > topslope)
        thingtopslope = topslope;

    if (thingbottomslope < bottomslope)
        thingbottomslope = bottomslope;

    aimslope = (thingtopslope+thingbottomslope)/2;
    linetarget = th;

    return false;
}


boolean PTR_ShootTraverse (intercept_t* in)
{
    fixed_t             x;
    fixed_t             y;
    fixed_t             z;
    fixed_t             frac;

    line_t*             li;

    mobj_t*             th;

    fixed_t             slope;
    fixed_t             dist;
    fixed_t             thingtopslope;
    fixed_t             thingbottomslope;

    if (in->isaline)
    {
        li = in->d.line;

        if (li->special)
            P_ShootSpecialLine (shootthing, li);

        if ( !(li->flags & ML_TWOSIDED) )
            goto hitline;

        P_LineOpening (li);

        dist = FixedMul (attackrange, in->frac);

        if (li->frontsector->floorheight != li->backsector->floorheight)
        {
            slope = FixedDiv (openbottom - shootz , dist);
            if (slope > aimslope)
                goto hitline;
        }

        if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
        {
            slope = FixedDiv (opentop - shootz , dist);
            if (slope < aimslope)
                goto hitline;
        }

        return true;


      hitline:
        frac = in->frac - FixedDiv (4*FRACUNIT,attackrange);
        x = trace.x + FixedMul (trace.dx, frac);
        y = trace.y + FixedMul (trace.dy, frac);
        z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

        if (li->frontsector->ceilingpic == skyflatnum)
        {
            if (z > li->frontsector->ceilingheight)
                return false;

            if  (li->backsector && li->backsector->ceilingpic == skyflatnum)
                return false;
        }

        P_SpawnPuff (x,y,z);

        return false;
    }

    th = in->d.thing;
    if (th == shootthing)
        return true;

    if (!(th->flags&MF_SHOOTABLE))
        return true;

    dist = FixedMul (attackrange, in->frac);
    thingtopslope = FixedDiv (th->z+th->height - shootz , dist);

    if (thingtopslope < aimslope)
        return true;

    thingbottomslope = FixedDiv (th->z - shootz, dist);

    if (thingbottomslope > aimslope)
        return true;


    frac = in->frac - FixedDiv (10*FRACUNIT,attackrange);

    x = trace.x + FixedMul (trace.dx, frac);
    y = trace.y + FixedMul (trace.dy, frac);
    z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

    if (in->d.thing->flags & MF_NOBLOOD)
        P_SpawnPuff (x,y,z);
    else
        P_SpawnBlood (x,y,z, la_damage);

    if (la_damage)
        P_DamageMobj (th, shootthing, shootthing, la_damage);

    return false;

}


fixed_t
P_AimLineAttack
( mobj_t*       t1,
  angle_t       angle,
  fixed_t       distance )
{
    fixed_t     x2;
    fixed_t     y2;

    angle >>= ANGLETOFINESHIFT;
    shootthing = t1;

    x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
    y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
    shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;

    topslope = 100*FRACUNIT/160;
    bottomslope = -100*FRACUNIT/160;

    attackrange = distance;
    linetarget = NULL;

    P_PathTraverse ( t1->x, t1->y,
                     x2, y2,
                     PT_ADDLINES|PT_ADDTHINGS,
                     PTR_AimTraverse );

    if (linetarget)
        return aimslope;

    return 0;
}


void
P_LineAttack
( mobj_t*       t1,
  angle_t       angle,
  fixed_t       distance,
  fixed_t       slope,
  int           damage )
{
    fixed_t     x2;
    fixed_t     y2;

    angle >>= ANGLETOFINESHIFT;
    shootthing = t1;
    la_damage = damage;
    x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
    y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
    shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;
    attackrange = distance;
    aimslope = slope;

    P_PathTraverse ( t1->x, t1->y,
                     x2, y2,
                     PT_ADDLINES|PT_ADDTHINGS,
                     PTR_ShootTraverse );
}



mobj_t*         usething;

boolean PTR_UseTraverse (intercept_t* in)
{
    int         side;

    if (!in->d.line->special)
    {
        P_LineOpening (in->d.line);
        if (openrange <= 0)
        {
            S_StartSound (usething, sfx_noway);

            return false;
        }
        return true ;
    }

    side = 0;
    if (P_PointOnLineSide (usething->x, usething->y, in->d.line) == 1)
        side = 1;


    P_UseSpecialLine (usething, in->d.line, side);

    return false;
}


void P_UseLines (player_t*      player)
{
    int         angle;
    fixed_t     x1;
    fixed_t     y1;
    fixed_t     x2;
    fixed_t     y2;

    usething = player->mo;

    angle = player->mo->angle >> ANGLETOFINESHIFT;

    x1 = player->mo->x;
    y1 = player->mo->y;
    x2 = x1 + (USERANGE>>FRACBITS)*finecosine[angle];
    y2 = y1 + (USERANGE>>FRACBITS)*finesine[angle];

    P_PathTraverse ( x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse );
}


mobj_t*         bombsource;
mobj_t*         bombspot;
int             bombdamage;


boolean PIT_RadiusAttack (mobj_t* thing)
{
    fixed_t     dx;
    fixed_t     dy;
    fixed_t     dist;

    if (!(thing->flags & MF_SHOOTABLE) )
        return true;

    if (thing->type == MT_CYBORG
        || thing->type == MT_SPIDER)
        return true;

    dx = abs(thing->x - bombspot->x);
    dy = abs(thing->y - bombspot->y);

    dist = dx>dy ? dx : dy;
    dist = (dist - thing->radius) >> FRACBITS;

    if (dist < 0)
        dist = 0;

    if (dist >= bombdamage)
        return true;

    if ( P_CheckSight (thing, bombspot) )
    {
        P_DamageMobj (thing, bombspot, bombsource, bombdamage - dist);
    }

    return true;
}


void
P_RadiusAttack
( mobj_t*       spot,
  mobj_t*       source,
  int           damage )
{
    int         x;
    int         y;

    int         xl;
    int         xh;
    int         yl;
    int         yh;

    fixed_t     dist;

    dist = (damage+MAXRADIUS)<<FRACBITS;
    yh = (spot->y + dist - bmaporgy)>>MAPBLOCKSHIFT;
    yl = (spot->y - dist - bmaporgy)>>MAPBLOCKSHIFT;
    xh = (spot->x + dist - bmaporgx)>>MAPBLOCKSHIFT;
    xl = (spot->x - dist - bmaporgx)>>MAPBLOCKSHIFT;
    bombspot = spot;
    bombsource = source;
    bombdamage = damage;

    for (y=yl ; y<=yh ; y++)
        for (x=xl ; x<=xh ; x++)
            P_BlockThingsIterator (x, y, PIT_RadiusAttack );
}



boolean         crushchange;
boolean         nofit;


boolean PIT_ChangeSector (mobj_t*       thing)
{
    mobj_t*     mo;

    if (P_ThingHeightClip (thing))
    {
        return true;
    }


    if (thing->health <= 0)
    {
        P_SetMobjState (thing, S_GIBS);

        thing->flags &= ~MF_SOLID;
        thing->height = 0;
        thing->radius = 0;

        return true;
    }

    if (thing->flags & MF_DROPPED)
    {
        P_RemoveMobj (thing);

        return true;
    }

    if (! (thing->flags & MF_SHOOTABLE) )
    {
        return true;
    }

    nofit = true;

    if (crushchange && !(leveltime&3) )
    {
        P_DamageMobj(thing,NULL,NULL,10);

        mo = P_SpawnMobj (thing->x,
                          thing->y,
                          thing->z + thing->height/2, MT_BLOOD);

        mo->momx = (P_Random() - P_Random ())<<12;
        mo->momy = (P_Random() - P_Random ())<<12;
    }

    return true;
}



boolean
P_ChangeSector
( sector_t*     sector,
  boolean       crunch )
{
    int         x;
    int         y;

    nofit = false;
    crushchange = crunch;

    for (x=sector->blockbox[BOXLEFT] ; x<= sector->blockbox[BOXRIGHT] ; x++)
        for (y=sector->blockbox[BOXBOTTOM];y<= sector->blockbox[BOXTOP] ; y++)
            P_BlockThingsIterator (x, y, PIT_ChangeSector);


    return nofit;
}

