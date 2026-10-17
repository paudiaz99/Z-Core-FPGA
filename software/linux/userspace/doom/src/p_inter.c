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
//      Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------

static const char __attribute__((unused))
rcsid[] = "$Id: p_inter.c,v 1.4 1997/02/03 22:45:11 b1 Exp $";


#include "doomdef.h"
#include "dstrings.h"
#include "sounds.h"

#include "doomstat.h"

#include "m_random.h"
#include "i_system.h"

#include "am_map.h"

#include "p_local.h"

#include "s_sound.h"

#ifdef __GNUG__
#pragma implementation "p_inter.h"
#endif
#include "p_inter.h"


#define BONUSADD        6




int     maxammo[NUMAMMO] = {200, 50, 300, 50};
int     clipammo[NUMAMMO] = {10, 4, 20, 1};




boolean
P_GiveAmmo
( player_t*     player,
  ammotype_t    ammo,
  int           num )
{
    int         oldammo;

    if (ammo == am_noammo)
        return false;

    if (ammo < 0 || ammo > NUMAMMO)
        I_Error ("P_GiveAmmo: bad type %i", ammo);

    if ( player->ammo[ammo] == player->maxammo[ammo]  )
        return false;

    if (num)
        num *= clipammo[ammo];
    else
        num = clipammo[ammo]/2;

    if (gameskill == sk_baby
        || gameskill == sk_nightmare)
    {
        num <<= 1;
    }


    oldammo = player->ammo[ammo];
    player->ammo[ammo] += num;

    if (player->ammo[ammo] > player->maxammo[ammo])
        player->ammo[ammo] = player->maxammo[ammo];

    if (oldammo)
        return true;

    switch (ammo)
    {
      case am_clip:
        if (player->readyweapon == wp_fist)
        {
            if (player->weaponowned[wp_chaingun])
                player->pendingweapon = wp_chaingun;
            else
                player->pendingweapon = wp_pistol;
        }
        break;

      case am_shell:
        if (player->readyweapon == wp_fist
            || player->readyweapon == wp_pistol)
        {
            if (player->weaponowned[wp_shotgun])
                player->pendingweapon = wp_shotgun;
        }
        break;

      case am_cell:
        if (player->readyweapon == wp_fist
            || player->readyweapon == wp_pistol)
        {
            if (player->weaponowned[wp_plasma])
                player->pendingweapon = wp_plasma;
        }
        break;

      case am_misl:
        if (player->readyweapon == wp_fist)
        {
            if (player->weaponowned[wp_missile])
                player->pendingweapon = wp_missile;
        }
      default:
        break;
    }

    return true;
}


boolean
P_GiveWeapon
( player_t*     player,
  weapontype_t  weapon,
  boolean       dropped )
{
    boolean     gaveammo;
    boolean     gaveweapon;

    if (netgame
        && (deathmatch!=2)
         && !dropped )
    {
        if (player->weaponowned[weapon])
            return false;

        player->bonuscount += BONUSADD;
        player->weaponowned[weapon] = true;

        if (deathmatch)
            P_GiveAmmo (player, weaponinfo[weapon].ammo, 5);
        else
            P_GiveAmmo (player, weaponinfo[weapon].ammo, 2);
        player->pendingweapon = weapon;

        if (player == &players[consoleplayer])
            S_StartSound (NULL, sfx_wpnup);
        return false;
    }

    if (weaponinfo[weapon].ammo != am_noammo)
    {
        if (dropped)
            gaveammo = P_GiveAmmo (player, weaponinfo[weapon].ammo, 1);
        else
            gaveammo = P_GiveAmmo (player, weaponinfo[weapon].ammo, 2);
    }
    else
        gaveammo = false;

    if (player->weaponowned[weapon])
        gaveweapon = false;
    else
    {
        gaveweapon = true;
        player->weaponowned[weapon] = true;
        player->pendingweapon = weapon;
    }

    return (gaveweapon || gaveammo);
}



boolean
P_GiveBody
( player_t*     player,
  int           num )
{
    if (player->health >= MAXHEALTH)
        return false;

    player->health += num;
    if (player->health > MAXHEALTH)
        player->health = MAXHEALTH;
    player->mo->health = player->health;

    return true;
}



boolean
P_GiveArmor
( player_t*     player,
  int           armortype )
{
    int         hits;

    hits = armortype*100;
    if (player->armorpoints >= hits)
        return false;

    player->armortype = armortype;
    player->armorpoints = hits;

    return true;
}



void
P_GiveCard
( player_t*     player,
  card_t        card )
{
    if (player->cards[card])
        return;

    player->bonuscount = BONUSADD;
    player->cards[card] = 1;
}


boolean
P_GivePower
( player_t*     player,
  int     power )
{
    if (power == pw_invulnerability)
    {
        player->powers[power] = INVULNTICS;
        return true;
    }

    if (power == pw_invisibility)
    {
        player->powers[power] = INVISTICS;
        player->mo->flags |= MF_SHADOW;
        return true;
    }

    if (power == pw_infrared)
    {
        player->powers[power] = INFRATICS;
        return true;
    }

    if (power == pw_ironfeet)
    {
        player->powers[power] = IRONTICS;
        return true;
    }

    if (power == pw_strength)
    {
        P_GiveBody (player, 100);
        player->powers[power] = 1;
        return true;
    }

    if (player->powers[power])
        return false;

    player->powers[power] = 1;
    return true;
}



void
P_TouchSpecialThing
( mobj_t*       special,
  mobj_t*       toucher )
{
    player_t*   player;
    int         i;
    fixed_t     delta;
    int         sound;

    delta = special->z - toucher->z;

    if (delta > toucher->height
        || delta < -8*FRACUNIT)
    {
        return;
    }


    sound = sfx_itemup;
    player = toucher->player;

    if (toucher->health <= 0)
        return;

    switch (special->sprite)
    {
      case SPR_ARM1:
        if (!P_GiveArmor (player, 1))
            return;
        player->message = GOTARMOR;
        break;

      case SPR_ARM2:
        if (!P_GiveArmor (player, 2))
            return;
        player->message = GOTMEGA;
        break;

      case SPR_BON1:
        player->health++;
        if (player->health > 200)
            player->health = 200;
        player->mo->health = player->health;
        player->message = GOTHTHBONUS;
        break;

      case SPR_BON2:
        player->armorpoints++;
        if (player->armorpoints > 200)
            player->armorpoints = 200;
        if (!player->armortype)
            player->armortype = 1;
        player->message = GOTARMBONUS;
        break;

      case SPR_SOUL:
        player->health += 100;
        if (player->health > 200)
            player->health = 200;
        player->mo->health = player->health;
        player->message = GOTSUPER;
        sound = sfx_getpow;
        break;

      case SPR_MEGA:
        if (gamemode != commercial)
            return;
        player->health = 200;
        player->mo->health = player->health;
        P_GiveArmor (player,2);
        player->message = GOTMSPHERE;
        sound = sfx_getpow;
        break;

      case SPR_BKEY:
        if (!player->cards[it_bluecard])
            player->message = GOTBLUECARD;
        P_GiveCard (player, it_bluecard);
        if (!netgame)
            break;
        return;

      case SPR_YKEY:
        if (!player->cards[it_yellowcard])
            player->message = GOTYELWCARD;
        P_GiveCard (player, it_yellowcard);
        if (!netgame)
            break;
        return;

      case SPR_RKEY:
        if (!player->cards[it_redcard])
            player->message = GOTREDCARD;
        P_GiveCard (player, it_redcard);
        if (!netgame)
            break;
        return;

      case SPR_BSKU:
        if (!player->cards[it_blueskull])
            player->message = GOTBLUESKUL;
        P_GiveCard (player, it_blueskull);
        if (!netgame)
            break;
        return;

      case SPR_YSKU:
        if (!player->cards[it_yellowskull])
            player->message = GOTYELWSKUL;
        P_GiveCard (player, it_yellowskull);
        if (!netgame)
            break;
        return;

      case SPR_RSKU:
        if (!player->cards[it_redskull])
            player->message = GOTREDSKULL;
        P_GiveCard (player, it_redskull);
        if (!netgame)
            break;
        return;

      case SPR_STIM:
        if (!P_GiveBody (player, 10))
            return;
        player->message = GOTSTIM;
        break;

      case SPR_MEDI:
        if (!P_GiveBody (player, 25))
            return;

        if (player->health < 25)
            player->message = GOTMEDINEED;
        else
            player->message = GOTMEDIKIT;
        break;


      case SPR_PINV:
        if (!P_GivePower (player, pw_invulnerability))
            return;
        player->message = GOTINVUL;
        sound = sfx_getpow;
        break;

      case SPR_PSTR:
        if (!P_GivePower (player, pw_strength))
            return;
        player->message = GOTBERSERK;
        if (player->readyweapon != wp_fist)
            player->pendingweapon = wp_fist;
        sound = sfx_getpow;
        break;

      case SPR_PINS:
        if (!P_GivePower (player, pw_invisibility))
            return;
        player->message = GOTINVIS;
        sound = sfx_getpow;
        break;

      case SPR_SUIT:
        if (!P_GivePower (player, pw_ironfeet))
            return;
        player->message = GOTSUIT;
        sound = sfx_getpow;
        break;

      case SPR_PMAP:
        if (!P_GivePower (player, pw_allmap))
            return;
        player->message = GOTMAP;
        sound = sfx_getpow;
        break;

      case SPR_PVIS:
        if (!P_GivePower (player, pw_infrared))
            return;
        player->message = GOTVISOR;
        sound = sfx_getpow;
        break;

      case SPR_CLIP:
        if (special->flags & MF_DROPPED)
        {
            if (!P_GiveAmmo (player,am_clip,0))
                return;
        }
        else
        {
            if (!P_GiveAmmo (player,am_clip,1))
                return;
        }
        player->message = GOTCLIP;
        break;

      case SPR_AMMO:
        if (!P_GiveAmmo (player, am_clip,5))
            return;
        player->message = GOTCLIPBOX;
        break;

      case SPR_ROCK:
        if (!P_GiveAmmo (player, am_misl,1))
            return;
        player->message = GOTROCKET;
        break;

      case SPR_BROK:
        if (!P_GiveAmmo (player, am_misl,5))
            return;
        player->message = GOTROCKBOX;
        break;

      case SPR_CELL:
        if (!P_GiveAmmo (player, am_cell,1))
            return;
        player->message = GOTCELL;
        break;

      case SPR_CELP:
        if (!P_GiveAmmo (player, am_cell,5))
            return;
        player->message = GOTCELLBOX;
        break;

      case SPR_SHEL:
        if (!P_GiveAmmo (player, am_shell,1))
            return;
        player->message = GOTSHELLS;
        break;

      case SPR_SBOX:
        if (!P_GiveAmmo (player, am_shell,5))
            return;
        player->message = GOTSHELLBOX;
        break;

      case SPR_BPAK:
        if (!player->backpack)
        {
            for (i=0 ; i<NUMAMMO ; i++)
                player->maxammo[i] *= 2;
            player->backpack = true;
        }
        for (i=0 ; i<NUMAMMO ; i++)
            P_GiveAmmo (player, i, 1);
        player->message = GOTBACKPACK;
        break;

      case SPR_BFUG:
        if (!P_GiveWeapon (player, wp_bfg, false) )
            return;
        player->message = GOTBFG9000;
        sound = sfx_wpnup;
        break;

      case SPR_MGUN:
        if (!P_GiveWeapon (player, wp_chaingun, special->flags&MF_DROPPED) )
            return;
        player->message = GOTCHAINGUN;
        sound = sfx_wpnup;
        break;

      case SPR_CSAW:
        if (!P_GiveWeapon (player, wp_chainsaw, false) )
            return;
        player->message = GOTCHAINSAW;
        sound = sfx_wpnup;
        break;

      case SPR_LAUN:
        if (!P_GiveWeapon (player, wp_missile, false) )
            return;
        player->message = GOTLAUNCHER;
        sound = sfx_wpnup;
        break;

      case SPR_PLAS:
        if (!P_GiveWeapon (player, wp_plasma, false) )
            return;
        player->message = GOTPLASMA;
        sound = sfx_wpnup;
        break;

      case SPR_SHOT:
        if (!P_GiveWeapon (player, wp_shotgun, special->flags&MF_DROPPED ) )
            return;
        player->message = GOTSHOTGUN;
        sound = sfx_wpnup;
        break;

      case SPR_SGN2:
        if (!P_GiveWeapon (player, wp_supershotgun, special->flags&MF_DROPPED ) )
            return;
        player->message = GOTSHOTGUN2;
        sound = sfx_wpnup;
        break;

      default:
        I_Error ("P_SpecialThing: Unknown gettable thing");
    }

    if (special->flags & MF_COUNTITEM)
        player->itemcount++;
    P_RemoveMobj (special);
    player->bonuscount += BONUSADD;
    if (player == &players[consoleplayer])
        S_StartSound (NULL, sound);
}


void
P_KillMobj
( mobj_t*       source,
  mobj_t*       target )
{
    mobjtype_t  item;
    mobj_t*     mo;

    target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);

    if (target->type != MT_SKULL)
        target->flags &= ~MF_NOGRAVITY;

    target->flags |= MF_CORPSE|MF_DROPOFF;
    target->height >>= 2;

    if (source && source->player)
    {
        if (target->flags & MF_COUNTKILL)
            source->player->killcount++;

        if (target->player)
            source->player->frags[target->player-players]++;
    }
    else if (!netgame && (target->flags & MF_COUNTKILL) )
    {
        players[0].killcount++;
    }

    if (target->player)
    {
        if (!source)
            target->player->frags[target->player-players]++;

        target->flags &= ~MF_SOLID;
        target->player->playerstate = PST_DEAD;
        P_DropWeapon (target->player);

        if (target->player == &players[consoleplayer]
            && automapactive)
        {
            AM_Stop ();
        }

    }

    if (target->health < -target->info->spawnhealth
        && target->info->xdeathstate)
    {
        P_SetMobjState (target, target->info->xdeathstate);
    }
    else
        P_SetMobjState (target, target->info->deathstate);
    target->tics -= P_Random()&3;

    if (target->tics < 1)
        target->tics = 1;



    switch (target->type)
    {
      case MT_WOLFSS:
      case MT_POSSESSED:
        item = MT_CLIP;
        break;

      case MT_SHOTGUY:
        item = MT_SHOTGUN;
        break;

      case MT_CHAINGUY:
        item = MT_CHAINGUN;
        break;

      default:
        return;
    }

    mo = P_SpawnMobj (target->x,target->y,ONFLOORZ, item);
    mo->flags |= MF_DROPPED;
}




void
P_DamageMobj
( mobj_t*       target,
  mobj_t*       inflictor,
  mobj_t*       source,
  int           damage )
{
    unsigned    ang;
    int         saved;
    player_t*   player;
    fixed_t     thrust;
    int         temp;

    if ( !(target->flags & MF_SHOOTABLE) )
        return;

    if (target->health <= 0)
        return;

    if ( target->flags & MF_SKULLFLY )
    {
        target->momx = target->momy = target->momz = 0;
    }

    player = target->player;
    if (player && gameskill == sk_baby)
        damage >>= 1;


    if (inflictor
        && !(target->flags & MF_NOCLIP)
        && (!source
            || !source->player
            || source->player->readyweapon != wp_chainsaw))
    {
        ang = R_PointToAngle2 ( inflictor->x,
                                inflictor->y,
                                target->x,
                                target->y);

        thrust = damage*(FRACUNIT>>3)*100/target->info->mass;

        if ( damage < 40
             && damage > target->health
             && target->z - inflictor->z > 64*FRACUNIT
             && (P_Random ()&1) )
        {
            ang += ANG180;
            thrust *= 4;
        }

        ang >>= ANGLETOFINESHIFT;
        target->momx += FixedMul (thrust, finecosine[ang]);
        target->momy += FixedMul (thrust, finesine[ang]);
    }

    if (player)
    {
        if (target->subsector->sector->special == 11
            && damage >= target->health)
        {
            damage = target->health - 1;
        }


        if ( damage < 1000
             && ( (player->cheats&CF_GODMODE)
                  || player->powers[pw_invulnerability] ) )
        {
            return;
        }

        if (player->armortype)
        {
            if (player->armortype == 1)
                saved = damage/3;
            else
                saved = damage/2;

            if (player->armorpoints <= saved)
            {
                saved = player->armorpoints;
                player->armortype = 0;
            }
            player->armorpoints -= saved;
            damage -= saved;
        }
        player->health -= damage;
        if (player->health < 0)
            player->health = 0;

        player->attacker = source;
        player->damagecount += damage;

        if (player->damagecount > 100)
            player->damagecount = 100;

        temp = damage < 100 ? damage : 100;

        if (player == &players[consoleplayer])
            I_Tactile (40,10,40+temp*2);
    }

    target->health -= damage;
    if (target->health <= 0)
    {
        P_KillMobj (source, target);
        return;
    }

    if ( (P_Random () < target->info->painchance)
         && !(target->flags&MF_SKULLFLY) )
    {
        target->flags |= MF_JUSTHIT;

        P_SetMobjState (target, target->info->painstate);
    }

    target->reactiontime = 0;

    if ( (!target->threshold || target->type == MT_VILE)
         && source && source != target
         && source->type != MT_VILE)
    {
        target->target = source;
        target->threshold = BASETHRESHOLD;
        if (target->state == &states[target->info->spawnstate]
            && target->info->seestate != S_NULL)
            P_SetMobjState (target, target->info->seestate);
    }

}

