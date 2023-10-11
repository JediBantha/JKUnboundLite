
#include "cg_headers.h"

#include "cg_media.h"
#include "FxScheduler.h"

void FX_BlasterPistolProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->gent->s.pos.trDelta, forward ) == 0.0f )
	{
		if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
		{
			forward[2] = 1.0f;
		}
	}

	int dif = cg.time - cent->gent->s.pos.trTime;

	if ( dif < 75 )
	{
		if ( dif < 0 )
		{
			dif = 0;
		}

		float scale = ( dif / 75.0f ) * 0.95f + 0.05f;

		VectorScale( forward, scale, forward );
	}

	if ( cent->gent && cent->gent->owner && cent->gent->owner->s.number > 0 )
	{
		theFxScheduler.PlayEffect( "blaster_pistol/NPCshot", cent->lerpOrigin, forward );
	}
	else
	{
		theFxScheduler.PlayEffect( "blaster_pistol/shot", cent->lerpOrigin, forward );
	}
}

void FX_BlasterPistolAltProjectileThink( centity_t *cent, const struct weaponInfo_s *weapon )
{
	vec3_t forward;

	if ( VectorNormalize2( cent->gent->s.pos.trDelta, forward ) == 0.0f )
	{
		if ( VectorNormalize2( cent->currentState.pos.trDelta, forward ) == 0.0f )
		{
			forward[2] = 1.0f;
		}
	}

	int dif = cg.time - cent->gent->s.pos.trTime;

	if ( dif < 75 )
	{
		if ( dif < 0 )
		{
			dif = 0;
		}

		float scale = ( dif / 75.0f ) * 0.95f + 0.05f;

		VectorScale( forward, scale, forward );
	}

	for ( int t = 1; t < cent->gent->count; t++ )
	{
		theFxScheduler.PlayEffect( "blaster_pistol/crackleShot", cent->lerpOrigin, forward );
	}

	theFxScheduler.PlayEffect( "blaster_pistol/shot", cent->lerpOrigin, forward );
}