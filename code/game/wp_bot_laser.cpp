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

#include "b_local.h"
#include "g_local.h"
#include "w_local.h"

//	Bot Laser
//---------------------------------------------------------
void WP_BotLaser( gentity_t *ent )
//---------------------------------------------------------
{
	int		damage = BRYAR_PISTOL_DAMAGE;
	float	velocity = BRYAR_PISTOL_VEL;

	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer < 0 )
		{
			damage = 7;
		}
		else
		{
			switch ( g_spskill->integer )
			{
			case 0:
				damage = 7;
				break;
			case 1:
				break;
			case 2:
			default:
				damage = 21;
				break;
			}
		}

		if ( g_spskill->integer > 2 )
		{
			velocity = floor((float)BRYAR_PISTOL_VEL * (1.0f + (0.1f * (g_spskill->integer - 2))));

			if ( velocity > Q3_INFINITE )
			{
				velocity = Q3_INFINITE;
			}
		}
	}

	gentity_t	*missile = CreateMissile( muzzle, forwardVec, velocity, 10000, ent );

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_ENERGY;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
}