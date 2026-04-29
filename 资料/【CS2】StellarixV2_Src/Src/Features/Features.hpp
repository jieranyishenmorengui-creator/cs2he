#pragma once

class CMeshDrawPrimitive;
class CViewSetup;
class CAggregateSceneObjectLighting;
class CUserCmd;
class CCSGOInput;
class CGameEventHelper;
class CGameEvent;
class CCSPlayerController;
class C_CSPlayerPawn;

class CFeatures
{
public:
	CFeatures( ) = default;
	~CFeatures( ) = default;

public:
	auto OnInit( ) -> bool;
	auto OnDestroy( ) -> void;

public:
	auto OnLevelInitHook( const char* szNewMap ) -> void;
	auto OnLevelShutdownHook( ) -> void;
	auto OnPresentHook( ) -> void;
	auto OnFrameStageNotifyHook( int nFrameStage ) -> void;
	auto OnCreateMovePredictHook( CCSGOInput* pCCSGOInput, CUserCmd* pUserCmd ) -> void;
	auto OnDrawArrayHook( void* pAnimatableSceneObjectDesc, void* pDx11, CMeshDrawPrimitive* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk ) -> void;
	auto OnPreOverrideViewHook( CViewSetup* pViewSetup ) -> void;
	auto OnPostOverrideViewHook( CViewSetup* pViewSetup ) -> void;
	auto OnSetModel( char* szModelName ) -> void;
	auto OnRenderLightingHook( CAggregateSceneObjectLighting* pCAggregateSceneObjectLighting ) -> void;

public:
	auto OnPlayerHurtEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController, CCSPlayerController* pAttackerController, C_CSPlayerPawn* pPlayerPawn, C_CSPlayerPawn* pAttackerPawn, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn ) -> void;
	auto OnBulletImpactEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController, C_CSPlayerPawn* pPlayerPawn ) -> void;

private:
	bool m_bInitialized = false;
};

auto GetFeatures( ) -> CFeatures*;