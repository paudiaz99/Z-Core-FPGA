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

#ifdef DEBUG
FILE*           debugfile;
#endif

boolean         advancedemo;




char            wadfile[1024];
char            mapdir[1024];
char            basedefault[1024];


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

    if ( ( gamemode == commercial )
         && (W_CheckNumForName("map01")<0) )
      return;

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



extern  boolean         demorecording;

void D_DoomLoop (void)
{
    if (demorecording)
        G_BeginRecording ();

#ifdef DEBUG
    if (M_CheckParm ("-debugfile"))
    {
        char    filename[20];
        sprintf (filename,"debug%i.txt",consoleplayer);
        printf ("debug output to: %s\n",filename);
        debugfile = fopen (filename,"w");
    }
#endif

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

        S_UpdateSounds (players[consoleplayer].mo);

        D_Display ();

#ifndef SNDSERV
        I_UpdateSound();
#endif
#ifndef SNDINTR
        I_SubmitSound();
#endif
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




char            title[128];



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

    char*       doom1wad;
    char*       doomwad;
    char*       doomuwad;
    char*       doom2wad;

    char*       doom2fwad;
    char*       plutoniawad;
    char*       tntwad;

#ifdef NORMALUNIX
    char *home;
    char *doomwaddir;
    doomwaddir = getenv("DOOMWADDIR");
    if (!doomwaddir) {
        doom2wad    = "doom2.wad";
        doomuwad    = "doomu.wad";
        doomwad     = "doom.wad";
        doom1wad    = "doom1.wad";
        plutoniawad = "plutonia.wad";
        tntwad      = "tnt.wad";
        doom2fwad   = "doom2f.wad";
    } else {
        doom2wad = malloc(strlen(doomwaddir)+1+9+1);
        sprintf(doom2wad, "%s/doom2.wad", doomwaddir);

        doomuwad = malloc(strlen(doomwaddir)+1+8+1);
        sprintf(doomuwad, "%s/doomu.wad", doomwaddir);

        doomwad = malloc(strlen(doomwaddir)+1+8+1);
        sprintf(doomwad, "%s/doom.wad", doomwaddir);

        doom1wad = malloc(strlen(doomwaddir)+1+9+1);
        sprintf(doom1wad, "%s/doom1.wad", doomwaddir);

        plutoniawad = malloc(strlen(doomwaddir)+1+ 12+1);
        sprintf(plutoniawad, "%s/plutonia.wad", doomwaddir);

        tntwad = malloc(strlen(doomwaddir)+1+9+1);
        sprintf(tntwad, "%s/tnt.wad", doomwaddir);

        doom2fwad = malloc(strlen(doomwaddir)+1+10+1);
        sprintf(doom2fwad, "%s/doom2f.wad", doomwaddir);
    }

    home = getenv("HOME");
    if (home)
        sprintf(basedefault, "%s/.doomrc", home);
    else
        sprintf(basedefault, "doomrc");
#endif

    if (M_CheckParm ("-shdev"))
    {
        gamemode = shareware;
        devparm = true;
        D_AddFile (DEVDATA"doom1.wad");
        D_AddFile (DEVMAPS"data_se/texture1.lmp");
        D_AddFile (DEVMAPS"data_se/pnames.lmp");
        strcpy (basedefault,DEVDATA"default.cfg");
        return;
    }

    if (M_CheckParm ("-regdev"))
    {
        gamemode = registered;
        devparm = true;
        D_AddFile (DEVDATA"doom.wad");
        D_AddFile (DEVMAPS"data_se/texture1.lmp");
        D_AddFile (DEVMAPS"data_se/texture2.lmp");
        D_AddFile (DEVMAPS"data_se/pnames.lmp");
        strcpy (basedefault,DEVDATA"default.cfg");
        return;
    }

    if (M_CheckParm ("-comdev"))
    {
        gamemode = commercial;
        devparm = true;
            D_AddFile (DEVDATA"doom2.wad");

        D_AddFile (DEVMAPS"cdata/texture1.lmp");
        D_AddFile (DEVMAPS"cdata/pnames.lmp");
        strcpy (basedefault,DEVDATA"default.cfg");
        return;
    }

    if ( !access (doom2fwad,R_OK) )
    {
        gamemode = commercial;
        language = french;
        printf("French version\n");
        D_AddFile (doom2fwad);
        return;
    }

    if ( !access (doom2wad,R_OK) )
    {
        gamemode = commercial;
        D_AddFile (doom2wad);
        return;
    }

    if ( !access (plutoniawad, R_OK ) )
    {
      gamemode = commercial;
      D_AddFile (plutoniawad);
      return;
    }

    if ( !access ( tntwad, R_OK ) )
    {
      gamemode = commercial;
      D_AddFile (tntwad);
      return;
    }

    if ( !access (doomuwad,R_OK) )
    {
      gamemode = retail;
      D_AddFile (doomuwad);
      return;
    }

    if ( !access (doomwad,R_OK) )
    {
      gamemode = registered;
      D_AddFile (doomwad);
      return;
    }

    if ( !access (doom1wad,R_OK) )
    {
      gamemode = shareware;
      D_AddFile (doom1wad);
      return;
    }

    printf("Game mode indeterminate.\n");
    gamemode = indetermined;

}

void FindResponseFile (void)
{
    int             i;
#define MAXARGVS        100

    for (i = 1;i < myargc;i++)
        if (myargv[i][0] == '@')
        {
            FILE *          handle;
            int             size;
            int             k;
            int             index;
            int             indexinfile;
            char    *infile;
            char    *file;
            char    *moreargs[20];
            char    *firstargv;

            handle = fopen (&myargv[i][1],"rb");
            if (!handle)
            {
                printf ("\nNo such response file!");
                exit(1);
            }
            printf("Found response file %s!\n",&myargv[i][1]);
            fseek (handle,0,SEEK_END);
            size = ftell(handle);
            fseek (handle,0,SEEK_SET);
            file = malloc (size);
            fread (file,size,1,handle);
            fclose (handle);

            for (index = 0,k = i+1; k < myargc; k++)
                moreargs[index++] = myargv[k];

            firstargv = myargv[0];
            myargv = malloc(sizeof(char *)*MAXARGVS);
            memset(myargv,0,sizeof(char *)*MAXARGVS);
            myargv[0] = firstargv;

            infile = file;
            indexinfile = k = 0;
            indexinfile++;
            do
            {
                myargv[indexinfile++] = infile+k;
                while(k < size &&
                      ((*(infile+k)>= ' '+1) && (*(infile+k)<='z')))
                    k++;
                *(infile+k) = 0;
                while(k < size &&
                      ((*(infile+k)<= ' ') || (*(infile+k)>'z')))
                    k++;
            } while(k < size);

            for (k = 0;k < index;k++)
                myargv[indexinfile++] = moreargs[k];
            myargc = indexinfile;

            printf("%d command-line args:\n",myargc);
            for (k=1;k<myargc;k++)
                printf("%s\n",myargv[k]);

            break;
        }
}


void D_DoomMain (void)
{
    int             p;
    char                    file[256];

    FindResponseFile ();

    IdentifyVersion ();

    setbuf (stdout, NULL);
    modifiedgame = false;

    nomonsters = M_CheckParm ("-nomonsters");
    respawnparm = M_CheckParm ("-respawn");
    fastparm = M_CheckParm ("-fast");
    devparm = M_CheckParm ("-devparm");
    if (M_CheckParm ("-altdeath"))
        deathmatch = 2;
    else if (M_CheckParm ("-deathmatch"))
        deathmatch = 1;

    switch ( gamemode )
    {
      case retail:
        sprintf (title,
                 "                         "
                 "The Ultimate DOOM Startup v%i.%i"
                 "                           ",
                 VERSION/100,VERSION%100);
        break;
      case shareware:
        sprintf (title,
                 "                            "
                 "DOOM Shareware Startup v%i.%i"
                 "                           ",
                 VERSION/100,VERSION%100);
        break;
      case registered:
        sprintf (title,
                 "                            "
                 "DOOM Registered Startup v%i.%i"
                 "                           ",
                 VERSION/100,VERSION%100);
        break;
      case commercial:
        sprintf (title,
                 "                         "
                 "DOOM 2: Hell on Earth v%i.%i"
                 "                           ",
                 VERSION/100,VERSION%100);
        break;
      default:
        sprintf (title,
                 "                     "
                 "Public DOOM - v%i.%i"
                 "                           ",
                 VERSION/100,VERSION%100);
        break;
    }

    printf ("%s\n",title);

    if (devparm)
        printf(D_DEVSTR);

    if ( (p=M_CheckParm ("-turbo")) )
    {
        int     scale = 200;
        extern int forwardmove[2];
        extern int sidemove[2];

        if (p<myargc-1)
            scale = atoi (myargv[p+1]);
        if (scale < 10)
            scale = 10;
        if (scale > 400)
            scale = 400;
        printf ("turbo scale: %i%%\n",scale);
        forwardmove[0] = forwardmove[0]*scale/100;
        forwardmove[1] = forwardmove[1]*scale/100;
        sidemove[0] = sidemove[0]*scale/100;
        sidemove[1] = sidemove[1]*scale/100;
    }

    p = M_CheckParm ("-wart");
    if (p)
    {
        myargv[p][4] = 'p';

        switch (gamemode )
        {
          case shareware:
          case retail:
          case registered:
            sprintf (file,"~"DEVMAPS"E%cM%c.wad",
                     myargv[p+1][0], myargv[p+2][0]);
            printf("Warping to Episode %s, Map %s.\n",
                   myargv[p+1],myargv[p+2]);
            break;

          case commercial:
          default:
            p = atoi (myargv[p+1]);
            if (p<10)
              sprintf (file,"~"DEVMAPS"cdata/map0%i.wad", p);
            else
              sprintf (file,"~"DEVMAPS"cdata/map%i.wad", p);
            break;
        }
        D_AddFile (file);
    }

    p = M_CheckParm ("-file");
    if (p)
    {
        modifiedgame = true;
        while (++p != myargc && myargv[p][0] != '-')
            D_AddFile (myargv[p]);
    }

    p = M_CheckParm ("-playdemo");

    if (!p)
        p = M_CheckParm ("-timedemo");

    if (p && p < myargc-1)
    {
        sprintf (file,"%s.lmp", myargv[p+1]);
        D_AddFile (file);
        printf("Playing demo %s.lmp.\n",myargv[p+1]);
    }

    startskill = sk_medium;
    startepisode = 1;
    startmap = 1;
    autostart = false;


    p = M_CheckParm ("-skill");
    if (p && p < myargc-1)
    {
        startskill = myargv[p+1][0]-'1';
        autostart = true;
    }

    p = M_CheckParm ("-episode");
    if (p && p < myargc-1)
    {
        startepisode = myargv[p+1][0]-'0';
        startmap = 1;
        autostart = true;
    }

    p = M_CheckParm ("-timer");
    if (p && p < myargc-1 && deathmatch)
    {
        int     time;
        time = atoi(myargv[p+1]);
        printf("Levels will end after %d minute",time);
        if (time>1)
            printf("s");
        printf(".\n");
    }

    p = M_CheckParm ("-avg");
    if (p && p < myargc-1 && deathmatch)
        printf("Austin Virtual Gaming: Levels will end after 20 minutes\n");

    p = M_CheckParm ("-warp");
    if (p && p < myargc-1)
    {
        if (gamemode == commercial)
            startmap = atoi (myargv[p+1]);
        else
        {
            startepisode = myargv[p+1][0]-'0';
            startmap = myargv[p+2][0]-'0';
        }
        autostart = true;
    }

    printf ("V_Init: allocate screens.\n");
    V_Init ();

    printf ("M_LoadDefaults: Load system defaults.\n");
    M_LoadDefaults ();

    printf ("Z_Init: Init zone memory allocation daemon. \n");
    Z_Init ();

    printf ("W_Init: Init WADfiles.\n");
    W_InitMultipleFiles (wadfiles);


    if (modifiedgame)
    {
        char name[23][8]=
        {
            "e2m1","e2m2","e2m3","e2m4","e2m5","e2m6","e2m7","e2m8","e2m9",
            "e3m1","e3m3","e3m3","e3m4","e3m5","e3m6","e3m7","e3m8","e3m9",
            "dphoof","bfgga0","heada1","cybra1","spida1d1"
        };
        int i;

        if ( gamemode == shareware)
            I_Error("\nYou cannot -file with the shareware "
                    "version. Register!");

        if (gamemode == registered)
            for (i = 0;i < 23; i++)
                if (W_CheckNumForName(name[i])<0)
                    I_Error("\nThis is not the registered version.");
    }

    if (modifiedgame)
    {
         printf (
            "===========================================================================\n"
            "ATTENTION:  This version of DOOM has been modified.  If you would like to\n"
            "get a copy of the original game, call 1-800-IDGAMES or see the readme file.\n"
            "        You will not receive technical support for modified games.\n"
            "                      press enter to continue\n"
            "===========================================================================\n"
            );
        getchar ();
    }


    switch ( gamemode )
    {
      case shareware:
      case indetermined:
        printf (
            "===========================================================================\n"
            "                                Shareware!\n"
            "===========================================================================\n"
        );
        break;
      case registered:
      case retail:
      case commercial:
        printf (
            "===========================================================================\n"
            "                 Commercial product - do not distribute!\n"
            "         Please report software piracy to the SPA: 1-800-388-PIR8\n"
            "===========================================================================\n"
        );
        break;

      default:
        break;
    }

    printf ("M_Init: Init miscellaneous info.\n");
    M_Init ();

    printf ("R_Init: Init DOOM refresh daemon - ");
    R_Init ();

    printf ("\nP_Init: Init Playloop state.\n");
    P_Init ();

    printf ("I_Init: Setting up machine state.\n");
    I_Init ();

    printf ("D_CheckNetGame: Checking network game status.\n");
    D_CheckNetGame ();

    printf ("S_Init: Setting up sound.\n");
    S_Init (snd_SfxVolume  , snd_MusicVolume   );

    printf ("HU_Init: Setting up heads up display.\n");
    HU_Init ();

    printf ("ST_Init: Init status bar.\n");
    ST_Init ();

    p = M_CheckParm ("-statcopy");
    if (p && p<myargc-1)
    {
        extern  void*   statcopy;

        statcopy = (void*)atoi(myargv[p+1]);
        printf ("External statistics registered.\n");
    }

    p = M_CheckParm ("-record");

    if (p && p < myargc-1)
    {
        G_RecordDemo (myargv[p+1]);
        autostart = true;
    }

    p = M_CheckParm ("-playdemo");
    if (p && p < myargc-1)
    {
        singledemo = true;
        G_DeferedPlayDemo (myargv[p+1]);
        D_DoomLoop ();
    }

    p = M_CheckParm ("-timedemo");
    if (p && p < myargc-1)
    {
        G_TimeDemo (myargv[p+1]);
        D_DoomLoop ();
    }

    p = M_CheckParm ("-loadgame");
    if (p && p < myargc-1)
    {
        sprintf(file, SAVEGAMENAME"%c.dsg",myargv[p+1][0]);
        G_LoadGame (file);
    }


    if ( gameaction != ga_loadgame )
    {
        if (autostart || netgame)
            G_InitNew (startskill, startepisode, startmap);
        else
            D_StartTitle ();

    }

    D_DoomLoop ();
}
