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
//      DOOM Network game communication and protocol,
//      all OS independend parts.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: d_net.c,v 1.3 1997/02/03 22:01:47 b1 Exp $";


#include "m_menu.h"
#include "i_system.h"
#include "i_video.h"
#include "i_net.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"

#define NCMD_EXIT               0x80000000
#define NCMD_RETRANSMIT         0x40000000
#define NCMD_SETUP              0x20000000
#define NCMD_KILL               0x10000000
#define NCMD_CHECKSUM           0x0fffffff


doomcom_t*      doomcom;
doomdata_t*     netbuffer;


#define RESENDCOUNT     10
#define PL_DRONE        0x80

ticcmd_t        localcmds[BACKUPTICS];

ticcmd_t        netcmds[MAXPLAYERS][BACKUPTICS];
int             nettics[MAXNETNODES];
boolean         nodeingame[MAXNETNODES];
boolean         remoteresend[MAXNETNODES];
int             resendto[MAXNETNODES];
int             resendcount[MAXNETNODES];

int             nodeforplayer[MAXPLAYERS];

int             maketic;
int             lastnettic;
int             skiptics;
int             ticdup;
int             maxsend;


void D_ProcessEvents (void);
void G_BuildTiccmd (ticcmd_t *cmd);
void D_DoAdvanceDemo (void);

boolean         reboundpacket;
doomdata_t      reboundstore;



int NetbufferSize (void)
{
    return (int)&(((doomdata_t *)0)->cmds[netbuffer->numtics]);
}

unsigned NetbufferChecksum (void)
{
    unsigned            c;
    int         i,l;

    c = 0x1234567;

#ifdef NORMALUNIX
    return 0;
#endif

    l = (NetbufferSize () - (int)&(((doomdata_t *)0)->retransmitfrom))/4;
    for (i=0 ; i<l ; i++)
        c += ((unsigned *)&netbuffer->retransmitfrom)[i] * (i+1);

    return c & NCMD_CHECKSUM;
}

int ExpandTics (int low)
{
    int delta;

    delta = low - (maketic&0xff);

    if (delta >= -64 && delta <= 64)
        return (maketic&~0xff) + low;
    if (delta > 64)
        return (maketic&~0xff) - 256 + low;
    if (delta < -64)
        return (maketic&~0xff) + 256 + low;

    I_Error ("ExpandTics: strange value %i at maketic %i",low,maketic);
    return 0;
}



void
HSendPacket
 (int   node,
  int   flags )
{
    netbuffer->checksum = NetbufferChecksum () | flags;

    if (!node)
    {
        reboundstore = *netbuffer;
        reboundpacket = true;
        return;
    }

    if (demoplayback)
        return;

    if (!netgame)
        I_Error ("Tried to transmit to another node");

    doomcom->command = CMD_SEND;
    doomcom->remotenode = node;
    doomcom->datalength = NetbufferSize ();

    if (debugfile)
    {
        int             i;
        int             realretrans;
        if (netbuffer->checksum & NCMD_RETRANSMIT)
            realretrans = ExpandTics (netbuffer->retransmitfrom);
        else
            realretrans = -1;

        fprintf (debugfile,"send (%i + %i, R %i) [%i] ",
                 ExpandTics(netbuffer->starttic),
                 netbuffer->numtics, realretrans, doomcom->datalength);

        for (i=0 ; i<doomcom->datalength ; i++)
            fprintf (debugfile,"%i ",((byte *)netbuffer)[i]);

        fprintf (debugfile,"\n");
    }

    I_NetCmd ();
}

boolean HGetPacket (void)
{
    if (reboundpacket)
    {
        *netbuffer = reboundstore;
        doomcom->remotenode = 0;
        reboundpacket = false;
        return true;
    }

    if (!netgame)
        return false;

    if (demoplayback)
        return false;

    doomcom->command = CMD_GET;
    I_NetCmd ();

    if (doomcom->remotenode == -1)
        return false;

    if (doomcom->datalength != NetbufferSize ())
    {
        if (debugfile)
            fprintf (debugfile,"bad packet length %i\n",doomcom->datalength);
        return false;
    }

    if (NetbufferChecksum () != (netbuffer->checksum&NCMD_CHECKSUM) )
    {
        if (debugfile)
            fprintf (debugfile,"bad packet checksum\n");
        return false;
    }

    if (debugfile)
    {
        int             realretrans;
        int     i;

        if (netbuffer->checksum & NCMD_SETUP)
            fprintf (debugfile,"setup packet\n");
        else
        {
            if (netbuffer->checksum & NCMD_RETRANSMIT)
                realretrans = ExpandTics (netbuffer->retransmitfrom);
            else
                realretrans = -1;

            fprintf (debugfile,"get %i = (%i + %i, R %i)[%i] ",
                     doomcom->remotenode,
                     ExpandTics(netbuffer->starttic),
                     netbuffer->numtics, realretrans, doomcom->datalength);

            for (i=0 ; i<doomcom->datalength ; i++)
                fprintf (debugfile,"%i ",((byte *)netbuffer)[i]);
            fprintf (debugfile,"\n");
        }
    }
    return true;
}


char    exitmsg[80];

void GetPackets (void)
{
    int         netconsole;
    int         netnode;
    ticcmd_t    *src, *dest;
    int         realend;
    int         realstart;

    while ( HGetPacket() )
    {
        if (netbuffer->checksum & NCMD_SETUP)
            continue;

        netconsole = netbuffer->player & ~PL_DRONE;
        netnode = doomcom->remotenode;

        realstart = ExpandTics (netbuffer->starttic);
        realend = (realstart+netbuffer->numtics);

        if (netbuffer->checksum & NCMD_EXIT)
        {
            if (!nodeingame[netnode])
                continue;
            nodeingame[netnode] = false;
            playeringame[netconsole] = false;
            strcpy (exitmsg, "Player 1 left the game");
            exitmsg[7] += netconsole;
            players[consoleplayer].message = exitmsg;
            if (demorecording)
                G_CheckDemoStatus ();
            continue;
        }

        if (netbuffer->checksum & NCMD_KILL)
            I_Error ("Killed by network driver");

        nodeforplayer[netconsole] = netnode;

        if ( resendcount[netnode] <= 0
             && (netbuffer->checksum & NCMD_RETRANSMIT) )
        {
            resendto[netnode] = ExpandTics(netbuffer->retransmitfrom);
            if (debugfile)
                fprintf (debugfile,"retransmit from %i\n", resendto[netnode]);
            resendcount[netnode] = RESENDCOUNT;
        }
        else
            resendcount[netnode]--;

        if (realend == nettics[netnode])
            continue;

        if (realend < nettics[netnode])
        {
            if (debugfile)
                fprintf (debugfile,
                         "out of order packet (%i + %i)\n" ,
                         realstart,netbuffer->numtics);
            continue;
        }

        if (realstart > nettics[netnode])
        {
            if (debugfile)
                fprintf (debugfile,
                         "missed tics from %i (%i - %i)\n",
                         netnode, realstart, nettics[netnode]);
            remoteresend[netnode] = true;
            continue;
        }

        {
            int         start;

            remoteresend[netnode] = false;

            start = nettics[netnode] - realstart;
            src = &netbuffer->cmds[start];

            while (nettics[netnode] < realend)
            {
                dest = &netcmds[netconsole][nettics[netnode]%BACKUPTICS];
                nettics[netnode]++;
                *dest = *src;
                src++;
            }
        }
    }
}


int      gametime;

void NetUpdate (void)
{
    int             nowtime;
    int             newtics;
    int                         i,j;
    int                         realstart;
    int                         gameticdiv;

    nowtime = I_GetTime ()/ticdup;
    newtics = nowtime - gametime;
    gametime = nowtime;

    if (newtics <= 0)
        goto listen;

    if (skiptics <= newtics)
    {
        newtics -= skiptics;
        skiptics = 0;
    }
    else
    {
        skiptics -= newtics;
        newtics = 0;
    }


    netbuffer->player = consoleplayer;

    gameticdiv = gametic/ticdup;
    for (i=0 ; i<newtics ; i++)
    {
        I_StartTic ();
        D_ProcessEvents ();
        if (maketic - gameticdiv >= BACKUPTICS/2-1)
            break;

        G_BuildTiccmd (&localcmds[maketic%BACKUPTICS]);
        maketic++;
    }


    if (singletics)
        return;

    for (i=0 ; i<doomcom->numnodes ; i++)
        if (nodeingame[i])
        {
            netbuffer->starttic = realstart = resendto[i];
            netbuffer->numtics = maketic - realstart;
            if (netbuffer->numtics > BACKUPTICS)
                I_Error ("NetUpdate: netbuffer->numtics > BACKUPTICS");

            resendto[i] = maketic - doomcom->extratics;

            for (j=0 ; j< netbuffer->numtics ; j++)
                netbuffer->cmds[j] =
                    localcmds[(realstart+j)%BACKUPTICS];

            if (remoteresend[i])
            {
                netbuffer->retransmitfrom = nettics[i];
                HSendPacket (i, NCMD_RETRANSMIT);
            }
            else
            {
                netbuffer->retransmitfrom = 0;
                HSendPacket (i, 0);
            }
        }

  listen:
    GetPackets ();
}



void CheckAbort (void)
{
    event_t *ev;
    int         stoptic;

    stoptic = I_GetTime () + 2;
    while (I_GetTime() < stoptic)
        I_StartTic ();

    I_StartTic ();
    for ( ; eventtail != eventhead
              ; eventtail = (eventtail+1)&(MAXEVENTS-1) )
    {
        ev = &events[eventtail];
        if (ev->type == ev_keydown && ev->data1 == KEY_ESCAPE)
            I_Error ("Network game synchronization aborted.");
    }
}


void D_ArbitrateNetStart (void)
{
    int         i;
    boolean     gotinfo[MAXNETNODES];

    autostart = true;
    memset (gotinfo,0,sizeof(gotinfo));

    if (doomcom->consoleplayer)
    {
        printf ("listening for network start info...\n");
        while (1)
        {
            CheckAbort ();
            if (!HGetPacket ())
                continue;
            if (netbuffer->checksum & NCMD_SETUP)
            {
                if (netbuffer->player != VERSION)
                    I_Error ("Different DOOM versions cannot play a net game!");
                startskill = netbuffer->retransmitfrom & 15;
                deathmatch = (netbuffer->retransmitfrom & 0xc0) >> 6;
                nomonsters = (netbuffer->retransmitfrom & 0x20) > 0;
                respawnparm = (netbuffer->retransmitfrom & 0x10) > 0;
                startmap = netbuffer->starttic & 0x3f;
                startepisode = netbuffer->starttic >> 6;
                return;
            }
        }
    }
    else
    {
        printf ("sending network start info...\n");
        do
        {
            CheckAbort ();
            for (i=0 ; i<doomcom->numnodes ; i++)
            {
                netbuffer->retransmitfrom = startskill;
                if (deathmatch)
                    netbuffer->retransmitfrom |= (deathmatch<<6);
                if (nomonsters)
                    netbuffer->retransmitfrom |= 0x20;
                if (respawnparm)
                    netbuffer->retransmitfrom |= 0x10;
                netbuffer->starttic = startepisode * 64 + startmap;
                netbuffer->player = VERSION;
                netbuffer->numtics = 0;
                HSendPacket (i, NCMD_SETUP);
            }

#if 1
            for(i = 10 ; i  &&  HGetPacket(); --i)
            {
                if((netbuffer->player&0x7f) < MAXNETNODES)
                    gotinfo[netbuffer->player&0x7f] = true;
            }
#else
            while (HGetPacket ())
            {
                gotinfo[netbuffer->player&0x7f] = true;
            }
#endif

            for (i=1 ; i<doomcom->numnodes ; i++)
                if (!gotinfo[i])
                    break;
        } while (i < doomcom->numnodes);
    }
}

extern  int                     viewangleoffset;

void D_CheckNetGame (void)
{
    int             i;

    for (i=0 ; i<MAXNETNODES ; i++)
    {
        nodeingame[i] = false;
        nettics[i] = 0;
        remoteresend[i] = false;
        resendto[i] = 0;
    }

    I_InitNetwork ();
    if (doomcom->id != DOOMCOM_ID)
        I_Error ("Doomcom buffer invalid!");

    netbuffer = &doomcom->data;
    consoleplayer = displayplayer = doomcom->consoleplayer;
    if (netgame)
        D_ArbitrateNetStart ();

    printf ("startskill %i  deathmatch: %i  startmap: %i  startepisode: %i\n",
            startskill, deathmatch, startmap, startepisode);

    ticdup = doomcom->ticdup;
    maxsend = BACKUPTICS/(2*ticdup)-1;
    if (maxsend<1)
        maxsend = 1;

    for (i=0 ; i<doomcom->numplayers ; i++)
        playeringame[i] = true;
    for (i=0 ; i<doomcom->numnodes ; i++)
        nodeingame[i] = true;

    printf ("player %i of %i (%i nodes)\n",
            consoleplayer+1, doomcom->numplayers, doomcom->numnodes);

}


void D_QuitNetGame (void)
{
    int             i, j;

    if (debugfile)
        fclose (debugfile);

    if (!netgame || !usergame || consoleplayer == -1 || demoplayback)
        return;

    netbuffer->player = consoleplayer;
    netbuffer->numtics = 0;
    for (i=0 ; i<4 ; i++)
    {
        for (j=1 ; j<doomcom->numnodes ; j++)
            if (nodeingame[j])
                HSendPacket (j, NCMD_EXIT);
        I_WaitVBL (1);
    }
}



int     frametics[4];
int     frameon;
int     frameskip[4];
int     oldnettics;

extern  boolean advancedemo;

void TryRunTics (void)
{
    int         i;
    int         lowtic;
    int         entertic;
    static int  oldentertics;
    int         realtics;
    int         availabletics;
    int         counts;
    int         numplaying;

    entertic = I_GetTime ()/ticdup;
    realtics = entertic - oldentertics;
    oldentertics = entertic;

    NetUpdate ();

    lowtic = MAXINT;
    numplaying = 0;
    for (i=0 ; i<doomcom->numnodes ; i++)
    {
        if (nodeingame[i])
        {
            numplaying++;
            if (nettics[i] < lowtic)
                lowtic = nettics[i];
        }
    }
    availabletics = lowtic - gametic/ticdup;

    if (realtics < availabletics-1)
        counts = realtics+1;
    else if (realtics < availabletics)
        counts = realtics;
    else
        counts = availabletics;

    if (counts < 1)
        counts = 1;

    frameon++;

    if (debugfile)
        fprintf (debugfile,
                 "=======real: %i  avail: %i  game: %i\n",
                 realtics, availabletics,counts);

    if (!demoplayback)
    {
        for (i=0 ; i<MAXPLAYERS ; i++)
            if (playeringame[i])
                break;
        if (consoleplayer == i)
        {
        }
        else
        {
            if (nettics[0] <= nettics[nodeforplayer[i]])
            {
                gametime--;
            }
            frameskip[frameon&3] = (oldnettics > nettics[nodeforplayer[i]]);
            oldnettics = nettics[0];
            if (frameskip[0] && frameskip[1] && frameskip[2] && frameskip[3])
            {
                skiptics = 1;
            }
        }
    }

    while (lowtic < gametic/ticdup + counts)
    {
        NetUpdate ();
        lowtic = MAXINT;

        for (i=0 ; i<doomcom->numnodes ; i++)
            if (nodeingame[i] && nettics[i] < lowtic)
                lowtic = nettics[i];

        if (lowtic < gametic/ticdup)
            I_Error ("TryRunTics: lowtic < gametic");

        if (I_GetTime ()/ticdup - entertic >= 20)
        {
            M_Ticker ();
            return;
        }
    }

    while (counts--)
    {
        for (i=0 ; i<ticdup ; i++)
        {
            if (gametic/ticdup > lowtic)
                I_Error ("gametic>lowtic");
            if (advancedemo)
                D_DoAdvanceDemo ();
            M_Ticker ();
            G_Ticker ();
            gametic++;

            if (i != ticdup-1)
            {
                ticcmd_t        *cmd;
                int                     buf;
                int                     j;

                buf = (gametic/ticdup)%BACKUPTICS;
                for (j=0 ; j<MAXPLAYERS ; j++)
                {
                    cmd = &netcmds[j][buf];
                    cmd->chatchar = 0;
                    if (cmd->buttons & BT_SPECIAL)
                        cmd->buttons = 0;
                }
            }
        }
        NetUpdate ();
    }
}
