#include "Visuals.hpp"
#include <Core/Sdk/Datatypes/CGameEvents.hpp>
#include <Core/Sdk/EntityCache/EntityCache.hpp>
#include "Effects/Effects.hpp"
#include "Chams/Chams.hpp"
#include "Esp/Esp.hpp"

static CVisuals g_CVisuals{};

auto CVisuals::OnInit( ) -> bool
{
	if ( !GetEsp( )->OnInit( ) )
		return false;
	L_PRINT( LOG_INFO ) << _xor( "Esp Initialized" );

	if ( !GetEffects( )->OnInit( ) )
		return false;
	L_PRINT( LOG_INFO ) << _xor( "Effects Initialized" );

	if ( !GetChams( )->OnInit( ) )
		return false;
	L_PRINT( LOG_INFO ) << _xor( "Chams Initialized" );

	m_bInitialized = true;
	return m_bInitialized;
}

auto CVisuals::OnDestroy( ) -> void
{
	if ( !m_bInitialized )
		return;
	m_bInitialized = false;


}

auto CVisuals::OnLevelInitHook( const char* szNewMap ) -> void
{
	GetEffects( )->OnLevelInitHook( szNewMap );
}

auto CVisuals::OnLevelShutdownHook( ) -> void
{
	GetEffects( )->OnLevelShutdownHook( );
}

auto CVisuals::OnPresentHook( ImDrawList* pBackgroundDrawList ) -> void
{
	GetEsp( )->OnPresentHook( pBackgroundDrawList );
	GetEffects( )->OnPresentHook( pBackgroundDrawList );
}

auto CVisuals::OnDrawArrayHook( void* pAnimatableSceneObjectDesc, void* pDx11, CMeshDrawPrimitive* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk ) -> void
{
	GetChams( )->OnDrawArrayHook( pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk );
}


auto CVisuals::OnPlayerHurtEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController, CCSPlayerController* pAttackerController, C_CSPlayerPawn* pPlayerPawn, C_CSPlayerPawn* pAttackerPawn, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn ) -> void
{
	GetEffects( )->OnPlayerHurtEvent( gameEventHelper, event, pPlayerController, pAttackerController, pPlayerPawn, pAttackerPawn, pLocalController, pLocalPawn );
}

auto CVisuals::OnBulletImpactEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController, C_CSPlayerPawn* pPlayerPawn ) -> void
{
	GetEffects( )->OnBulletImpactEvent( gameEventHelper, event, pPlayerController, pPlayerPawn );
}

auto GetVisuals( ) -> CVisuals*
{
	return &g_CVisuals;
}
