#include "Features.hpp"
#include "Visuals/Visuals.hpp"
#include <Core/Config/Variables.hpp>
#include <Core/Renderer/Renderer.hpp>
#include <Core/Sdk/Const.hpp>
#include <Core/Sdk/Datatypes/CGameEvents.hpp>
#include <Core/Sdk/Datatypes/CUserCmd.hpp>
#include <Core/Sdk/EntityCache/EntityCache.hpp>
#include <Core/Utils/Logger/Logger.hpp>
#include "Rage/Rage.hpp"
#include "AntiAim/AntiAim.hpp"
#include "Misc/Misc.hpp"
#include "Misc/Movement/Movement.hpp"


static CFeatures g_CFeatures{ };

auto CFeatures::OnInit( ) -> bool
{
	if ( !GetVisuals( )->OnInit( ) )
		return false;
	L_PRINT( LOG_INFO ) << _xor( "Visuals Initialized" );

	if ( !GetMisc( )->OnInit( ) )
		return false;
	L_PRINT( LOG_INFO ) << _xor( "Misc Initialized" );

	m_bInitialized = true;
	return m_bInitialized;
}

auto CFeatures::OnDestroy( ) -> void
{
	if ( !m_bInitialized )
		return;
	m_bInitialized = false;

	GetVisuals( )->OnDestroy( );
}

auto CFeatures::OnLevelInitHook( const char* szNewMap ) -> void
{
	GetVisuals( )->OnLevelInitHook( szNewMap );
	GetMisc( )->OnLevelInitHook( szNewMap );
}

auto CFeatures::OnLevelShutdownHook( ) -> void
{
	GetVisuals( )->OnLevelShutdownHook( );
	GetMisc( )->OnLevelShutdownHook( );
}

auto CFeatures::OnPresentHook( ) -> void
{
	if ( !m_bInitialized )
		return;

	ImDrawList* pBackgroundDrawList = ImGui::GetBackgroundDrawList( );
	if ( pBackgroundDrawList )
	{
		GetVisuals( )->OnPresentHook( pBackgroundDrawList );
	}
}

auto CFeatures::OnFrameStageNotifyHook( int nFrameStage ) -> void
{
	if ( !m_bInitialized )
		return;

	GetMisc( )->OnFrameStageNotifyHook( nFrameStage );
}

auto CFeatures::OnCreateMovePredictHook( CCSGOInput* pCCSGOInput, CUserCmd* pUserCmd ) -> void
{
	GetAntiAim( )->OnCreateMovePredictHook( pCCSGOInput, pUserCmd );
	GetRage( )->OnCreateMovePredictHook( pCCSGOInput, pUserCmd );

	GetMovement( )->FixCmdButtons( pUserCmd, pUserCmd ? pUserCmd->csgoUserCmd.mutable_base( ) : nullptr );
	GetMovement( )->MovementCorrect( pUserCmd ? pUserCmd->csgoUserCmd.mutable_base( ) : nullptr, LocalPlayerData.angViewAngle );
}

auto CFeatures::OnDrawArrayHook( void* pAnimatableSceneObjectDesc, void* pDx11, CMeshDrawPrimitive* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk ) -> void
{
	if ( !m_bInitialized )
		return;

	GetVisuals( )->OnDrawArrayHook( pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk );
}

auto CFeatures::OnPreOverrideViewHook( CViewSetup* pViewSetup ) -> void
{
	if ( !m_bInitialized )
		return;

	GetMisc( )->OnPreOverrideViewHook( pViewSetup );
}

auto CFeatures::OnPostOverrideViewHook( CViewSetup* pViewSetup ) -> void
{
	if ( !m_bInitialized )
		return;

	GetMisc( )->OnPostOverrideViewHook( pViewSetup );
}

auto CFeatures::OnSetModel( char* szModelName ) -> void
{
	if ( !m_bInitialized )
		return;
}

auto CFeatures::OnRenderLightingHook( CAggregateSceneObjectLighting* pCAggregateSceneObjectLighting ) -> void
{
	if ( !m_bInitialized )
		return;
}


auto CFeatures::OnPlayerHurtEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController, CCSPlayerController* pAttackerController, C_CSPlayerPawn* pPlayerPawn, C_CSPlayerPawn* pAttackerPawn, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn ) -> void
{
	GetVisuals( )->OnPlayerHurtEvent( gameEventHelper, event, pPlayerController, pAttackerController, pPlayerPawn, pAttackerPawn, pLocalController, pLocalPawn );
}

auto CFeatures::OnBulletImpactEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController, C_CSPlayerPawn* pPlayerPawn ) -> void
{
	GetVisuals( )->OnBulletImpactEvent( gameEventHelper, event, pPlayerController, pPlayerPawn );
}

auto GetFeatures( ) -> CFeatures*
{
	return &g_CFeatures;
}
