
#include "g_local.h"
#include "w_local.h"
#include "g_functions.h"
#include "g_items.h"
#include "wp_saber.h"
#include "../cgame/cg_local.h"
#include "b_local.h"

//===============================================
//Portable E-Web -rww
//===============================================
//put the e-web away/remove it from the owner
void EWebDisattach(gentity_t *owner, gentity_t *eweb)
{
    owner->client->ewebIndex = 0;
	owner->client->ps.emplacedIndex = 0;
	
	if (owner->health > 0)
	{
		owner->client->ps.stats[STAT_WEAPONS] = WP_EMPLACED_GUN;
	}
	else
	{
		owner->client->ps.stats[STAT_WEAPONS] = 0;
	}
	
	eweb->think = G_FreeEntity;
	eweb->nextthink = level.time;
}

//precache misc e-web assets
void EWebPrecache(void)
{
	RegisterItem( FindItemForWeapon(WP_TURRET) );
	G_EffectIndex("detpack/explosion.efx");
	G_EffectIndex("turret/muzzle_flash.efx");
}

//e-web death
#define EWEB_DEATH_RADIUS		128
#define EWEB_DEATH_DMG			90

extern void BG_CycleInven(playerState_t *ps, int direction); //bg_misc.c

void EWebDie(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	vec3_t fxDir;

	G_RadiusDamage(self->currentOrigin, self, EWEB_DEATH_DMG, EWEB_DEATH_RADIUS, self, MOD_SUICIDE);

	VectorSet(fxDir, 1.0f, 0.0f, 0.0f);
	G_PlayEffect( "detpack/explosion", self->currentOrigin, fxDir);

	if (self->r.ownerNum != ENTITYNUM_NONE)
	{
		gentity_t *owner = &g_entities[self->r.ownerNum];

		if (owner->inuse && owner->client)
		{
			EWebDisattach(owner, self);

			//make sure it resets next time we spawn one in case we someone obtain one before death
			owner->client->ewebHealth = -1;
			
			for ( int i = 1 ; i < bg_numItems ; i++ )
			{
				//take it away from him, it is gone forever.
				owner->client->ps.stats[STAT_ITEMS] &= ~(1<<INV_EWEB);

				if (owner->client->ps.stats[STAT_ITEMS] > 0 &&
					bg_itemlist[owner->client->ps.stats[STAT_ITEMS]].giType == IT_HOLDABLE &&
					bg_itemlist[owner->client->ps.stats[STAT_ITEMS]].giTag == INV_EWEB)
				{ //he has it selected so deselect it and select the first thing available
					owner->client->ps.stats[STAT_ITEMS] = 0;
					BG_CycleInven(&owner->client->ps, 1);
				}
			}
		}
	}
}

//e-web pain
void EWebPain(gentity_t *self, gentity_t *attacker, int damage)
{
	//update the owner's health status of me
	if (self->r.ownerNum != ENTITYNUM_NONE)
	{
		gentity_t *owner = &g_entities[self->r.ownerNum];

		if (owner->inuse && owner->client)
		{
			owner->client->ewebHealth = self->health;
		}
	}
}

//special routine for tracking angles between client and server
void EWeb_SetBoneAngles(gentity_t *ent, char *bone, vec3_t angles)
{
	int *thebone = &ent->s.boneIndex1;
	int *firstFree = NULL;
	int i = 0;
	int boneIndex = G_BoneIndex(bone);
	int flags, up, right, forward;
	vec3_t *boneVector = &ent->s.boneAngles1;
	vec3_t *freeBoneVec = NULL;

	while (thebone)
	{
		if (!*thebone && !firstFree)
		{ //if the value is 0 then this index is clear, we can use it if we don't find the bone we want already existing.
			firstFree = thebone;
			freeBoneVec = boneVector;
		}
		else if (*thebone)
		{
			if (*thebone == boneIndex)
			{ //this is it
				break;
			}
		}

		switch (i)
		{
		case 0:
			thebone = &ent->s.boneIndex2;
			boneVector = &ent->s.boneAngles2;
			break;
		case 1:
			thebone = &ent->s.boneIndex3;
			boneVector = &ent->s.boneAngles3;
			break;
		case 2:
			thebone = &ent->s.boneIndex4;
			boneVector = &ent->s.boneAngles4;
			break;
		default:
			thebone = NULL;
			boneVector = NULL;
			break;
		}

		i++;
	}

	if (!thebone)
	{ //didn't find it, create it
		if (!firstFree)
		{ //no free bones.. can't do a thing then.
			Com_Printf("WARNING: E-Web has no free bone indexes\n");
			return;
		}

		thebone = firstFree;

		*thebone = boneIndex;
		boneVector = freeBoneVec;
	}

	//If we got here then we have a vector and an index.

	//Copy the angles over the vector in the entitystate, so we can use the corresponding index
	//to set the bone angles on the client.
	VectorCopy(angles, *boneVector);

	//Now set the angles on our server instance if we have one.

	if (!ent->ghoul2)
	{
		return;
	}

	flags = BONE_ANGLES_POSTMULT;
	up = POSITIVE_Y;
	right = NEGATIVE_Z;
	forward = NEGATIVE_X;

	//first 3 bits is forward, second 3 bits is right, third 3 bits is up
	ent->s.boneOrient = ((forward)|(right<<3)|(up<<6));

	trap->G2API_SetBoneAngles( ent->ghoul2,
					0,
					bone,
					angles,
					flags,
					up,
					right,
					forward,
					NULL,
					100,
					level.time );
}

//start an animation on model_root both server side and client side
void EWeb_SetBoneAnim(gentity_t *eweb, int startFrame, int endFrame)
{
	//set info on the entity so it knows to start the anim on the client next snapshot.
	eweb->s.eFlags |= EF_G2ANIMATING;

	if (eweb->s.torsoAnim == startFrame && eweb->s.legsAnim == endFrame)
	{ //already playing this anim, let's flag it to restart
		eweb->s.torsoFlip = !eweb->s.torsoFlip;
	}
	else
	{
		eweb->s.torsoAnim = startFrame;
		eweb->s.legsAnim = endFrame;
	}

	//now set the animation on the server ghoul2 instance.
	assert(eweb->ghoul2);

	gi.G2API_SetBoneAnim(&eweb->ghoul2[0], "model_root", startFrame, endFrame,
		(BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND), 1.0f, level.time, -1, 100);
}

//fire a shot off
#define EWEB_MISSILE_DAMAGE			20
void EWebFire(gentity_t *owner, gentity_t *eweb)
{
	mdxaBone_t boltMatrix;
	gentity_t *missile;
	vec3_t p, d, bPoint;

	if (eweb->genericValue10 == -1)
	{ //oh no
		assert(!"Bad e-web bolt");
		return;
	}

	//get the muzzle point
	gi.G2API_GetBoltMatrix(eweb->ghoul2, 0, eweb->genericValue10, &boltMatrix, eweb->s.apos.trBase, eweb->r.currentOrigin, level.time, NULL, eweb->modelScale);
	gi.G2API_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, p);
	gi.G2API_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, d);

	//Start the thing backwards into the bounding box so it can't start inside other solid things
	VectorMA(p, -16.0f, d, bPoint);

	//create the missile
	missile = CreateMissile( bPoint, d, 1200.0f, 10000, owner, qfalse );

	missile->classname = "emplaced_proj";
	missile->s.weapon = WP_TURRET;

	missile->damage = (EMPLACED_DAMAGE/4);
	missile->dflags = (DAMAGE_DEATH_KNOCKBACK|DAMAGE_HEAVY_WEAP_CLASS);
	missile->methodOfDeath = MOD_EMPLACED;
	missile->clipmask = (MASK_SHOT|CONTENTS_LIGHTSABER);

	//ignore the e-web entity
	missile->passThroughNum = eweb->s.number+1;

	//times it can bounce before it dies
	missile->bounceCount = 8;

	//play the muzzle flash
	vectoangles(d, d);
	CG_PlayEffectID( G_EffectIndex("turret/muzzle_flash.efx"), p, d);
}

//lock the owner into place relative to the cannon pos
void EWebPositionUser(gentity_t *owner, gentity_t *eweb)
{
	mdxaBone_t boltMatrix;
	vec3_t p, d;
	trace_t tr;

	gi.G2API_GetBoltMatrix(eweb->ghoul2, 0, eweb->genericValue9, &boltMatrix, eweb->s.apos.trBase, eweb->r.currentOrigin, level.time, NULL, eweb->modelScale);
	gi.G2API_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, p);
	gi.G2API_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, d);

	VectorMA(p, 32.0f, d, p);
	p[2] = eweb->currentOrigin[2];

	p[2] += 4.0f;

	gi.trace(&tr, owner->client->ps.origin, owner->mins, owner->maxs, p, owner->s.number, MASK_PLAYERSOLID, G2_NOCOLLIDE, 0);

	if (!tr.startsolid && !tr.allsolid && tr.fraction == 1.0f)
	{ //all clear, we can move there
		vec3_t pDown;

		VectorCopy(p, pDown);
		pDown[2] -= 7.0f;
		gi.trace(&tr, p, owner->mins, owner->maxs, pDown, owner->s.number, MASK_PLAYERSOLID, G2_NOCOLLIDE, 0);

		if (!tr.startsolid && !tr.allsolid)
		{
			VectorSubtract(owner->client->ps.origin, tr.endpos, d);
			if (VectorLength(d) > 1.0f)
			{ //we moved, do some animating
				vec3_t dAng;
				int aFlags = SETANIM_FLAG_HOLD;

				vectoangles(d, dAng);
				dAng[YAW] = AngleSubtract(owner->client->ps.viewangles[YAW], dAng[YAW]);
				
				if (dAng[YAW] > 0.0f)
				{
					if (owner->client->ps.legsAnim == BOTH_STRAFE_RIGHT1)
					{ //reset to change direction
						aFlags |= SETANIM_FLAG_OVERRIDE;
					}
					
					NPC_SetAnim(owner, &owner->client->pers.cmd, SETANIM_LEGS, BOTH_STRAFE_LEFT1, aFlags);
				}
				else
				{
					if (owner->client->ps.legsAnim == BOTH_STRAFE_LEFT1)
					{ //reset to change direction
						aFlags |= SETANIM_FLAG_OVERRIDE;
					}

					NPC_SetAnim(owner, &owner->client->pers.cmd, SETANIM_LEGS, BOTH_STRAFE_RIGHT1, aFlags);
				}
			}
			else if (owner->client->ps.legsAnim == BOTH_STRAFE_RIGHT1 || owner->client->ps.legsAnim == BOTH_STRAFE_LEFT1)
			{ //don't keep animating in place
				owner->client->ps.legsTimer = 0;
			}

			G_SetOrigin(owner, tr.endpos);
			VectorCopy(tr.endpos, owner->client->ps.origin);
		}
	}
	else
	{ //can't move here.. stop using the thing I guess
		EWebDisattach(owner, eweb);
	}
}

//keep the bone angles updated based on the owner's look dir
void EWebUpdateBoneAngles(gentity_t *owner, gentity_t *eweb)
{
	vec3_t yAng;
	float ideal;
	float incr;
	const float turnCap = 4.0f; //max degrees we can turn per update

	VectorClear(yAng);
	ideal = AngleSubtract(owner->client->ps.viewangles[YAW], eweb->s.angles[YAW]);
	incr = AngleSubtract(ideal, eweb->angle);

	if (incr > turnCap)
	{
		incr = turnCap;
	}
	else if (incr < -turnCap)
	{
		incr = -turnCap;
	}

	eweb->angle += incr;

	yAng[0] = eweb->angle;
	EWeb_SetBoneAngles(eweb, "cannon_Yrot", yAng);

	EWebPositionUser(owner, eweb);
	if (!owner->client->ewebIndex)
	{ //was removed during position function
		return;
	}

	VectorClear(yAng);
	yAng[2] = AngleSubtract(owner->client->ps.viewangles[PITCH], eweb->s.angles[PITCH])*0.8f;
	EWeb_SetBoneAngles(eweb, "cannon_Xrot", yAng);
}

//keep it updated
extern int BG_EmplacedView(vec3_t baseAngles, vec3_t angles, float *newYaw, float constraint); //bg_misc.c

void EWebThink(gentity_t *self)
{
	qboolean killMe = qfalse;
	const float gravity = 3.0f;
	const float mass = 0.09f;
	const float bounce = 1.1f;

	if (self->r.ownerNum == ENTITYNUM_NONE)
	{
		killMe = qtrue;
	}
	else
	{
		gentity_t *owner = &g_entities[self->r.ownerNum];

		if (!owner->inuse || !owner->client || owner->client->pers.connected != CON_CONNECTED ||
			owner->client->ewebIndex != self->s.number || owner->health < 1)
		{
			killMe = qtrue;
		}
		else if (owner->client->ps.emplacedIndex != self->s.number)
		{ //just go back to the inventory then
			EWebDisattach(owner, self);
			return;
		}

		if (!killMe)
		{
			float yaw;

			if (BG_EmplacedView(owner->client->ps.viewangles, self->s.angles, &yaw, self->s.origin2[0]))
			{
				owner->client->ps.viewangles[YAW] = yaw;
			}
			owner->client->ps.weapon = WP_EMPLACED_GUN;
			owner->client->ps.stats[STAT_WEAPONS] = WP_EMPLACED_GUN;

			if (self->genericValue8 < level.time)
			{ //make sure the anim timer is done
				EWebUpdateBoneAngles(owner, self);
				if (!owner->client->ewebIndex)
				{ //was removed during position function
					return;
				}

				if (owner->client->pers.cmd.buttons & BUTTON_ATTACK)
				{
					if (self->genericValue5 < level.time)
					{ //we can fire another shot off
						EWebFire(owner, self);

						//cheap firing anim
						EWeb_SetBoneAnim(self, 2, 4);
						self->genericValue3 = 1;

						//set fire debounce time
						self->genericValue5 = level.time + 100;
					}
				}
				else if (self->genericValue5 < level.time && self->genericValue3)
				{ //reset the anim back to non-firing
					EWeb_SetBoneAnim(self, 0, 1);
					self->genericValue3 = 0;
				}
			}
		}
	}

	if (killMe)
	{ //something happened to the owner, let's explode
		EWebDie(self, self, self, 999, MOD_SUICIDE);
		return;
	}

	//run some physics on it real quick so it falls and stuff properly
	//G_RunExPhys(self, gravity, mass, bounce, qfalse, NULL, 0);

	self->nextthink = level.time;
}

#define EWEB_HEALTH			200

//spawn and set up an e-web ent
gentity_t *EWeb_Create(gentity_t *spawner)
{
	const char *modelName = "models/map_objects/hoth/eweb_model.glm";
	int failSound = G_SoundIndex("sound/interface/shieldcon_empty");
	
	gentity_t *ent;
	trace_t tr;
	vec3_t fAng, fwd, pos, downPos, s;
	vec3_t mins, maxs;

	VectorSet(mins, -32, -32, -24);
	VectorSet(maxs, 32, 32, 24);

	VectorSet(fAng, 0, spawner->client->ps.viewangles[1], 0);
	AngleVectors(fAng, fwd, 0, 0);

	VectorCopy(spawner->client->ps.origin, s);
	//allow some fudge
	s[2] += 12.0f;

	VectorMA(s, 48.0f, fwd, pos);

	gi.trace(&tr, s, mins, maxs, pos, spawner->s.number, MASK_PLAYERSOLID, G2_NOCOLLIDE, 0);

	if (tr.allsolid || tr.startsolid || tr.fraction != 1.0f)
	{ //can't spawn here, we are in solid
		G_Sound(spawner, CHAN_AUTO, failSound);
		return NULL;
	}

	ent = G_Spawn();

	ent->clipmask = MASK_PLAYERSOLID;
	ent->contents = MASK_PLAYERSOLID;

	ent->physicsObject = qtrue;

	//for the sake of being able to differentiate client-side between this and an emplaced gun
	ent->s.weapon = WP_NONE;

	VectorCopy(pos, downPos);
	downPos[2] -= 18.0f;
	gi.trace(&tr, pos, mins, maxs, downPos, spawner->s.number, MASK_PLAYERSOLID, G2_NOCOLLIDE, 0);

	if (tr.startsolid || 
		tr.allsolid || 
		tr.fraction == 1.0f || 
		tr.entityNum < ENTITYNUM_WORLD)
	{ //didn't hit ground.
		G_FreeEntity(ent);
		G_Sound(spawner, CHAN_AUTO, failSound);
		return NULL;
	}

	VectorCopy(tr.endpos, pos);

	G_SetOrigin(ent, pos);

	VectorCopy(fAng, ent->s.apos.trBase);
	VectorCopy(fAng, ent->currentAngles);

	ent->owner = spawner->s.number;
	//ent->teamowner = spawner->client->sess.sessionTeam;

	ent->takedamage = qtrue;

	if (spawner->client->ewebHealth <= 0)
	{ //refresh the owner's e-web health if its last e-web did not exist or was killed
		spawner->client->ewebHealth = EWEB_HEALTH;
	}

	//resume health of last deployment
	ent->maxHealth = EWEB_HEALTH;
	ent->health = spawner->client->ewebHealth;
	G_ScaleNetHealth(ent);

	ent->die = EWebDie;
	ent->pain = EWebPain;

	ent->think = EWebThink;
	ent->nextthink = level.time;

	//set up the g2 model info
	ent->s.modelGhoul2 = 1;
	ent->s.g2radius = 128;
	ent->s.modelindex = G_ModelIndex((char *)modelName);

	gi.G2API_InitGhoul2Model( ent->ghoul2, modelName, 0, 0, 0, 0, 0);

	if (!ent->ghoul2)
	{ //should not happen, but just to be safe.
		G_FreeEntity(ent);
		return NULL;
	}

	//initialize bone angles
	EWeb_SetBoneAngles(ent, "cannon_Yrot", vec3_origin);
	EWeb_SetBoneAngles(ent, "cannon_Xrot", vec3_origin);

	ent->genericValue10 = gi.G2API_AddBolt(&ent->ghoul2[0], "*cannonflash"); //muzzle bolt
	ent->genericValue9 = gi.G2API_AddBolt(&ent->ghoul2[0], "cannon_Yrot"); //for placing the owner relative to rotation

	//set the constraints for this guy as an emplaced weapon, and his constraint angles
	ent->s.origin2[0] = 360.0f; //360 degrees in either direction

	VectorCopy(fAng, ent->s.angles); //consider "angle 0" for constraint

	//angle of y rot bone
	ent->angle = 0.0f;

	ent->ownerNum = spawner->s.number;
	gi.linkentity( ent );

	//store off the owner's current weapons, we will be forcing him to use the "emplaced" weapon
	ent->genericValue11 = spawner->client->ps.stats[STAT_WEAPONS];

	//start the "unfolding" anim
	EWeb_SetBoneAnim(ent, 4, 20);
	//don't allow use until the anim is done playing (rough time estimate)
	ent->genericValue8 = level.time + 500;

	VectorCopy(mins, ent->mins);
	VectorCopy(maxs, ent->maxs);

	return ent;
}

#define EWEB_USE_DEBOUNCE		1000
//use the e-web
void ItemUse_UseEWeb(gentity_t *ent)
{
	if (ent->client->ewebTime > level.time)
	{ //can't use again yet
		return;
	}

	if (ent->client->ps.weaponTime > 0 )
	{ //busy doing something else
		return;
	}

	if (ent->client->ps.emplacedIndex && !ent->client->ewebIndex)
	{ //using an emplaced gun already that isn't our own e-web
		return;
	}

	if (ent->client->ewebIndex)
	{ //put it away
		EWebDisattach(ent, &g_entities[ent->client->ewebIndex]);
	}
	else
	{ //create it
		gentity_t *eweb = EWeb_Create(ent);

		if (eweb)
		{ //if it's null the thing couldn't spawn (probably no room)
			ent->client->ewebIndex = eweb->s.number;
			ent->client->ps.emplacedIndex = eweb->s.number;
		}
	}

	ent->client->ewebTime = level.time + EWEB_USE_DEBOUNCE;
}
//===============================================
//End E-Web
//===============================================