
#include "g_local.h"
#include "b_local.h"
#include "g_functions.h"
#include "wp_saber.h"
#include "w_local.h"
#include "../cgame/cg_local.h"

#define	SCEPTER_DAMAGE				6

#define SCEPTER_ALT_DAMAGE			18
#define SCEPTER_CHARGE_UNIT			200.0f

void WP_ScepterMainFire( gentity_t *ent )
{
	int			min = SCEPTER_DAMAGE, max = (min * 2);
	int			damage = Q_irand( SCEPTER_DAMAGE, (min * 2) );
	int			dflags = DAMAGE_NO_KNOCKBACK;

	if ( !ent->ghoul2.size() || ent->weaponModel[0] <= 0 )
	{
		return;
	}
	
	if ( ent->genericBolt1 != -1 )
	{
		int			curTime = (cg.time ? cg.time : level.time);
		qboolean	hit = qfalse;
		int			lastHit = ENTITYNUM_NONE;
		
		for ( int time = curTime - 25; (time <= (curTime + 25) && !hit); time += 25 )
		{
			mdxaBone_t	boltMatrix;
			vec3_t		tip, dir, base, angles = { 0, NPC->currentAngles[YAW], 0 };
			trace_t		trace;

			gi.G2API_GetBoltMatrix( 
				ent->ghoul2, 
				ent->weaponModel[0],
				ent->genericBolt1,
				&boltMatrix, 
				angles, 
				ent->currentOrigin, 
				time,
				NULL, 
				ent->s.modelScale 
			);

			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, base );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_X, dir );

			VectorMA( base, 512, dir, tip );
			
			gi.trace( &trace, base, vec3_origin, vec3_origin, 
				tip, ent->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
			
			if ( trace.fraction < 1.0f )
			{
				gentity_t *traceEnt = &g_entities[trace.entityNum];

				if ( time == curTime )
				{
					G_PlayEffect( G_EffectIndex( "scepter/impact.efx" ), trace.endpos, trace.plane.normal );
				}

				if ( traceEnt->takedamage
					&& trace.entityNum != lastHit
					&& (!traceEnt->client || traceEnt == ent->enemy) )
				{
					G_Damage( 
						traceEnt, 
						ent, 
						ent, 
						vec3_origin, 
						trace.endpos, 
						damage,
						dflags,
						MOD_SCEPTER 
					);
					
					if ( traceEnt->client )
					{
						G_Throw( traceEnt, dir, Q_flrand( 50, 80 ) );

						if ( traceEnt->health > 0 && !Q_irand( 0, (1 + G_DifficultyLimit()) ) )
						{
							G_Knockdown( traceEnt, NPC, dir, 300, qtrue );
						}
					}

					hit = qtrue;
					lastHit = trace.entityNum;
				}
			}
		}
	}
}

void WP_ScepterAltFire( gentity_t *ent )
{
	int			damage = SCEPTER_ALT_DAMAGE, skip, traces = DISRUPTOR_ALT_TRACES;
	int			dflags = (DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC|DAMAGE_HEAVY_WEAP_CLASS);
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	vec3_t		muzzle2, spot, dir;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		dist, shotDist, shotRange = 8192;
	qboolean	hitDodged = qfalse;

	if ( !ent->ghoul2.size() || ent->weaponModel[0] <= 0 )
	{
		return;
	}
	
	if ( ent->genericBolt1 != -1 )
	{
		ent->client->ps.groundEntityNum = ENTITYNUM_NONE;

		if ( (ent->client->ps.pm_flags & PMF_DUCKED) )
		{
			ent->client->ps.pm_time = 100;
		}
		else
		{
			ent->client->ps.pm_time = 250;
		}

		ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK|PMF_TIME_NOFRICTION;

		VectorCopy( muzzle, muzzle2 );

		VectorCopy( muzzle, start );
		WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

		int count = ( level.time - ent->client->ps.weaponChargeTime ) / SCEPTER_CHARGE_UNIT;

		if ( count < 1 )
		{
			count = 1;
		}
		else if ( count > 5 )
		{
			count = 5;
		}

		damage *= count;
		skip = ent->s.number;

		vec3_t shot_mins, shot_maxs;

		VectorSet( shot_mins, -1, -1, -1 );
		VectorSet( shot_maxs, 1, 1, 1 );

		for ( int i = 0; i < traces; i++ )
		{
			VectorMA(start, shotRange, forwardVec, end);

			gi.trace(&tr, start, shot_mins, shot_maxs, 
				end, skip, MASK_SHOT, G2_COLLIDE, 10);

			if (tr.surfaceFlags & SURF_NOIMPACT)
			{
				render_impact = qfalse;
			}

			if (tr.entityNum == ent->s.number)
			{
				VectorCopy(tr.endpos, muzzle2);
				VectorCopy(tr.endpos, start);

				skip = tr.entityNum;

				continue;
			}

			if (tr.fraction >= 1.0f)
			{
				break;
			}

			traceEnt = &g_entities[tr.entityNum];

			if (traceEnt
				&& (traceEnt->s.weapon == WP_SABER || 
					(traceEnt->client 
						&& (traceEnt->client->NPC_class == CLASS_BOBAFETT || traceEnt->client->NPC_class == CLASS_REBORN))))
			{
				hitDodged = Jedi_DodgeEvasion(traceEnt, ent, &tr, HL_NONE);
			}

			if (!hitDodged)
			{
				if (render_impact)
				{
					if ((tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage)
						|| !Q_stricmp(traceEnt->classname, "misc_model_breakable")
						|| traceEnt->s.eType == ET_MOVER)
					{
						G_PlayEffect(G_EffectIndex("scepter/player/alt_hit"), tr.endpos, tr.plane.normal);

						if (traceEnt->client && LogAccuracyHit(traceEnt, ent))
						{
							ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
						}

						int hitLoc = G_GetHitLocFromTrace(&tr, MOD_CONC_ALT);
						qboolean noKnockBack = (qboolean)((traceEnt->flags & FL_NO_KNOCKBACK) != 0);
						
						if (traceEnt 
							&& traceEnt->client 
							&& traceEnt->client->NPC_class == CLASS_GALAKMECH)
						{
							G_Damage(
								traceEnt, 
								ent, 
								ent, 
								forwardVec, 
								tr.endpos, 
								10, 
								dflags, 
								MOD_SCEPTER_ALT, 
								hitLoc
							);
							break;
						}

						G_Damage(
							traceEnt, 
							ent, 
							ent, 
							forwardVec, 
							tr.endpos, 
							damage, 
							dflags, 
							MOD_SCEPTER_ALT, 
							hitLoc
						);

						if (traceEnt->client)
						{
							vec3_t pushDir;

							VectorCopy(forwardVec, pushDir);

							if (pushDir[2] < 0.2f)
							{
								pushDir[2] = 0.2f;
							}

							//if ( traceEnt->NPC || Q_irand(0,g_spskill->integer+1) )
							{
								if (!noKnockBack)
								{
									G_Throw(traceEnt, pushDir, 200);

									if (traceEnt->client->NPC_class == CLASS_ROCKETTROOPER)
									{
										traceEnt->client->ps.pm_time = Q_irand(1500, 3000);
									}
								}

								if (traceEnt->health > 0)
								{
									if (G_HasKnockdownAnims(traceEnt))
									{
										G_Knockdown(traceEnt, ent, pushDir, 400, qtrue);
									}
								}
							}
						}

						if (traceEnt->s.eType == ET_MOVER)
						{
							break;
						}
					}
					else
					{
						tent = G_TempEntity(tr.endpos, EV_SCEPTER_ALT_MISS);
						tent->svFlags |= SVF_BROADCAST;
						VectorCopy(tr.plane.normal, tent->pos1);
						break;
					}
				}
				else
				{
					break;
				}
			}

			VectorCopy(tr.endpos, muzzle2);
			VectorCopy(tr.endpos, start);

			skip = tr.entityNum;
			hitDodged = qfalse;
		}
		
		tent = G_TempEntity( tr.endpos, EV_SCEPTER_ALT_SHOT );
		tent->svFlags |= SVF_BROADCAST;
		VectorCopy( muzzle, tent->s.origin2 );

		VectorSubtract( tr.endpos, muzzle, dir );

		shotDist = VectorNormalize( dir );

		for ( dist = 0; dist < shotDist; dist += 64 )
		{
			VectorMA( muzzle, dist, dir, spot );
			AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
		}

		VectorMA( start, shotDist-4, forwardVec, spot );
		AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );

		G_PlayEffect( G_EffectIndex( "scepter/player/altmuzzle_flash" ), muzzle, forwardVec );
	}
}

void WP_Scepter( gentity_t *ent, qboolean alt_fire )
{
	if ( alt_fire )
	{
		WP_ScepterAltFire( ent );
	}
	else
	{
		WP_ScepterMainFire( ent );
	}
}