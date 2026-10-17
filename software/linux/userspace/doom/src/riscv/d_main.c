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
//      DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//      plus functions to determine game mode (shareware, registered),
//      parse command line parameters, configure game parameters (turbo),
//      and call the startup functions.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: d_main.c,v 1.8 1997/02/03 22:45:09 b1 Exp $";


#define BGCOLOR         7
#define FGCOLOR         8


#ifdef NORMALUNIX
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif


#include "doomdef.h"
#include "doomstat.h"

#include "dstrings.h"
#include "sounds.h"


#include "z_zone.h"
#include "w_wad.h"
#include "s_sound.h"
#include "v_video.h"

#include "f_finale.h"
#include "f_wipe.h"

#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"

#include "i_system.h"
#include "i_sound.h"
#include "i_video.h"

#include "g_game.h"

#include "hu_stuff.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"

#include "p_setup.h"
#include "r_local.h"


#include "d_main.h"
#include "console.h"

void D_DoomLoop (void);


char*           wadfiles[MAXWADFILES];


boolean         devparm;
boolean         nomonsters;
boolean         respawnparm;
boolean         fastparm;

boolean         drone;

boolean         singletics = false;




extern  boolean inhelpscreens;

skill_t         startskill;
int             startepisode;
int             startmap;
boolean         autostart;

boolean         advancedemo;




char            wadfile[1024];
char            mapdir[1024];
char            basedefault[] = "doomrc";


void D_CheckNetGame (void);
void D_ProcessEvents (void);
void G_BuildTiccmd (ticcmd_t* cmd);
void D_DoAdvanceDemo (void);


event_t         events[MAXEVENTS];
int             eventhead;
int             eventtail;


void D_PostEvent (event_t* ev)
{
    events[eventhead] = *ev;
    eventhead = (eventhead+1)&(MAXEVENTS-1);
}


void D_ProcessEvents (void)
{
    event_t*    ev;

    for ( ; eventtail != eventhead ; eventtail = (eventtail+1)&(MAXEVENTS-1) )
    {
        ev = &events[eventtail];
        if (M_Responder (ev))
            continue;
        G_Responder (ev);
    }
}





gamestate_t     wipegamestate = GS_DEMOSCREEN;
extern  boolean setsizeneeded;
extern  int             showMessages;
void R_ExecuteSetViewSize (void);

void D_Display (void)
{
    static  boolean             viewactivestate = false;
    static  boolean             menuactivestate = false;
    static  boolean             inhelpscreensstate = false;
    static  boolean             fullscreen = false;
    static  gamestate_t         oldgamestate = -1;
    static  int                 borderdrawcount;
    int                         nowtime;
    int                         tics;
    int                         wipestart;
    int                         y;
    boolean                     done;
    boolean                     wipe;
    boolean                     redrawsbar;

    if (nodrawers)
        return;

    redrawsbar = false;

    if (setsizeneeded)
    {
        R_ExecuteSetViewSize ();
        oldgamestate = -1;
        borderdrawcount = 3;
    }

    if (gamestate != wipegamestate)
    {
        wipe = true;
        wipe_StartScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);
    }
    else
        wipe = false;

    if (gamestate == GS_LEVEL && gametic)
        HU_Erase();

    switch (gamestate)
    {
      case GS_LEVEL:
        if (!gametic)
            break;
        if (automapactive)
            AM_Drawer ();
        if (wipe || (viewheight != 200 && fullscreen) )
            redrawsbar = true;
        if (inhelpscreensstate && !inhelpscreens)
            redrawsbar = true;
        ST_Drawer (viewheight == 200, redrawsbar );
        fullscreen = viewheight == 200;
        break;

      case GS_INTERMISSION:
        WI_Drawer ();
        break;

      case GS_FINALE:
        F_Drawer ();
        break;

      case GS_DEMOSCREEN:
        D_PageDrawer ();
        break;
    }

    I_UpdateNoBlit ();

    if (gamestate == GS_LEVEL && !automapactive && gametic)
        R_RenderPlayerView (&players[displayplayer]);

    if (gamestate == GS_LEVEL && gametic)
        HU_Drawer ();

    if (gamestate != oldgamestate && gamestate != GS_LEVEL)
        I_SetPalette (W_CacheLumpName ("PLAYPAL",PU_CACHE));

    if (gamestate == GS_LEVEL && oldgamestate != GS_LEVEL)
    {
        viewactivestate = false;
        R_FillBackScreen ();
    }

    if (gamestate == GS_LEVEL && !automapactive && scaledviewwidth != 320)
    {
        if (menuactive || menuactivestate || !viewactivestate)
            borderdrawcount = 3;
        if (borderdrawcount)
        {
            R_DrawViewBorder ();
            borderdrawcount--;
        }

    }

    menuactivestate = menuactive;
    viewactivestate = viewactive;
    inhelpscreensstate = inhelpscreens;
    oldgamestate = wipegamestate = gamestate;

    if (paused)
    {
        if (automapactive)
            y = 4;
        else
            y = viewwindowy+4;
        V_DrawPatchDirect(viewwindowx+(scaledviewwidth-68)/2,
                          y,0,W_CacheLumpName ("M_PAUSE", PU_CACHE));
    }


    M_Drawer ();
    NetUpdate ();


    if (!wipe)
    {
        I_FinishUpdate ();
        return;
    }

    wipe_EndScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

    wipestart = I_GetTime () - 1;

    do
    {
        do
        {
            nowtime = I_GetTime ();
            tics = nowtime - wipestart;
        } while (!tics);
        wipestart = nowtime;
        done = wipe_ScreenWipe(wipe_Melt
                               , 0, 0, SCREENWIDTH, SCREENHEIGHT, tics);
        I_UpdateNoBlit ();
        M_Drawer ();
        I_FinishUpdate ();
    } while (!done);
}



void D_DoomLoop (void)
{
    I_InitGraphics ();

    while (1)
    {
        I_StartFrame ();

        if (singletics)
        {
            I_StartTic ();
            D_ProcessEvents ();
            G_BuildTiccmd (&netcmds[consoleplayer][maketic%BACKUPTICS]);
            if (advancedemo)
                D_DoAdvanceDemo ();
            M_Ticker ();
            G_Ticker ();
            gametic++;
            maketic++;
        }
        else
        {
            TryRunTics ();
        }

        D_Display ();
    }
}



int             demosequence;
int             pagetic;
char                    *pagename;


void D_PageTicker (void)
{
    if (--pagetic < 0)
        D_AdvanceDemo ();
}



void D_PageDrawer (void)
{
    V_DrawPatch (0,0, 0, W_CacheLumpName(pagename, PU_CACHE));
}


void D_AdvanceDemo (void)
{
    advancedemo = true;
}


 void D_DoAdvanceDemo (void)
{
    players[consoleplayer].playerstate = PST_LIVE;
    advancedemo = false;
    usergame = false;
    paused = false;
    gameaction = ga_nothing;

    if ( gamemode == retail )
      demosequence = (demosequence+1)%7;
    else
      demosequence = (demosequence+1)%6;

    switch (demosequence)
    {
      case 0:
        if ( gamemode == commercial )
            pagetic = 35 * 11;
        else
            pagetic = 170;
        gamestate = GS_DEMOSCREEN;
        pagename = "TITLEPIC";
        if ( gamemode == commercial )
          S_StartMusic(mus_dm2ttl);
        else
          S_StartMusic (mus_intro);
        break;
      case 1:
        G_DeferedPlayDemo ("demo1");
        break;
      case 2:
        pagetic = 200;
        gamestate = GS_DEMOSCREEN;
        pagename = "CREDIT";
        break;
      case 3:
        G_DeferedPlayDemo ("demo2");
        break;
      case 4:
        gamestate = GS_DEMOSCREEN;
        if ( gamemode == commercial)
        {
            pagetic = 35 * 11;
            pagename = "TITLEPIC";
            S_StartMusic(mus_dm2ttl);
        }
        else
        {
            pagetic = 200;

            if ( gamemode == retail )
              pagename = "CREDIT";
            else
              pagename = "HELP2";
        }
        break;
      case 5:
        G_DeferedPlayDemo ("demo3");
        break;
      case 6:
        G_DeferedPlayDemo ("demo4");
        break;
    }
}



void D_StartTitle (void)
{
    gameaction = ga_nothing;
    demosequence = -1;
    D_AdvanceDemo ();
}






void D_AddFile (char *file)
{
    int     numwadfiles;
    char    *newfile;

    for (numwadfiles = 0 ; wadfiles[numwadfiles] ; numwadfiles++)
        ;

    newfile = malloc (strlen(file)+1);
    strcpy (newfile, file);

    wadfiles[numwadfiles] = newfile;
}

void IdentifyVersion (void)
{
    devparm = false;

    gamemode = shareware;
    D_AddFile ("doom1.wad");
}


void D_DoomMain (void)
{
    console_puts ("[UART] D_DoomMain: start\r\n");

    IdentifyVersion ();
    console_puts ("[UART] IdentifyVersion: doom1.wad queued\r\n");

    setbuf (stdout, NULL);
    modifiedgame = false;

    nomonsters  = false;
    respawnparm = false;
    fastparm    = false;
    devparm     = false;
    deathmatch  = 0;

    startskill   = sk_medium;
    startepisode = 1;
    startmap     = 1;
    autostart    = false;

#ifdef ZCORE_DOOM
    int warp = M_CheckParm ("-warp");
    int skill = M_CheckParm ("-skill");
    if (warp)
    {
        if (warp + 2 >= myargc)
            I_Error ("-warp requires episode and map");
        startepisode = atoi (myargv[warp + 1]);
        startmap = atoi (myargv[warp + 2]);
        if (startepisode != 1 || startmap < 1 || startmap > 9)
            I_Error ("Shareware maps are episode 1, maps 1 through 9");
        autostart = true;
    }
    if (skill)
    {
        if (skill + 1 >= myargc || atoi (myargv[skill + 1]) < 1 ||
            atoi (myargv[skill + 1]) > 5)
            I_Error ("-skill requires 1 through 5");
        startskill = atoi (myargv[skill + 1]) - 1;
    }
#endif

    printf ( "----------------------------\n"
             "RISC-V DOOM Startup v%i.%i\n"
             "----------------------------\n",
             VERSION/100,VERSION%100);

    printf ("V_Init: allocate screens.\n");
    V_Init ();

    printf ("M_LoadDefaults: Load system defaults.\n");
    M_LoadDefaults ();

    printf ("Z_Init: Init zone memory allocation daemon. \n");
    Z_Init ();
    console_puts ("[UART] Z_Init done\r\n");

    printf ("W_Init: Init WADfiles.\n");
    W_InitMultipleFiles (wadfiles);
    console_printf ("[UART] W_Init: numlumps=%d\r\n", numlumps);

    printf ("M_Init: Init miscellaneous info.\n");
    M_Init ();
    console_puts ("[UART] M_Init done\r\n");

    printf ("R_Init: Init DOOM refresh daemon - ");
    R_Init ();
    console_puts ("[UART] R_Init done\r\n");

    printf ("\nP_Init: Init Playloop state.\n");
    P_Init ();
    console_puts ("[UART] P_Init done\r\n");

    printf ("I_Init: Setting up machine state.\n");
    I_Init ();
    console_puts ("[UART] I_Init done\r\n");

    printf ("D_CheckNetGame: Checking network game status.\n");
    D_CheckNetGame ();
    console_puts ("[UART] D_CheckNetGame done\r\n");

    printf ("S_Init: Setting up sound.\n");
    S_Init (snd_SfxVolume  , snd_MusicVolume   );
    console_puts ("[UART] S_Init done\r\n");

    printf ("HU_Init: Setting up heads up display.\n");
    HU_Init ();
    console_puts ("[UART] HU_Init done\r\n");

    printf ("ST_Init: Init status bar.\n");
    ST_Init ();
    console_puts ("[UART] ST_Init done, entering title/demo loop\r\n");

    if ( gameaction != ga_loadgame )
    {
        if (autostart || netgame)
            G_InitNew (startskill, startepisode, startmap);
        else
            D_StartTitle ();
    }

    D_DoomLoop ();
}
