#pragma once
#include <Core/Renderer/Renderer.hpp>
#include <ImGui/imgui.h>
#include <string>


class CGameEventHelper;
class CGameEvent;
class CCSPlayerController;
class C_CSPlayerPawn;
class CMeshDrawPrimitive;

class CVisuals
{
public:
	CVisuals( ) = default;
	~CVisuals( ) = default;

public:
	auto OnInit( ) -> bool;
	auto OnDestroy( ) -> void;

public:
	auto OnLevelInitHook( const char* szNewMap ) -> void;
	auto OnLevelShutdownHook( ) -> void;
	auto OnPresentHook( ImDrawList* pBackgroundDrawList ) -> void;
	auto OnDrawArrayHook( void* pAnimatableSceneObjectDesc, void* pDx11, CMeshDrawPrimitive* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk ) -> void;


public:
	auto OnPlayerHurtEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController, CCSPlayerController* pAttackerController, C_CSPlayerPawn* pPlayerPawn, C_CSPlayerPawn* pAttackerPawn, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn ) -> void;
	auto OnBulletImpactEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController, C_CSPlayerPawn* pPlayerPawn ) -> void;

private:
	bool m_bInitialized = false;
};

auto GetVisuals( ) -> CVisuals*;