#include "Effects.hpp"
#include <Core/Config/Variables.hpp>
#include <Core/Interfaces/List/CParticleSystem.hpp>
#include <Core/Sdk/Datatypes/CGameEvents.hpp>
#include <Core/Sdk/Datatypes/Color.hpp>
#include <Core/Sdk/Datatypes/Vector.hpp>
#include <Core/Sdk/EntityData.hpp>
#include <Core/Sdk/Globals.hpp>
#include <windows.h>

#pragma comment(lib, "Winmm.lib") // Link with the Winmm library


static CEffects g_CEffects{ };

auto CEffects::OnInit( ) -> bool
{
	LoadSounds( );

	m_bInitialized = true;
	return m_bInitialized;
}

auto CEffects::OnDestroy( ) -> void
{
	if ( !m_bInitialized )
		return;
	m_bInitialized = false;
}

auto CEffects::OnLevelInitHook( const char* szNewMap ) -> void
{
	m_flCrosshairMarkTime = -1;
}

auto CEffects::OnLevelShutdownHook( ) -> void
{
	m_flCrosshairMarkTime = -1;
}

auto CEffects::OnPresentHook( ImDrawList* pBackgroundDrawList ) -> void
{
	RenderHitMarkers( pBackgroundDrawList );
	RenderPlayerTrail( pBackgroundDrawList );
}


auto CEffects::RenderHitMarkers( ImDrawList* pBackgroundDrawList ) -> void
{
	const float flDamageHitMarkerSize = 16.f;
	const float flLineSize = 8.f;
	const ImVec2 vecScreenCenter = ImGui::GetIO( ).DisplaySize / 2.f;

	if ( C_GET( bool, Vars.bHitMarkerCrosshair ) && m_flCrosshairMarkTime >= CheatData.flGVCurrentTime )
	{
		animCrosshairHitmark.Update( ImGui::GetIO( ).DeltaTime, .3f );
		const float flHitMarkerCrosshairAlpha = animCrosshairHitmark.GetValue( 1.f );

		pBackgroundDrawList->AddLine( ImVec2( vecScreenCenter.x - flLineSize - .5f, vecScreenCenter.y - flLineSize - .5f ), ImVec2( vecScreenCenter.x - ( flLineSize / 4 ), vecScreenCenter.y - ( flLineSize / 4 ) ), Color_t( 0, 0, 0, static_cast< int >( 255.f * flHitMarkerCrosshairAlpha ) ).GetU32( ), 4.5f );
		pBackgroundDrawList->AddLine( ImVec2( vecScreenCenter.x - flLineSize - .5f, vecScreenCenter.y + flLineSize + .5f ), ImVec2( vecScreenCenter.x - ( flLineSize / 4 ), vecScreenCenter.y + ( flLineSize / 4 ) ), Color_t( 0, 0, 0, static_cast< int >( 255.f * flHitMarkerCrosshairAlpha ) ).GetU32( ), 4.5f );
		pBackgroundDrawList->AddLine( ImVec2( vecScreenCenter.x + flLineSize + .5f, vecScreenCenter.y + flLineSize + .5f ), ImVec2( vecScreenCenter.x + ( flLineSize / 4 ), vecScreenCenter.y + ( flLineSize / 4 ) ), Color_t( 0, 0, 0, static_cast< int >( 255.f * flHitMarkerCrosshairAlpha ) ).GetU32( ), 4.5f );
		pBackgroundDrawList->AddLine( ImVec2( vecScreenCenter.x + flLineSize + .5f, vecScreenCenter.y - flLineSize - .5f ), ImVec2( vecScreenCenter.x + ( flLineSize / 4 ), vecScreenCenter.y - ( flLineSize / 4 ) ), Color_t( 0, 0, 0, static_cast< int >( 255.f * flHitMarkerCrosshairAlpha ) ).GetU32( ), 4.5f );

		pBackgroundDrawList->AddLine( ImVec2( vecScreenCenter.x - flLineSize, vecScreenCenter.y - flLineSize ), ImVec2( vecScreenCenter.x - ( flLineSize / 4 ), vecScreenCenter.y - ( flLineSize / 4 ) ), Color_t( 255, 255, 255, static_cast< int >( 255.f * flHitMarkerCrosshairAlpha ) ).GetU32( ), 1.7f );
		pBackgroundDrawList->AddLine( ImVec2( vecScreenCenter.x - flLineSize, vecScreenCenter.y + flLineSize ), ImVec2( vecScreenCenter.x - ( flLineSize / 4 ), vecScreenCenter.y + ( flLineSize / 4 ) ), Color_t( 255, 255, 255, static_cast< int >( 255.f * flHitMarkerCrosshairAlpha ) ).GetU32( ), 1.7f );
		pBackgroundDrawList->AddLine( ImVec2( vecScreenCenter.x + flLineSize, vecScreenCenter.y + flLineSize ), ImVec2( vecScreenCenter.x + ( flLineSize / 4 ), vecScreenCenter.y + ( flLineSize / 4 ) ), Color_t( 255, 255, 255, static_cast< int >( 255.f * flHitMarkerCrosshairAlpha ) ).GetU32( ), 1.7f );
		pBackgroundDrawList->AddLine( ImVec2( vecScreenCenter.x + flLineSize, vecScreenCenter.y - flLineSize ), ImVec2( vecScreenCenter.x + ( flLineSize / 4 ), vecScreenCenter.y - ( flLineSize / 4 ) ), Color_t( 255, 255, 255, static_cast< int >( 255.f * flHitMarkerCrosshairAlpha ) ).GetU32( ), 1.7f );
	}

	if ( C_GET( bool, Vars.bHitGlobalDamageHitMarker ) )
	{
		std::vector<DamageHitMarker_t> vecDelete{ };
		for ( DamageHitMarker_t& DamageHitMarker : m_vecDamageHitMarkers )
		{
			if ( DamageHitMarker.flCreateTime + m_flDamageHitMarkerLifeTime >= CheatData.flGVCurrentTime )
			{
				DamageHitMarker.vecPosition.z += 25 * ImGui::GetIO( ).DeltaTime;
				ImVec2 vecScreen;
				if ( GetRenderer( )->WorldToScreen( DamageHitMarker.vecPosition, &vecScreen ) )
					pBackgroundDrawList->AddTextOutline( StellarixFonts::fontVerdana, flDamageHitMarkerSize, vecScreen, ImColor( 255, 255, 255 ), std::to_string( DamageHitMarker.iDamage ).c_str( ) );
			}
			else
				vecDelete.push_back( DamageHitMarker );
		}

		for ( DamageHitMarker_t flDelete : vecDelete )
		{
			auto it = std::find_if( m_vecDamageHitMarkers.begin( ), m_vecDamageHitMarkers.end( ), [&]( const DamageHitMarker_t& el )
			{
				return el.flCreateTime == flDelete.flCreateTime;
			} );
			if ( it != m_vecDamageHitMarkers.end( ) )
				m_vecDamageHitMarkers.erase( it );
		}
	}
}

static std::deque<Vector_t> deqPlayerTrail;
auto CEffects::RenderPlayerTrail( ImDrawList* pBackgroundDrawList ) -> void
{
	if ( !C_GET( bool, Vars.bPlayerTrail ) )
		return;

	if ( deqPlayerTrail.size( ) > 250 )
	{
		for ( int i = 0; i < deqPlayerTrail.size( ) - 250; i++ )
			deqPlayerTrail.pop_front( );
	}
	deqPlayerTrail.push_back( LocalPlayerData.vecOrigin );

	ImColor trailColor = C_GET( Color_t, Vars.colPlayerTrailColor ).GetU32( );

	for ( size_t i = 1; i < deqPlayerTrail.size( ); ++i )
	{
		ImVec2 screenPos1, screenPos2;
		if ( !GetRenderer( )->WorldToScreen( deqPlayerTrail[ i - 1 ], &screenPos1 ) )
			continue;
		if ( !GetRenderer( )->WorldToScreen( deqPlayerTrail[ i ], &screenPos2 ) )
			continue;

		pBackgroundDrawList->AddLine(
			ImVec2( screenPos1.x, screenPos1.y ),
			ImVec2( screenPos2.x, screenPos2.y ),
			trailColor,
			3.5f
		);
	}
}


auto CEffects::OnPlayerHurtEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController, CCSPlayerController* pAttackerController, C_CSPlayerPawn* pPlayerPawn, C_CSPlayerPawn* pAttackerPawn, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn ) -> void
{
	if ( pLocalController != pAttackerController )
		return;

	if ( C_GET( bool, Vars.bHitHealthBoostEffect ) )
		pAttackerPawn->m_flHealthShotBoostExpirationTime( ) = CheatData.flGVCurrentTime + m_flHitHealthBoostLifeTime;

	m_flCrosshairMarkTime = CheatData.flGVCurrentTime + m_flHitMarkerLifeTime;
	animCrosshairHitmark.SetSwitch( false );
	animCrosshairHitmark.Update( 0, 0 );
	animCrosshairHitmark.SetSwitch( true );

	if ( C_GET( bool, Vars.bHitGlobalDamageHitMarker ) )
		m_vecDamageHitMarkers.push_back( DamageHitMarker_t( pPlayerPawn->GetEyePosition( ), gameEventHelper.GetDamage( ), CheatData.flGVCurrentTime ) );


	PlayHitSound( );
}

auto CEffects::OnBulletImpactEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController, C_CSPlayerPawn* pPlayerPawn ) -> void
{
	if ( C_GET( bool, Vars.bBulletBeam ) )
		DrawBeam( pPlayerPawn->GetEyePosition( ), gameEventHelper.GetBulletPos( ), C_GET( Color_t, Vars.colBeamColor ) );
}

auto CEffects::DrawBeam( Vector_t vecStart, Vector_t vecEnd, Color_t color ) -> void
{
	if ( !GetInterfaceManager( )->GetGameParticleManager( ) )
		return;

	// Init beam info & color
	static CBeamInfo beamInfo = CBeamInfo( );
	beamInfo.m_ParticleData = {};
	particle_color clr = { color.r, color.g, color.b };

	// Cache a 'new' particle effect (idx -1)
	GetInterfaceManager( )->GetGameParticleManager( )->CacheParticleEffect( &beamInfo.m_nEffectIndex, _xor( "particles/entity/spectator_utility_trail.vpcf" ) );
	// Create the particle effect.
	GetInterfaceManager( )->GetGameParticleManager( )->CreateParticleEffect( beamInfo.m_nEffectIndex, 16, &clr, 0 );

	// Create snapshot of particle
	GetInterfaceManager( )->GetParticleSystemManager( )->CreateSnapshot( &beamInfo.m_hSnapsotParticle );
	// Unk
	GetInterfaceManager( )->GetGameParticleManager( )->UnknownFunction( beamInfo.m_nEffectIndex, 0, &beamInfo.m_hSnapsotParticle );


	// Get direction & stages of particle
	auto beamDirection = vecEnd - vecStart;
	std::vector<Vector_t> beamPositions = {
		vecStart,
		//vecStart + ( beamDirection * 0.3f ),
		vecStart + ( beamDirection * 0.5f ),
		vecEnd
	};

	for ( int i = 0; i < beamPositions.size( ); i++ )
	{
		// Create new particle info
		CParticleInformation particle_info{};
		particle_info.time = 2.f;
		particle_info.unk2 = .7f;
		particle_info.width = 1.1f;

		// create particle effect for this stage/position
		GetInterfaceManager( )->GetGameParticleManager( )->CreateParticleEffect( beamInfo.m_nEffectIndex, 3, &particle_info, 0 );

		// Init new position & time holders
		beamInfo.m_vPositions = new Vector_t[ i + 1 ];
		beamInfo.m_flTimes = new float[ i + 1 ];

		// update positions & times
		for ( int j{}; j < i + 1; j++ )
		{
			beamInfo.m_vPositions[ j ] = beamPositions[ j ];
			beamInfo.m_flTimes[ j ] = TICK_INTERVAL * float( j );
		}
		beamInfo.m_ParticleData.m_vecPosition = beamInfo.m_vPositions;
		beamInfo.m_ParticleData.m_flTimes2 = beamInfo.m_flTimes;

		// Draw particle
		GetInterfaceManager( )->GetParticleSystemManager( )->Draw( &beamInfo.m_hSnapsotParticle, i + 1, &beamInfo.m_ParticleData );

		// Clear positions & times
		delete[ ] beamInfo.m_vPositions;
		delete[ ] beamInfo.m_flTimes;
	}
}

auto CEffects::PlayHitSound( ) -> void
{
	if ( C_GET( int, Vars.iHitSound ) > -1 && C_GET( int, Vars.iHitSound ) < m_vecHitSounds.size( ) )
	{
		try
		{
			PlaySound( TEXT( m_vecHitSounds[ C_GET( int, Vars.iHitSound ) ].szPath.c_str( ) ), NULL, SND_FILENAME | SND_ASYNC );
		}
		catch ( ... )
		{
			C_GET( int, Vars.iHitSound ) = -1;
		}
	}
	else
	{
		C_GET( int, Vars.iHitSound ) = -1;
	}
}

auto CEffects::LoadSounds( ) -> void
{
	m_vecHitSounds.clear( );
	for ( const auto& entry : std::filesystem::recursive_directory_iterator( GetExplorerManager( )->GetHitSoundsDirectory( ) ) )
	{
		if ( GetExplorerManager( )->IsAudioByExtension( entry.path( ).string( ) ) )
		{
			HitSound_t hitSound;
			hitSound.szName = entry.path( ).stem( ).string( );
			hitSound.szPath = ( GetExplorerManager( )->GetHitSoundsDirectory( ) / entry.path( ) ).string( ); // Get relative path

			m_vecHitSounds.push_back( std::move( hitSound ) );
		}
	}
}

auto CEffects::GetHitSound( int iSoundIndex ) -> HitSound_t
{
	if ( iSoundIndex == -1 )
	{
		HitSound_t customSound;
		customSound.szName = "Disabled";
		return customSound;
	}

	if ( iSoundIndex < 0 || iSoundIndex >= m_vecHitSounds.size( ) )
		return HitSound_t( );

	return m_vecHitSounds[ iSoundIndex ];
}

auto GetEffects( ) -> CEffects*
{
	return &g_CEffects;
}

