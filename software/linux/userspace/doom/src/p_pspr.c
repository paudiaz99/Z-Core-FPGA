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
//      Weapon sprite animation, weapon objects.
//      Action functions for weapons.
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: p_pspr.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";


#include "doomdef.h"
#include "d_event.h"


#include "m_random.h"
#include "p_local.h"
#include "s_sound.h"

#include "doomstat.h"

#include "sounds.h"

#include "p_pspr.h"

#define LOWERSPEED              FRACUNIT*6
#define RAISESPEED              FRACUNIT*6

#define WEAPONBOTTOM    128*FRACUNIT
#define WEAPONTOP               32*FRACUNIT


#define BFGCELLS                40


void
P_SetPsprite
( player_t*     player,
  int           position,
  statenum_t    stnum )
{
    pspdef_t*   psp;
    state_t*    state;

    psp = &player->psprites[position];

    do
    {
        if (!stnum)
        {
            psp->state = NULL;
            break;
        }

        state = &states[stnum];
        psp->state = state;
        psp->tics = state->tics;

        if (state->misc1)
        {
            psp->sx = state->misc1 << FRACBITS;
            psp->sy = state->misc2 << FRACBITS;
        }

        if (state->action.acp2)
        {
            state->action.acp2(player, psp);
            if (!psp->state)
                break;
        }

        stnum = psp->state->nextstate;

    } while (!psp->tics);
}



fixed_t         swingx;
fixed_t         swingy;

void P_CalcSwing (player_t*     player)
{
    fixed_t     swing;
    int         angle;


    swing = player->bob;

    angle = (FINEANGLES/70*leveltime)&FINEMASK;
    swingx = FixedMul ( swing, finesine[angle]);

    angle = (FINEANGLES/70*leveltime+FINEANGLES/2)&FINEMASK;
    swingy = -FixedMul ( swingx, finesine[angle]);
}



void P_BringUpWeapon (player_t* player)
{
    statenum_t  newstate;

    if (player->pendingweapon == wp_nochange)
        player->pendingweapon = player->readyweapon;

    if (player->pendingweapon == wp_chainsaw)
        S_StartSound (player->mo, sfx_sawup);

    newstate = weaponinfo[player->pendingweapon].upstate;

    player->pendingweapon = wp_nochange;
    player->psprites[ps_weapon].sy = WEAPONBOTTOM;

    P_SetPsprite (player, ps_weapon, newstate);
}

boolean P_CheckAmmo (player_t* player)
{
    ammotype_t          ammo;
    int                 count;

    ammo = weaponinfo[player->readyweapon].ammo;

    if (player->readyweapon == wp_bfg)
        count = BFGCELLS;
    else if (player->readyweapon == wp_supershotgun)
        count = 2;
    else
        count = 1;

    if (ammo == am_noammo || player->ammo[ammo] >= count)
        return true;

    do
    {
        if (player->weaponowned[wp_plasma]
            && player->ammo[am_cell]
            && (gamemode != shareware) )
        {
            player->pendingweapon = wp_plasma;
        }
        else if (player->weaponowned[wp_supershotgun]
                 && player->ammo[am_shell]>2
                 && (gamemode == commercial) )
        {
            player->pendingweapon = wp_supershotgun;
        }
        else if (player->weaponowned[wp_chaingun]
                 && player->ammo[am_clip])
        {
            player->pendingweapon = wp_chaingun;
        }
        else if (player->weaponowned[wp_shotgun]
                 && player->ammo[am_shell])
        {
            player->pendingweapon = wp_shotgun;
        }
        else if (player->ammo[am_clip])
        {
            player->pendingweapon = wp_pistol;
        }
        else if (player->weaponowned[wp_chainsaw])
        {
            player->pendingweapon = wp_chainsaw;
        }
        else if (player->weaponowned[wp_missile]
                 && player->ammo[am_misl])
        {
            player->pendingweapon = wp_missile;
        }
        else if (player->weaponowned[wp_bfg]
                 && player->ammo[am_cell]>40
                 && (gamemode != shareware) )
        {
            player->pendingweapon = wp_bfg;
        }
        else
        {
            player->pendingweapon = wp_fist;
        }

    } while (player->pendingweapon == wp_nochange);

    P_SetPsprite (player,
                  ps_weapon,
                  weaponinfo[player->readyweapon].downstate);

    return false;
}


void P_FireWeapon (player_t* player)
{
    statenum_t  newstate;

    if (!P_CheckAmmo (player))
        return;

    P_SetMobjState (player->mo, S_PLAY_ATK1);
    newstate = weaponinfo[player->readyweapon].atkstate;
    P_SetPsprite (player, ps_weapon, newstate);
    P_NoiseAlert (player->mo, player->mo);
}



void P_DropWeapon (player_t* player)
{
    P_SetPsprite (player,
                  ps_weapon,
                  weaponinfo[player->readyweapon].downstate);
}



void
A_WeaponReady
( player_t*     player,
  pspdef_t*     psp )
{
    statenum_t  newstate;
    int         angle;

    if (player->mo->state == &states[S_PLAY_ATK1]
        || player->mo->state == &states[S_PLAY_ATK2] )
    {
        P_SetMobjState (player->mo, S_PLAY);
    }

    if (player->readyweapon == wp_chainsaw
        && psp->state == &states[S_SAW])
    {
        S_StartSound (player->mo, sfx_sawidl);
    }

    if (player->pendingweapon != wp_nochange || !player->health)
    {
        newstate = weaponinfo[player->readyweapon].downstate;
        P_SetPsprite (player, ps_weapon, newstate);
        return;
    }

    if (player->cmd.buttons & BT_ATTACK)
    {
        if ( !player->attackdown
             || (player->readyweapon != wp_missile
                 && player->readyweapon != wp_bfg) )
        {
            player->attackdown = true;
            P_FireWeapon (player);
            return;
        }
    }
    else
        player->attackdown = false;

    angle = (128*leveltime)&FINEMASK;
    psp->sx = FRACUNIT + FixedMul (player->bob, finecosine[angle]);
    angle &= FINEANGLES/2-1;
    psp->sy = WEAPONTOP + FixedMul (player->bob, finesine[angle]);
}



void A_ReFire
( player_t*     player,
  pspdef_t*     psp )
{

    if ( (player->cmd.buttons & BT_ATTACK)
         && player->pendingweapon == wp_nochange
         && player->health)
    {
        player->refire++;
        P_FireWeapon (player);
    }
    else
    {
        player->refire = 0;
        P_CheckAmmo (player);
    }
}


void
A_CheckReload
( player_t*     player,
  pspdef_t*     psp )
{
    P_CheckAmmo (player);
#if 0
    if (player->ammo[am_shell]<2)
        P_SetPsprite (player, ps_weapon, S_DSNR1);
#endif
}



void
A_Lower
( player_t*     player,
  pspdef_t*     psp )
{
    psp->sy += LOWERSPEED;

    if (psp->sy < WEAPONBOTTOM )
        return;

    if (player->playerstate == PST_DEAD)
    {
        psp->sy = WEAPONBOTTOM;

        return;
    }

    if (!player->health)
    {
        P_SetPsprite (player,  ps_weapon, S_NULL);
        return;
    }

    player->readyweapon = player->pendingweapon;

    P_BringUpWeapon (player);
}


void
A_Raise
( player_t*     player,
  pspdef_t*     psp )
{
    statenum_t  newstate;

    psp->sy -= RAISESPEED;

    if (psp->sy > WEAPONTOP )
        return;

    psp->sy = WEAPONTOP;

    newstate = weaponinfo[player->readyweapon].readystate;

    P_SetPsprite (player, ps_weapon, newstate);
}



void
A_GunFlash
( player_t*     player,
  pspdef_t*     psp )
{
    P_SetMobjState (player->mo, S_PLAY_ATK2);
    P_SetPsprite (player,ps_flash,weaponinfo[player->readyweapon].flashstate);
}





void
A_Punch
( player_t*     player,
  pspdef_t*     psp )
{
    angle_t     angle;
    int         damage;
    int         slope;

    damage = (P_Random ()%10+1)<<1;

    if (player->powers[pw_strength])
        damage *= 10;

    angle = player->mo->angle;
    angle += (P_Random()-P_Random())<<18;
    slope = P_AimLineAttack (player->mo, angle, MELEERANGE);
    P_LineAttack (player->mo, angle, MELEERANGE, slope, damage);

    if (linetarget)
    {
        S_StartSound (player->mo, sfx_punch);
        player->mo->angle = R_PointToAngle2 (player->mo->x,
                                             player->mo->y,
                                             linetarget->x,
                                             linetarget->y);
    }
}


void
A_Saw
( player_t*     player,
  pspdef_t*     psp )
{
    angle_t     angle;
    int         damage;
    int         slope;

    damage = 2*(P_Random ()%10+1);
    angle = player->mo->angle;
    angle += (P_Random()-P_Random())<<18;

    slope = P_AimLineAttack (player->mo, angle, MELEERANGE+1);
    P_LineAttack (player->mo, angle, MELEERANGE+1, slope, damage);

    if (!linetarget)
    {
        S_StartSound (player->mo, sfx_sawful);
        return;
    }
    S_StartSound (player->mo, sfx_sawhit);

    angle = R_PointToAngle2 (player->mo->x, player->mo->y,
                             linetarget->x, linetarget->y);
    if (angle - player->mo->angle > ANG180)
    {
        if (angle - player->mo->angle < -ANG90/20)
            player->mo->angle = angle + ANG90/21;
        else
            player->mo->angle -= ANG90/20;
    }
    else
    {
        if (angle - player->mo->angle > ANG90/20)
            player->mo->angle = angle - ANG90/21;
        else
            player->mo->angle += ANG90/20;
    }
    player->mo->flags |= MF_JUSTATTACKED;
}



void
A_FireMissile
( player_t*     player,
  pspdef_t*     psp )
{
    player->ammo[weaponinfo[player->readyweapon].ammo]--;
    P_SpawnPlayerMissile (player->mo, MT_ROCKET);
}


void
A_FireBFG
( player_t*     player,
  pspdef_t*     psp )
{
    player->ammo[weaponinfo[player->readyweapon].ammo] -= BFGCELLS;
    P_SpawnPlayerMissile (player->mo, MT_BFG);
}



void
A_FirePlasma
( player_t*     player,
  pspdef_t*     psp )
{
    player->ammo[weaponinfo[player->readyweapon].ammo]--;

    P_SetPsprite (player,
                  ps_flash,
                  weaponinfo[player->readyweapon].flashstate+(P_Random ()&1) );

    P_SpawnPlayerMissile (player->mo, MT_PLASMA);
}



fixed_t         bulletslope;


void P_BulletSlope (mobj_t*     mo)
{
    angle_t     an;

    an = mo->angle;
    bulletslope = P_AimLineAttack (mo, an, 16*64*FRACUNIT);

    if (!linetarget)
    {
        an += 1<<26;
        bulletslope = P_AimLineAttack (mo, an, 16*64*FRACUNIT);
        if (!linetarget)
        {
            an -= 2<<26;
            bulletslope = P_AimLineAttack (mo, an, 16*64*FRACUNIT);
        }
    }
}


void
P_GunShot
( mobj_t*       mo,
  boolean       accurate )
{
    angle_t     angle;
    int         damage;

    damage = 5*(P_Random ()%3+1);
    angle = mo->angle;

    if (!accurate)
        angle += (P_Random()-P_Random())<<18;

    P_LineAttack (mo, angle, MISSILERANGE, bulletslope, damage);
}


void
A_FirePistol
( player_t*     player,
  pspdef_t*     psp )
{
    S_StartSound (player->mo, sfx_pistol);

    P_SetMobjState (player->mo, S_PLAY_ATK2);
    player->ammo[weaponinfo[player->readyweapon].ammo]--;

    P_SetPsprite (player,
                  ps_flash,
                  weaponinfo[player->readyweapon].flashstate);

    P_BulletSlope (player->mo);
    P_GunShot (player->mo, !player->refire);
}


void
A_FireShotgun
( player_t*     player,
  pspdef_t*     psp )
{
    int         i;

    S_StartSound (player->mo, sfx_shotgn);
    P_SetMobjState (player->mo, S_PLAY_ATK2);

    player->ammo[weaponinfo[player->readyweapon].ammo]--;

    P_SetPsprite (player,
                  ps_flash,
                  weaponinfo[player->readyweapon].flashstate);

    P_BulletSlope (player->mo);

    for (i=0 ; i<7 ; i++)
        P_GunShot (player->mo, false);
}



void
A_FireShotgun2
( player_t*     player,
  pspdef_t*     psp )
{
    int         i;
    angle_t     angle;
    int         damage;


    S_StartSound (player->mo, sfx_dshtgn);
    P_SetMobjState (player->mo, S_PLAY_ATK2);

    player->ammo[weaponinfo[player->readyweapon].ammo]-=2;

    P_SetPsprite (player,
                  ps_flash,
                  weaponinfo[player->readyweapon].flashstate);

    P_BulletSlope (player->mo);

    for (i=0 ; i<20 ; i++)
    {
        damage = 5*(P_Random ()%3+1);
        angle = player->mo->angle;
        angle += (P_Random()-P_Random())<<19;
        P_LineAttack (player->mo,
                      angle,
                      MISSILERANGE,
                      bulletslope + ((P_Random()-P_Random())<<5), damage);
    }
}


void
A_FireCGun
( player_t*     player,
  pspdef_t*     psp )
{
    S_StartSound (player->mo, sfx_pistol);

    if (!player->ammo[weaponinfo[player->readyweapon].ammo])
        return;

    P_SetMobjState (player->mo, S_PLAY_ATK2);
    player->ammo[weaponinfo[player->readyweapon].ammo]--;

    P_SetPsprite (player,
                  ps_flash,
                  weaponinfo[player->readyweapon].flashstate
                  + psp->state
                  - &states[S_CHAIN1] );

    P_BulletSlope (player->mo);

    P_GunShot (player->mo, !player->refire);
}



void A_Light0 (player_t *player, pspdef_t *psp)
{
    player->extralight = 0;
}

void A_Light1 (player_t *player, pspdef_t *psp)
{
    player->extralight = 1;
}

void A_Light2 (player_t *player, pspdef_t *psp)
{
    player->extralight = 2;
}


void A_BFGSpray (mobj_t* mo)
{
    int                 i;
    int                 j;
    int                 damage;
    angle_t             an;

    for (i=0 ; i<40 ; i++)
    {
        an = mo->angle - ANG90/2 + ANG90/40*i;

        P_AimLineAttack (mo->target, an, 16*64*FRACUNIT);

        if (!linetarget)
            continue;

        P_SpawnMobj (linetarget->x,
                     linetarget->y,
                     linetarget->z + (linetarget->height>>2),
                     MT_EXTRABFG);

        damage = 0;
        for (j=0;j<15;j++)
            damage += (P_Random()&7) + 1;

        P_DamageMobj (linetarget, mo->target,mo->target, damage);
    }
}


void
A_BFGsound
( player_t*     player,
  pspdef_t*     psp )
{
    S_StartSound (player->mo, sfx_bfg);
}



void P_SetupPsprites (player_t* player)
{
    int i;

    for (i=0 ; i<NUMPSPRITES ; i++)
        player->psprites[i].state = NULL;

    player->pendingweapon = player->readyweapon;
    P_BringUpWeapon (player);
}




void P_MovePsprites (player_t* player)
{
    int         i;
    pspdef_t*   psp;
    state_t*    state;

    psp = &player->psprites[0];
    for (i=0 ; i<NUMPSPRITES ; i++, psp++)
    {
        if ( (state = psp->state) )
        {

            if (psp->tics != -1)
            {
                psp->tics--;
                if (!psp->tics)
                    P_SetPsprite (player, i, psp->state->nextstate);
            }
        }
    }

    player->psprites[ps_flash].sx = player->psprites[ps_weapon].sx;
    player->psprites[ps_flash].sy = player->psprites[ps_weapon].sy;
}


