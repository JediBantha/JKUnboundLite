
#include "cg_headers.h"
#include "cg_media.h"
#include "FxScheduler.h"

static vec3_t WHITE	= { 1.0f, 1.0f, 1.0f };

void FX_ScepterAltShot( vec3_t start, vec3_t end )
{
	FX_AddLine( 
		-1, 
		start, 
		end, 
		0.1f, 
		10.0f, 
		0.0f,
		1.0f, 
		0.0f, 
		0.0f,
		WHITE, 
		WHITE, 
		0.0f,
		175, 
		cgi_R_RegisterShader( "gfx/effects/orangeLine" ),
		0, 
		FX_SIZE_LINEAR | FX_ALPHA_LINEAR 
	);

	vec3_t	BRIGHT = { 1.0f, 0.5f, 0.1f };

	FX_AddLine( 
		-1, 
		start, 
		end, 
		0.1f, 
		7.0f, 
		0.0f,
		1.0f, 
		0.0f, 
		0.0f,
		BRIGHT, 
		BRIGHT, 
		0.0f,
		150, 
		cgi_R_RegisterShader( "gfx/misc/whiteline2" ),
		0, 
		FX_SIZE_LINEAR | FX_ALPHA_LINEAR 
	);
}

void FX_ScepterAltMiss( vec3_t origin, vec3_t normal )
{
	vec3_t pos, c1, c2;

	VectorMA( origin, 4.0f, normal, c1 );
	VectorCopy( c1, c2 );

	c1[2] += 4;
	c2[2] += 12;

	VectorAdd( origin, normal, pos );

	pos[2] += 28;

	FX_AddBezier( 
		origin, 
		pos, 
		c1, 
		vec3_origin, 
		c2, 
		vec3_origin, 
		6.0f, 
		6.0f, 
		0.0f, 
		0.0f, 
		0.2f, 
		0.5f, 
		WHITE, 
		WHITE, 
		0.0f, 
		4000, 
		cgi_R_RegisterShader( "gfx/effects/smokeTrail" ), 
		FX_ALPHA_WAVE 
	);

	theFxScheduler.PlayEffect( "scepter/player/alt_miss", origin, normal );
}