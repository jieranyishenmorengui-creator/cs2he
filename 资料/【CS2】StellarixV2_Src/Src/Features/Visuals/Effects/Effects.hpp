#pragma once
#include <Core/Renderer/Renderer.hpp>
#include <ImGui/imgui.h>
#include <string>
#include <vector>
#include <Core/Utils/Animations/AnimationHandler.hpp>


class CGameEventHelper;
class CGameEvent;
class CCSPlayerController;
class C_CSPlayerPawn;

struct Vector_t;
struct Color_t;


struct HitSound_t
{
	std::string szName = "";
	std::string szPath = "";
};

struct DamageHitMarker_t
{
	constexpr DamageHitMarker_t( const Vector_t vecPosition_, const int iDamage_, const float flCreateTime_ ) :
		vecPosition( vecPosition_ ), iDamage( iDamage_ ), flCreateTime( flCreateTime_ )
	{
	}

	Vector_t vecPosition;
	int iDamage;
	float flCreateTime;
};

class CEffects
{
public:
	CEffects( ) = default;
	~CEffects( ) = default;

public:
	auto OnInit( ) -> bool;
	auto OnDestroy( ) -> void;

public:
	auto OnLevelInitHook( const char* szNewMap ) -> void;
	auto OnLevelShutdownHook( ) -> void;
	auto OnPresentHook( ImDrawList* pBackgroundDrawList ) -> void;

public:
	auto RenderHitMarkers( ImDrawList* pBackgroundDrawList ) -> void;
	auto RenderPlayerTrail( ImDrawList* pBackgroundDrawList ) -> void;

public:
	auto OnPlayerHurtEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController, CCSPlayerController* pAttackerController, C_CSPlayerPawn* pPlayerPawn, C_CSPlayerPawn* pAttackerPawn, CCSPlayerController* pLocalController, C_CSPlayerPawn* pLocalPawn ) -> void;
	auto OnPlayerDeathEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController ) -> void;
	auto OnBulletImpactEvent( CGameEventHelper gameEventHelper, CGameEvent* event, CCSPlayerController* pPlayerController, C_CSPlayerPawn* pPlayerPawn ) -> void;

public:
	auto DrawBeam( Vector_t vecStart, Vector_t vecEnd, Color_t color ) -> void;

public:
	auto PlayHitSound( ) -> void;
	auto LoadSounds( ) -> void;
	auto GetHitSound( int iSoundIndex ) -> HitSound_t;
	auto GetSoundsCount( ) -> int
	{
		return m_vecHitSounds.size( );
	}

private:
	float m_flCrosshairMarkTime = -1.f;
	const float m_flHitMarkerLifeTime = 1.f;
	const float m_flDamageHitMarkerLifeTime = 2.2f;
	const float m_flHitHealthBoostLifeTime = 1.2f;
	std::vector<DamageHitMarker_t> m_vecDamageHitMarkers{ };

private:
	AnimationHandler_t animCrosshairHitmark;

private:
	std::vector<HitSound_t> m_vecHitSounds{ };

private:
	bool m_bInitialized = false;
};

auto GetEffects( ) -> CEffects*;