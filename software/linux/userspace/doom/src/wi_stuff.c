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
//      Intermission screens.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: wi_stuff.c,v 1.7 1997/02/03 22:45:13 b1 Exp $";


#include <stdio.h>

#include "z_zone.h"

#include "m_random.h"
#include "m_swap.h"

#include "i_system.h"

#include "w_wad.h"

#include "g_game.h"

#include "r_local.h"
#include "s_sound.h"

#include "doomstat.h"

#include "sounds.h"

#include "v_video.h"

#include "wi_stuff.h"



#define NUMEPISODES     4
#define NUMMAPS         9




#define WI_TITLEY               2
#define WI_SPACINGY             33

#define SP_STATSX               50
#define SP_STATSY               50

#define SP_TIMEX                16
#define SP_TIMEY                (SCREENHEIGHT-32)


#define NG_STATSY               50
#define NG_STATSX               (32 + SHORT(star->width)/2 + 32*!dofrags)

#define NG_SPACINGX             64


#define DM_MATRIXX              42
#define DM_MATRIXY              68

#define DM_SPACINGX             40

#define DM_TOTALSX              269

#define DM_KILLERSX             10
#define DM_KILLERSY             100
#define DM_VICTIMSX             5
#define DM_VICTIMSY             50




typedef enum
{
    ANIM_ALWAYS,
    ANIM_RANDOM,
    ANIM_LEVEL

} animenum_t;

typedef struct
{
    int         x;
    int         y;

} point_t;


typedef struct
{
    animenum_t  type;

    int         period;

    int         nanims;

    point_t     loc;

    int         data1;

    int         data2;

    patch_t*    p[3];


    int         nexttic;

    int         lastdrawn;

    int         ctr;

    int         state;

} anim_t;


static point_t lnodes[NUMEPISODES][NUMMAPS] =
{
    {
        { 185, 164 },
        { 148, 143 },
        { 69, 122 },
        { 209, 102 },
        { 116, 89 },
        { 166, 55 },
        { 71, 56 },
        { 135, 29 },
        { 71, 24 }
    },

    {
        { 254, 25 },
        { 97, 50 },
        { 188, 64 },
        { 128, 78 },
        { 214, 92 },
        { 133, 130 },
        { 208, 136 },
        { 148, 140 },
        { 235, 158 }
    },

    {
        { 156, 168 },
        { 48, 154 },
        { 174, 95 },
        { 265, 75 },
        { 130, 48 },
        { 279, 23 },
        { 198, 48 },
        { 140, 25 },
        { 281, 136 }
    }

};


static anim_t epsd0animinfo[] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 224, 104 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 184, 160 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 112, 136 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 72, 112 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 88, 96 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 48 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 192, 40 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 136, 16 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 80, 16 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 24 } }
};

static anim_t epsd1animinfo[] =
{
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 1 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 2 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 3 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 4 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 5 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 6 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 7 },
    { ANIM_LEVEL, TICRATE/3, 3, { 192, 144 }, 8 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 8 }
};

static anim_t epsd2animinfo[] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 168 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 40, 136 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 160, 96 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 80 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 120, 32 } },
    { ANIM_ALWAYS, TICRATE/4, 3, { 40, 0 } }
};

static int NUMANIMS[NUMEPISODES] =
{
    sizeof(epsd0animinfo)/sizeof(anim_t),
    sizeof(epsd1animinfo)/sizeof(anim_t),
    sizeof(epsd2animinfo)/sizeof(anim_t)
};

static anim_t *anims[NUMEPISODES] =
{
    epsd0animinfo,
    epsd1animinfo,
    epsd2animinfo
};



#define FB 0


#define SP_KILLS                0
#define SP_ITEMS                2
#define SP_SECRET               4
#define SP_FRAGS                6
#define SP_TIME                 8
#define SP_PAR                  ST_TIME

#define SP_PAUSE                1

#define SHOWNEXTLOCDELAY        4


static int              acceleratestage;

static int              me;

static stateenum_t      state;

static wbstartstruct_t* wbs;

static wbplayerstruct_t* plrs;

static int              cnt;

static int              bcnt;

static int              firstrefresh;

static int              cnt_kills[MAXPLAYERS];
static int              cnt_items[MAXPLAYERS];
static int              cnt_secret[MAXPLAYERS];
static int              cnt_time;
static int              cnt_par;
static int              cnt_pause;

static int              NUMCMAPS;



static patch_t*         bg;

static patch_t*         yah[2];

static patch_t*         splat;

static patch_t*         percent;
static patch_t*         colon;

static patch_t*         num[10];

static patch_t*         wiminus;

static patch_t*         finished;

static patch_t*         entering;

static patch_t*         sp_secret;

static patch_t*         kills;
static patch_t*         secret;
static patch_t*         items;
static patch_t*         frags;

static patch_t*         time;
static patch_t*         par;
static patch_t*         sucks;

static patch_t*         killers;
static patch_t*         victims;

static patch_t*         total;
static patch_t*         star;
static patch_t*         bstar;

static patch_t*         p[MAXPLAYERS];

static patch_t*         bp[MAXPLAYERS];

static patch_t**        lnames;




void WI_slamBackground(void)
{
    memcpy(screens[0], screens[1], SCREENWIDTH * SCREENHEIGHT);
    V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
}

boolean WI_Responder(event_t* ev)
{
    return false;
}


void WI_drawLF(void)
{
    int y = WI_TITLEY;

    V_DrawPatch((SCREENWIDTH - SHORT(lnames[wbs->last]->width))/2,
                y, FB, lnames[wbs->last]);

    y += (5*SHORT(lnames[wbs->last]->height))/4;

    V_DrawPatch((SCREENWIDTH - SHORT(finished->width))/2,
                y, FB, finished);
}



void WI_drawEL(void)
{
    int y = WI_TITLEY;

    V_DrawPatch((SCREENWIDTH - SHORT(entering->width))/2,
                y, FB, entering);

    y += (5*SHORT(lnames[wbs->next]->height))/4;

    V_DrawPatch((SCREENWIDTH - SHORT(lnames[wbs->next]->width))/2,
                y, FB, lnames[wbs->next]);

}

void
WI_drawOnLnode
( int           n,
  patch_t*      c[] )
{

    int         i;
    int         left;
    int         top;
    int         right;
    int         bottom;
    boolean     fits = false;

    i = 0;
    do
    {
        left = lnodes[wbs->epsd][n].x - SHORT(c[i]->leftoffset);
        top = lnodes[wbs->epsd][n].y - SHORT(c[i]->topoffset);
        right = left + SHORT(c[i]->width);
        bottom = top + SHORT(c[i]->height);

        if (left >= 0
            && right < SCREENWIDTH
            && top >= 0
            && bottom < SCREENHEIGHT)
        {
            fits = true;
        }
        else
        {
            i++;
        }
    } while (!fits && i!=2);

    if (fits && i<2)
    {
        V_DrawPatch(lnodes[wbs->epsd][n].x, lnodes[wbs->epsd][n].y,
                    FB, c[i]);
    }
    else
    {
        printf("Could not place patch on level %d", n+1);
    }
}



void WI_initAnimatedBack(void)
{
    int         i;
    anim_t*     a;

    if (gamemode == commercial)
        return;

    if (wbs->epsd > 2)
        return;

    for (i=0;i<NUMANIMS[wbs->epsd];i++)
    {
        a = &anims[wbs->epsd][i];

        a->ctr = -1;

        if (a->type == ANIM_ALWAYS)
            a->nexttic = bcnt + 1 + (M_Random()%a->period);
        else if (a->type == ANIM_RANDOM)
            a->nexttic = bcnt + 1 + a->data2+(M_Random()%a->data1);
        else if (a->type == ANIM_LEVEL)
            a->nexttic = bcnt + 1;
    }

}

void WI_updateAnimatedBack(void)
{
    int         i;
    anim_t*     a;

    if (gamemode == commercial)
        return;

    if (wbs->epsd > 2)
        return;

    for (i=0;i<NUMANIMS[wbs->epsd];i++)
    {
        a = &anims[wbs->epsd][i];

        if (bcnt == a->nexttic)
        {
            switch (a->type)
            {
              case ANIM_ALWAYS:
                if (++a->ctr >= a->nanims) a->ctr = 0;
                a->nexttic = bcnt + a->period;
                break;

              case ANIM_RANDOM:
                a->ctr++;
                if (a->ctr == a->nanims)
                {
                    a->ctr = -1;
                    a->nexttic = bcnt+a->data2+(M_Random()%a->data1);
                }
                else a->nexttic = bcnt + a->period;
                break;

              case ANIM_LEVEL:
                if (!(state == StatCount && i == 7)
                    && wbs->next == a->data1)
                {
                    a->ctr++;
                    if (a->ctr == a->nanims) a->ctr--;
                    a->nexttic = bcnt + a->period;
                }
                break;
            }
        }

    }

}

void WI_drawAnimatedBack(void)
{
    int                 i;
    anim_t*             a;

    if (commercial)
        return;

    if (wbs->epsd > 2)
        return;

    for (i=0 ; i<NUMANIMS[wbs->epsd] ; i++)
    {
        a = &anims[wbs->epsd][i];

        if (a->ctr >= 0)
            V_DrawPatch(a->loc.x, a->loc.y, FB, a->p[a->ctr]);
    }

}


int
WI_drawNum
( int           x,
  int           y,
  int           n,
  int           digits )
{

    int         fontwidth = SHORT(num[0]->width);
    int         neg;
    int         temp;

    if (digits < 0)
    {
        if (!n)
        {
            digits = 1;
        }
        else
        {
            digits = 0;
            temp = n;

            while (temp)
            {
                temp /= 10;
                digits++;
            }
        }
    }

    neg = n < 0;
    if (neg)
        n = -n;

    if (n == 1994)
        return 0;

    while (digits--)
    {
        x -= fontwidth;
        V_DrawPatch(x, y, FB, num[ n % 10 ]);
        n /= 10;
    }

    if (neg)
        V_DrawPatch(x-=8, y, FB, wiminus);

    return x;

}

void
WI_drawPercent
( int           x,
  int           y,
  int           p )
{
    if (p < 0)
        return;

    V_DrawPatch(x, y, FB, percent);
    WI_drawNum(x, y, p, -1);
}



void
WI_drawTime
( int           x,
  int           y,
  int           t )
{

    int         div;
    int         n;

    if (t<0)
        return;

    if (t <= 61*59)
    {
        div = 1;

        do
        {
            n = (t / div) % 60;
            x = WI_drawNum(x, y, n, 2) - SHORT(colon->width);
            div *= 60;

            if (div==60 || t / div)
                V_DrawPatch(x, y, FB, colon);

        } while (t / div);
    }
    else
    {
        V_DrawPatch(x - SHORT(sucks->width), y, FB, sucks);
    }
}


void WI_End(void)
{
    void WI_unloadData(void);
    WI_unloadData();
}

void WI_initNoState(void)
{
    state = NoState;
    acceleratestage = 0;
    cnt = 10;
}

void WI_updateNoState(void) {

    WI_updateAnimatedBack();

    if (!--cnt)
    {
        WI_End();
        G_WorldDone();
    }

}

static boolean          snl_pointeron = false;


void WI_initShowNextLoc(void)
{
    state = ShowNextLoc;
    acceleratestage = 0;
    cnt = SHOWNEXTLOCDELAY * TICRATE;

    WI_initAnimatedBack();
}

void WI_updateShowNextLoc(void)
{
    WI_updateAnimatedBack();

    if (!--cnt || acceleratestage)
        WI_initNoState();
    else
        snl_pointeron = (cnt & 31) < 20;
}

void WI_drawShowNextLoc(void)
{

    int         i;
    int         last;

    WI_slamBackground();

    WI_drawAnimatedBack();

    if ( gamemode != commercial)
    {
        if (wbs->epsd > 2)
        {
            WI_drawEL();
            return;
        }

        last = (wbs->last == 8) ? wbs->next - 1 : wbs->last;

        for (i=0 ; i<=last ; i++)
            WI_drawOnLnode(i, &splat);

        if (wbs->didsecret)
            WI_drawOnLnode(8, &splat);

        if (snl_pointeron)
            WI_drawOnLnode(wbs->next, yah);
    }

    if ( (gamemode != commercial)
         || wbs->next != 30)
        WI_drawEL();

}

void WI_drawNoState(void)
{
    snl_pointeron = true;
    WI_drawShowNextLoc();
}

int WI_fragSum(int playernum)
{
    int         i;
    int         frags = 0;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (playeringame[i]
            && i!=playernum)
        {
            frags += plrs[playernum].frags[i];
        }
    }


    frags -= plrs[playernum].frags[playernum];

    return frags;
}



static int              dm_state;
static int              dm_frags[MAXPLAYERS][MAXPLAYERS];
static int              dm_totals[MAXPLAYERS];



void WI_initDeathmatchStats(void)
{

    int         i;
    int         j;

    state = StatCount;
    acceleratestage = 0;
    dm_state = 1;

    cnt_pause = TICRATE;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (playeringame[i])
        {
            for (j=0 ; j<MAXPLAYERS ; j++)
                if (playeringame[j])
                    dm_frags[i][j] = 0;

            dm_totals[i] = 0;
        }
    }

    WI_initAnimatedBack();
}



void WI_updateDeathmatchStats(void)
{

    int         i;
    int         j;

    boolean     stillticking;

    WI_updateAnimatedBack();

    if (acceleratestage && dm_state != 4)
    {
        acceleratestage = 0;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (playeringame[i])
            {
                for (j=0 ; j<MAXPLAYERS ; j++)
                    if (playeringame[j])
                        dm_frags[i][j] = plrs[i].frags[j];

                dm_totals[i] = WI_fragSum(i);
            }
        }


        S_StartSound(0, sfx_barexp);
        dm_state = 4;
    }


    if (dm_state == 2)
    {
        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (playeringame[i])
            {
                for (j=0 ; j<MAXPLAYERS ; j++)
                {
                    if (playeringame[j]
                        && dm_frags[i][j] != plrs[i].frags[j])
                    {
                        if (plrs[i].frags[j] < 0)
                            dm_frags[i][j]--;
                        else
                            dm_frags[i][j]++;

                        if (dm_frags[i][j] > 99)
                            dm_frags[i][j] = 99;

                        if (dm_frags[i][j] < -99)
                            dm_frags[i][j] = -99;

                        stillticking = true;
                    }
                }
                dm_totals[i] = WI_fragSum(i);

                if (dm_totals[i] > 99)
                    dm_totals[i] = 99;

                if (dm_totals[i] < -99)
                    dm_totals[i] = -99;
            }

        }
        if (!stillticking)
        {
            S_StartSound(0, sfx_barexp);
            dm_state++;
        }

    }
    else if (dm_state == 4)
    {
        if (acceleratestage)
        {
            S_StartSound(0, sfx_slop);

            if ( gamemode == commercial)
                WI_initNoState();
            else
                WI_initShowNextLoc();
        }
    }
    else if (dm_state & 1)
    {
        if (!--cnt_pause)
        {
            dm_state++;
            cnt_pause = TICRATE;
        }
    }
}



void WI_drawDeathmatchStats(void)
{

    int         i;
    int         j;
    int         x;
    int         y;
    int         w;

    WI_slamBackground();

    WI_drawAnimatedBack();
    WI_drawLF();

    V_DrawPatch(DM_TOTALSX-SHORT(total->width)/2,
                DM_MATRIXY-WI_SPACINGY+10,
                FB,
                total);

    V_DrawPatch(DM_KILLERSX, DM_KILLERSY, FB, killers);
    V_DrawPatch(DM_VICTIMSX, DM_VICTIMSY, FB, victims);

    x = DM_MATRIXX + DM_SPACINGX;
    y = DM_MATRIXY;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (playeringame[i])
        {
            V_DrawPatch(x-SHORT(p[i]->width)/2,
                        DM_MATRIXY - WI_SPACINGY,
                        FB,
                        p[i]);

            V_DrawPatch(DM_MATRIXX-SHORT(p[i]->width)/2,
                        y,
                        FB,
                        p[i]);

            if (i == me)
            {
                V_DrawPatch(x-SHORT(p[i]->width)/2,
                            DM_MATRIXY - WI_SPACINGY,
                            FB,
                            bstar);

                V_DrawPatch(DM_MATRIXX-SHORT(p[i]->width)/2,
                            y,
                            FB,
                            star);
            }
        }
        else
        {
        }
        x += DM_SPACINGX;
        y += WI_SPACINGY;
    }

    y = DM_MATRIXY+10;
    w = SHORT(num[0]->width);

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        x = DM_MATRIXX + DM_SPACINGX;

        if (playeringame[i])
        {
            for (j=0 ; j<MAXPLAYERS ; j++)
            {
                if (playeringame[j])
                    WI_drawNum(x+w, y, dm_frags[i][j], 2);

                x += DM_SPACINGX;
            }
            WI_drawNum(DM_TOTALSX+w, y, dm_totals[i], 2);
        }
        y += WI_SPACINGY;
    }
}

static int      cnt_frags[MAXPLAYERS];
static int      dofrags;
static int      ng_state;

void WI_initNetgameStats(void)
{

    int i;

    state = StatCount;
    acceleratestage = 0;
    ng_state = 1;

    cnt_pause = TICRATE;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (!playeringame[i])
            continue;

        cnt_kills[i] = cnt_items[i] = cnt_secret[i] = cnt_frags[i] = 0;

        dofrags += WI_fragSum(i);
    }

    dofrags = !!dofrags;

    WI_initAnimatedBack();
}



void WI_updateNetgameStats(void)
{

    int         i;
    int         fsum;

    boolean     stillticking;

    WI_updateAnimatedBack();

    if (acceleratestage && ng_state != 10)
    {
        acceleratestage = 0;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
            cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;
            cnt_secret[i] = (plrs[i].ssecret * 100) / wbs->maxsecret;

            if (dofrags)
                cnt_frags[i] = WI_fragSum(i);
        }
        S_StartSound(0, sfx_barexp);
        ng_state = 10;
    }

    if (ng_state == 2)
    {
        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_kills[i] += 2;

            if (cnt_kills[i] >= (plrs[i].skills * 100) / wbs->maxkills)
                cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
            else
                stillticking = true;
        }

        if (!stillticking)
        {
            S_StartSound(0, sfx_barexp);
            ng_state++;
        }
    }
    else if (ng_state == 4)
    {
        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_items[i] += 2;
            if (cnt_items[i] >= (plrs[i].sitems * 100) / wbs->maxitems)
                cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;
            else
                stillticking = true;
        }
        if (!stillticking)
        {
            S_StartSound(0, sfx_barexp);
            ng_state++;
        }
    }
    else if (ng_state == 6)
    {
        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_secret[i] += 2;

            if (cnt_secret[i] >= (plrs[i].ssecret * 100) / wbs->maxsecret)
                cnt_secret[i] = (plrs[i].ssecret * 100) / wbs->maxsecret;
            else
                stillticking = true;
        }

        if (!stillticking)
        {
            S_StartSound(0, sfx_barexp);
            ng_state += 1 + 2*!dofrags;
        }
    }
    else if (ng_state == 8)
    {
        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_frags[i] += 1;

            if (cnt_frags[i] >= (fsum = WI_fragSum(i)))
                cnt_frags[i] = fsum;
            else
                stillticking = true;
        }

        if (!stillticking)
        {
            S_StartSound(0, sfx_pldeth);
            ng_state++;
        }
    }
    else if (ng_state == 10)
    {
        if (acceleratestage)
        {
            S_StartSound(0, sfx_sgcock);
            if ( gamemode == commercial )
                WI_initNoState();
            else
                WI_initShowNextLoc();
        }
    }
    else if (ng_state & 1)
    {
        if (!--cnt_pause)
        {
            ng_state++;
            cnt_pause = TICRATE;
        }
    }
}



void WI_drawNetgameStats(void)
{
    int         i;
    int         x;
    int         y;
    int         pwidth = SHORT(percent->width);

    WI_slamBackground();

    WI_drawAnimatedBack();

    WI_drawLF();

    V_DrawPatch(NG_STATSX+NG_SPACINGX-SHORT(kills->width),
                NG_STATSY, FB, kills);

    V_DrawPatch(NG_STATSX+2*NG_SPACINGX-SHORT(items->width),
                NG_STATSY, FB, items);

    V_DrawPatch(NG_STATSX+3*NG_SPACINGX-SHORT(secret->width),
                NG_STATSY, FB, secret);

    if (dofrags)
        V_DrawPatch(NG_STATSX+4*NG_SPACINGX-SHORT(frags->width),
                    NG_STATSY, FB, frags);

    y = NG_STATSY + SHORT(kills->height);

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (!playeringame[i])
            continue;

        x = NG_STATSX;
        V_DrawPatch(x-SHORT(p[i]->width), y, FB, p[i]);

        if (i == me)
            V_DrawPatch(x-SHORT(p[i]->width), y, FB, star);

        x += NG_SPACINGX;
        WI_drawPercent(x-pwidth, y+10, cnt_kills[i]);   x += NG_SPACINGX;
        WI_drawPercent(x-pwidth, y+10, cnt_items[i]);   x += NG_SPACINGX;
        WI_drawPercent(x-pwidth, y+10, cnt_secret[i]);  x += NG_SPACINGX;

        if (dofrags)
            WI_drawNum(x, y+10, cnt_frags[i], -1);

        y += WI_SPACINGY;
    }

}

static int      sp_state;

void WI_initStats(void)
{
    state = StatCount;
    acceleratestage = 0;
    sp_state = 1;
    cnt_kills[0] = cnt_items[0] = cnt_secret[0] = -1;
    cnt_time = cnt_par = -1;
    cnt_pause = TICRATE;

    WI_initAnimatedBack();
}

void WI_updateStats(void)
{

    WI_updateAnimatedBack();

    if (acceleratestage && sp_state != 10)
    {
        acceleratestage = 0;
        cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
        cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
        cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
        cnt_time = plrs[me].stime / TICRATE;
        cnt_par = wbs->partime / TICRATE;
        S_StartSound(0, sfx_barexp);
        sp_state = 10;
    }

    if (sp_state == 2)
    {
        cnt_kills[0] += 2;

        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        if (cnt_kills[0] >= (plrs[me].skills * 100) / wbs->maxkills)
        {
            cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
            S_StartSound(0, sfx_barexp);
            sp_state++;
        }
    }
    else if (sp_state == 4)
    {
        cnt_items[0] += 2;

        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        if (cnt_items[0] >= (plrs[me].sitems * 100) / wbs->maxitems)
        {
            cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
            S_StartSound(0, sfx_barexp);
            sp_state++;
        }
    }
    else if (sp_state == 6)
    {
        cnt_secret[0] += 2;

        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        if (cnt_secret[0] >= (plrs[me].ssecret * 100) / wbs->maxsecret)
        {
            cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
            S_StartSound(0, sfx_barexp);
            sp_state++;
        }
    }

    else if (sp_state == 8)
    {
        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        cnt_time += 3;

        if (cnt_time >= plrs[me].stime / TICRATE)
            cnt_time = plrs[me].stime / TICRATE;

        cnt_par += 3;

        if (cnt_par >= wbs->partime / TICRATE)
        {
            cnt_par = wbs->partime / TICRATE;

            if (cnt_time >= plrs[me].stime / TICRATE)
            {
                S_StartSound(0, sfx_barexp);
                sp_state++;
            }
        }
    }
    else if (sp_state == 10)
    {
        if (acceleratestage)
        {
            S_StartSound(0, sfx_sgcock);

            if (gamemode == commercial)
                WI_initNoState();
            else
                WI_initShowNextLoc();
        }
    }
    else if (sp_state & 1)
    {
        if (!--cnt_pause)
        {
            sp_state++;
            cnt_pause = TICRATE;
        }
    }

}

void WI_drawStats(void)
{
    int lh;

    lh = (3*SHORT(num[0]->height))/2;

    WI_slamBackground();

    WI_drawAnimatedBack();

    WI_drawLF();

    V_DrawPatch(SP_STATSX, SP_STATSY, FB, kills);
    WI_drawPercent(SCREENWIDTH - SP_STATSX, SP_STATSY, cnt_kills[0]);

    V_DrawPatch(SP_STATSX, SP_STATSY+lh, FB, items);
    WI_drawPercent(SCREENWIDTH - SP_STATSX, SP_STATSY+lh, cnt_items[0]);

    V_DrawPatch(SP_STATSX, SP_STATSY+2*lh, FB, sp_secret);
    WI_drawPercent(SCREENWIDTH - SP_STATSX, SP_STATSY+2*lh, cnt_secret[0]);

    V_DrawPatch(SP_TIMEX, SP_TIMEY, FB, time);
    WI_drawTime(SCREENWIDTH/2 - SP_TIMEX, SP_TIMEY, cnt_time);

    if (wbs->epsd < 3)
    {
        V_DrawPatch(SCREENWIDTH/2 + SP_TIMEX, SP_TIMEY, FB, par);
        WI_drawTime(SCREENWIDTH - SP_TIMEX, SP_TIMEY, cnt_par);
    }

}

void WI_checkForAccelerate(void)
{
    int   i;
    player_t  *player;

    for (i=0, player = players ; i<MAXPLAYERS ; i++, player++)
    {
        if (playeringame[i])
        {
            if (player->cmd.buttons & BT_ATTACK)
            {
                if (!player->attackdown)
                    acceleratestage = 1;
                player->attackdown = true;
            }
            else
                player->attackdown = false;
            if (player->cmd.buttons & BT_USE)
            {
                if (!player->usedown)
                    acceleratestage = 1;
                player->usedown = true;
            }
            else
                player->usedown = false;
        }
    }
}



void WI_Ticker(void)
{
    bcnt++;

    if (bcnt == 1)
    {
        if ( gamemode == commercial )
          S_ChangeMusic(mus_dm2int, true);
        else
          S_ChangeMusic(mus_inter, true);
    }

    WI_checkForAccelerate();

    switch (state)
    {
      case StatCount:
        if (deathmatch) WI_updateDeathmatchStats();
        else if (netgame) WI_updateNetgameStats();
        else WI_updateStats();
        break;

      case ShowNextLoc:
        WI_updateShowNextLoc();
        break;

      case NoState:
        WI_updateNoState();
        break;
    }

}

void WI_loadData(void)
{
    int         i;
    int         j;
    char        name[9];
    anim_t*     a;

    if (gamemode == commercial)
        strcpy(name, "INTERPIC");
    else
        sprintf(name, "WIMAP%d", wbs->epsd);

    if ( gamemode == retail )
    {
      if (wbs->epsd == 3)
        strcpy(name,"INTERPIC");
    }

    bg = W_CacheLumpName(name, PU_CACHE);
    V_DrawPatch(0, 0, 1, bg);



    if (gamemode == commercial)
    {
        NUMCMAPS = 32;
        lnames = (patch_t **) Z_Malloc(sizeof(patch_t*) * NUMCMAPS,
                                       PU_STATIC, 0);
        for (i=0 ; i<NUMCMAPS ; i++)
        {
            sprintf(name, "CWILV%2.2d", i);
            lnames[i] = W_CacheLumpName(name, PU_STATIC);
        }
    }
    else
    {
        lnames = (patch_t **) Z_Malloc(sizeof(patch_t*) * NUMMAPS,
                                       PU_STATIC, 0);
        for (i=0 ; i<NUMMAPS ; i++)
        {
            sprintf(name, "WILV%d%d", wbs->epsd, i);
            lnames[i] = W_CacheLumpName(name, PU_STATIC);
        }

        yah[0] = W_CacheLumpName("WIURH0", PU_STATIC);

        yah[1] = W_CacheLumpName("WIURH1", PU_STATIC);

        splat = W_CacheLumpName("WISPLAT", PU_STATIC);

        if (wbs->epsd < 3)
        {
            for (j=0;j<NUMANIMS[wbs->epsd];j++)
            {
                a = &anims[wbs->epsd][j];
                for (i=0;i<a->nanims;i++)
                {
                    if (wbs->epsd != 1 || j != 8)
                    {
                        sprintf(name, "WIA%d%.2d%.2d", wbs->epsd, j, i);
                        a->p[i] = W_CacheLumpName(name, PU_STATIC);
                    }
                    else
                    {
                        a->p[i] = anims[1][4].p[i];
                    }
                }
            }
        }
    }

    wiminus = W_CacheLumpName("WIMINUS", PU_STATIC);

    for (i=0;i<10;i++)
    {
        sprintf(name, "WINUM%d", i);
        num[i] = W_CacheLumpName(name, PU_STATIC);
    }

    percent = W_CacheLumpName("WIPCNT", PU_STATIC);

    finished = W_CacheLumpName("WIF", PU_STATIC);

    entering = W_CacheLumpName("WIENTER", PU_STATIC);

    kills = W_CacheLumpName("WIOSTK", PU_STATIC);

    secret = W_CacheLumpName("WIOSTS", PU_STATIC);

    sp_secret = W_CacheLumpName("WISCRT2", PU_STATIC);

    if (french)
    {
        if (netgame && !deathmatch)
            items = W_CacheLumpName("WIOBJ", PU_STATIC);
        else
            items = W_CacheLumpName("WIOSTI", PU_STATIC);
    } else
        items = W_CacheLumpName("WIOSTI", PU_STATIC);

    frags = W_CacheLumpName("WIFRGS", PU_STATIC);

    colon = W_CacheLumpName("WICOLON", PU_STATIC);

    time = W_CacheLumpName("WITIME", PU_STATIC);

    sucks = W_CacheLumpName("WISUCKS", PU_STATIC);

    par = W_CacheLumpName("WIPAR", PU_STATIC);

    killers = W_CacheLumpName("WIKILRS", PU_STATIC);

    victims = W_CacheLumpName("WIVCTMS", PU_STATIC);

    total = W_CacheLumpName("WIMSTT", PU_STATIC);

    star = W_CacheLumpName("STFST01", PU_STATIC);

    bstar = W_CacheLumpName("STFDEAD0", PU_STATIC);

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        sprintf(name, "STPB%d", i);
        p[i] = W_CacheLumpName(name, PU_STATIC);

        sprintf(name, "WIBP%d", i+1);
        bp[i] = W_CacheLumpName(name, PU_STATIC);
    }

}

void WI_unloadData(void)
{
    int         i;
    int         j;

    Z_ChangeTag(wiminus, PU_CACHE);

    for (i=0 ; i<10 ; i++)
        Z_ChangeTag(num[i], PU_CACHE);

    if (gamemode == commercial)
    {
        for (i=0 ; i<NUMCMAPS ; i++)
            Z_ChangeTag(lnames[i], PU_CACHE);
    }
    else
    {
        Z_ChangeTag(yah[0], PU_CACHE);
        Z_ChangeTag(yah[1], PU_CACHE);

        Z_ChangeTag(splat, PU_CACHE);

        for (i=0 ; i<NUMMAPS ; i++)
            Z_ChangeTag(lnames[i], PU_CACHE);

        if (wbs->epsd < 3)
        {
            for (j=0;j<NUMANIMS[wbs->epsd];j++)
            {
                if (wbs->epsd != 1 || j != 8)
                    for (i=0;i<anims[wbs->epsd][j].nanims;i++)
                        Z_ChangeTag(anims[wbs->epsd][j].p[i], PU_CACHE);
            }
        }
    }

    Z_Free(lnames);

    Z_ChangeTag(percent, PU_CACHE);
    Z_ChangeTag(colon, PU_CACHE);
    Z_ChangeTag(finished, PU_CACHE);
    Z_ChangeTag(entering, PU_CACHE);
    Z_ChangeTag(kills, PU_CACHE);
    Z_ChangeTag(secret, PU_CACHE);
    Z_ChangeTag(sp_secret, PU_CACHE);
    Z_ChangeTag(items, PU_CACHE);
    Z_ChangeTag(frags, PU_CACHE);
    Z_ChangeTag(time, PU_CACHE);
    Z_ChangeTag(sucks, PU_CACHE);
    Z_ChangeTag(par, PU_CACHE);

    Z_ChangeTag(victims, PU_CACHE);
    Z_ChangeTag(killers, PU_CACHE);
    Z_ChangeTag(total, PU_CACHE);

    for (i=0 ; i<MAXPLAYERS ; i++)
        Z_ChangeTag(p[i], PU_CACHE);

    for (i=0 ; i<MAXPLAYERS ; i++)
        Z_ChangeTag(bp[i], PU_CACHE);
}

void WI_Drawer (void)
{
    switch (state)
    {
      case StatCount:
        if (deathmatch)
            WI_drawDeathmatchStats();
        else if (netgame)
            WI_drawNetgameStats();
        else
            WI_drawStats();
        break;

      case ShowNextLoc:
        WI_drawShowNextLoc();
        break;

      case NoState:
        WI_drawNoState();
        break;
    }
}


void WI_initVariables(wbstartstruct_t* wbstartstruct)
{

    wbs = wbstartstruct;

#ifdef RANGECHECKING
    if (gamemode != commercial)
    {
      if ( gamemode == retail )
        RNGCHECK(wbs->epsd, 0, 3);
      else
        RNGCHECK(wbs->epsd, 0, 2);
    }
    else
    {
        RNGCHECK(wbs->last, 0, 8);
        RNGCHECK(wbs->next, 0, 8);
    }
    RNGCHECK(wbs->pnum, 0, MAXPLAYERS);
    RNGCHECK(wbs->pnum, 0, MAXPLAYERS);
#endif

    acceleratestage = 0;
    cnt = bcnt = 0;
    firstrefresh = 1;
    me = wbs->pnum;
    plrs = wbs->plyr;

    if (!wbs->maxkills)
        wbs->maxkills = 1;

    if (!wbs->maxitems)
        wbs->maxitems = 1;

    if (!wbs->maxsecret)
        wbs->maxsecret = 1;

    if ( gamemode != retail )
      if (wbs->epsd > 2)
        wbs->epsd -= 3;
}

void WI_Start(wbstartstruct_t* wbstartstruct)
{

    WI_initVariables(wbstartstruct);
    WI_loadData();

    if (deathmatch)
        WI_initDeathmatchStats();
    else if (netgame)
        WI_initNetgameStats();
    else
        WI_initStats();
}
