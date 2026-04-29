#include "Rage.hpp"
#include <Core/Config/Variables.hpp>
#include <Core/Engine/Convar/Convar.hpp>
#include <Core/Interfaces/List/IEngineCVar.hpp>
#include <Core/Sdk/Datatypes/CUserCmd.hpp>
#include <Core/Sdk/EntityCache/EntityCache.hpp>


static CRage g_CRage{ };

static int iLimitEntityPerTick = 3;
auto CRage::OnCreateMovePredictHook( CCSGOInput* pInput, CUserCmd* pUserCmd ) -> void
{
	if ( ENABLE_RAGE == 0 || !C_GET( bool, Vars.bEnableRage ) )
		return;

	CCSPlayerController* pLocalController = CCSPlayerController::GetLocalPlayerController( );
	if ( !pLocalController )
		return;
	if ( !pLocalController->m_bPawnIsAlive( ) )
		return;

	RageConfig.flFov = C_GET( float, Vars.flFieldOfViewRage );
	UpdateAimHitboxes( RageConfig.vecAimHitboxes, C_GET_ARRAY( unsigned int, 50, Vars.nHitboxesRage, 0 ), RageConfig.nHitboxesRageBackup );
	UpdateAimHitboxes( RageConfig.vecAimMultipointHitboxes, C_GET_ARRAY( unsigned int, 50, Vars.nMultipointHitboxesRage, 0 ), RageConfig.nMultipointHitboxesRageBackup );


	auto& mapCachedEntities = GetEntityCache( )->LagCompCachedEntities->GetEntities( );
	if ( mapCachedEntities.empty( ) )
		return;

	GetEntityCache( )->LagCompCachedEntities->Lock( );
	for ( auto& pairEntity : mapCachedEntities )
	{
		CBaseHandle hCachedEntityHandle = pairEntity.first;
		CCachedEntities::VariantCachedEntities& variantCachedEntity = pairEntity.second.first;
		ECachedEntityType nCachedEntityType = pairEntity.second.second;

		if ( !hCachedEntityHandle.IsValid( ) )
			continue;

		if ( nCachedEntityType != ECachedEntityType::kPlayer )
			continue;

		CCachedPlayer* pCachedPlayer = std::get_if<CCachedPlayer>( &variantCachedEntity );
		if ( !pCachedPlayer )
			continue;

		pCachedPlayer->OnInit( );
		if ( !pCachedPlayer->IsValid( ) )
			continue;
	}
	GetEntityCache( )->LagCompCachedEntities->UnLock( );
}

auto CRage::UpdateAimHitboxes( std::vector<int>& vecHitboxes, unsigned int nSelected, unsigned int& nBackup ) -> void
{
	if ( nSelected == nBackup )
		return;

	vecHitboxes.clear( );
	nBackup = nSelected;

	if ( nSelected & kAimHitboxesHead )
		vecHitboxes.push_back( HITBOX_HEAD );

	if ( nSelected & kAimHitboxesChest )
	{
		vecHitboxes.push_back( HITBOX_CHEST );
		vecHitboxes.push_back( HITBOX_LOWER_CHEST );
		vecHitboxes.push_back( HITBOX_UPPER_CHEST );
	}

	if ( nSelected & kAimHitboxesStomach )
		vecHitboxes.push_back( HITBOX_STOMACH );

	if ( nSelected & kAimHitboxesCenter )
		vecHitboxes.push_back( HITBOX_NECK );

	if ( nSelected & kAimHitboxesPelvis )
		vecHitboxes.push_back( HITBOX_PELVIS );

	if ( nSelected & kAimHitboxesHands )
	{
		vecHitboxes.push_back( HITBOX_RIGHT_HAND );
		vecHitboxes.push_back( HITBOX_LEFT_HAND );
	}

	if ( nSelected & kAimHitboxesLegs )
	{
		vecHitboxes.push_back( HITBOX_RIGHT_FOOT );
		vecHitboxes.push_back( HITBOX_LEFT_FOOT );
	}
}

auto GetRage( ) -> CRage*
{
	return &g_CRage;
}
