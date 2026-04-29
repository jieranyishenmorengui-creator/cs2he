#pragma once
#include <Core/Sdk/EntityData.hpp>


class CLagCompRecord
{
public:
	CLagCompRecord( ) = default;
	CLagCompRecord( C_CSPlayerPawn* pC_CSPlayerPawn )
	{
		if ( !pC_CSPlayerPawn || !pC_CSPlayerPawn->m_pGameSceneNode( ) )
			return;

		m_pCGameSceneNode = pC_CSPlayerPawn->m_pGameSceneNode( );
		SaveData( pC_CSPlayerPawn );
	}

public:
	__forceinline auto SaveData( C_CSPlayerPawn* pC_CSPlayerPawn ) -> void
	{
		if ( !pC_CSPlayerPawn )
			return;

		if ( !m_pCGameSceneNode )
			return;

		auto pCollision = pC_CSPlayerPawn->m_pCollision( );
		if ( !pCollision )
			return;

		CSkeletonInstance* pSkeletonInstance = m_pCGameSceneNode->GetSkeletonInstance( );
		if ( !pSkeletonInstance )
			return;

		iBoneCount = pSkeletonInstance->nBoneCount;
		flSimulationTime = pC_CSPlayerPawn->m_flSimulationTime( );

		VecMins = pCollision->m_vecMins( );
		VecMaxs = pCollision->m_vecMaxs( );
		VecVelocity = pC_CSPlayerPawn->m_vecVelocity( );
		VecOrigin = pC_CSPlayerPawn->GetOrigin( );

		std::memmove( BoneData, pSkeletonInstance->pBoneCache, sizeof( Matrix2x4_t ) * iBoneCount );

		bValidRecord = true;
	}

private:
	CGameSceneNode* m_pCGameSceneNode = nullptr;

public:
	Matrix2x4_t BoneData[ 128 ];
	Matrix2x4_t BoneDataBackup[ 128 ];

	int iBoneCount = 0;
	float flSimulationTime = 0.f;
	Vector_t VecMins{};
	Vector_t VecMaxs{};
	Vector_t VecVelocity{};
	Vector_t VecOrigin{};

	bool bValidRecord = false;
};