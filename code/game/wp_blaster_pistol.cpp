/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "g_local.h"
#include "b_local.h"
#include "wp_saber.h"
#include "w_local.h"


//---------------
//	Bryar Pistol
//---------------

//---------------------------------------------------------
void WP_FireBryarPistol( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage = !alt_fire ? weaponData[WP_BRYAR_PISTOL].damage : weaponData[WP_BRYAR_PISTOL].altDamage;
	float	velocity = BRYAR_PISTOL_VEL;

	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer > 2 )
		{
			velocity = floor((float)BRYAR_PISTOL_VEL * (1.0f + (0.1f * (g_spskill->integer - 2))));

			if ( velocity > Q3_INFINITE )
			{
				velocity = Q3_INFINITE;
			}
		}

		if ( g_spskill->integer <= 0 )
		{
			damage -= 7;
		}
		else if ( g_spskill->integer > 1 )
		{
			damage += 7;
		}
	}

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
		|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
	{//force sight 2+ gives perfect aim
		//FIXME: maybe force sight level 3 autoaims some?
		if ( ent->NPC && ent->NPC->currentAim < 5 )
		{
			vec3_t	angs;

			vectoangles( forwardVec, angs );

			if ( ent->client->NPC_class == CLASS_IMPWORKER )
			{//*sigh*, hack to make impworkers less accurate without affecteing imperial officer accuracy
				angs[PITCH] += ( Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
				angs[YAW]	+= ( Q_flrand(-1.0f, 1.0f) * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
			}
			else
			{
				angs[PITCH] += ( Q_flrand(-1.0f, 1.0f) * ((5-ent->NPC->currentAim)*0.25f) );
				angs[YAW]	+= ( Q_flrand(-1.0f, 1.0f) * ((5-ent->NPC->currentAim)*0.25f) );
			}

			AngleVectors( angs, forwardVec, NULL, NULL );
		}
	}

	WP_MissileTargetHint(ent, start, forwardVec);

	gentity_t	*missile = CreateMissile( start, forwardVec, velocity, 10000, ent, alt_fire );

	char		*projName = "bryar_proj";

	switch ( ent->s.weapon )
	{//*SIGH*... I hate our weapon system...
	case WP_BLASTER_PISTOL:
	case WP_JAWA:
		missile->s.weapon = ent->s.weapon;
		projName = "pistol_proj";
		break;
	default:
		missile->s.weapon = WP_BRYAR_PISTOL;
		break;
	}

	missile->classname = projName;

	if ( alt_fire )
	{
		int count = ( level.time - ent->client->ps.weaponChargeTime ) / BRYAR_CHARGE_UNIT;

		if ( count < 1 )
		{
			count = 1;
		}
		else if ( count > 5 )
		{
			count = 5;
		}

		damage *= count;
		missile->count = count; // this will get used in the projectile rendering code to make a beefier effect
	}

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	if ( alt_fire )
	{
		missile->methodOfDeath = MOD_BRYAR_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BRYAR;
	}

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if ( ent->weaponModel[1] > 0 )
	{//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = (ent->count)?0:1;
	}
}