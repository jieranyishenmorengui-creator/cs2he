#include "Chams.hpp"
#include "Materials.hpp"
#include <Core/Config/Variables.hpp>
#include <Core/Engine/FuncrionCS2.hpp>
#include <Core/Hooks/List/Hook_DrawArray.hpp>
#include <Core/Interfaces/List/IMaterialSystem.hpp>
#include <Core/Sdk/Datatypes/Color.hpp>
#include <Core/Sdk/Datatypes/KeyValue3.hpp>
#include <Core/Sdk/Datatypes/StrongHandle.hpp>
#include <Core/Utils/PatternManager/PatternManager.hpp>
#include <Core/Engine/Convar/Convar.hpp>
#include <Core/Interfaces/List/IEngineCVar.hpp>


struct CustomMaterial_t
{
	CStrongHandle<CMaterial2> pVisibleMaterial;
	CStrongHandle<CMaterial2> pInvisibleMaterial;
};
static CustomMaterial_t arrMaterials[ kMaterialMax ];

static const FNV1A_t uGlowMaterialHash = FNV1A::HashConst( "materials/dev/glowproperty.vmat" );
static const FNV1A_t uC_CS2HudModelArmsHash = FNV1A::HashConst( "C_CS2HudModelArms" );
static const FNV1A_t uC_CS2HudModelWeaponHash = FNV1A::HashConst( "C_CS2HudModelWeapon" );


static CChams g_CChams{ };

auto CChams::OnInit( ) -> bool
{
	if ( m_bInitialized )
		return m_bInitialized;

	arrMaterials[ kMaterialIlluminate ] = CustomMaterial_t{
		.pVisibleMaterial = CreateMaterial( _xor( "materials/dev/illuminate.vmat" ), szVMatBufferIlluminateVisible ),
		.pInvisibleMaterial = CreateMaterial( _xor( "materials/dev/illuminate.vmat" ), szVMatBufferIlluminateInvisible )
	};

	arrMaterials[ kMaterialGlow ] = CustomMaterial_t{
		.pVisibleMaterial = CreateMaterial( _xor( "materials/dev/glowproperty.vmat" ), szVMatBufferGlowVisible ),
		.pInvisibleMaterial = CreateMaterial( _xor( "materials/dev/glowproperty.vmat" ), szVMatBufferGlowInisible )
	};

	arrMaterials[ kMaterialPrimary ] = CustomMaterial_t{
		.pVisibleMaterial = CreateMaterial( _xor( "materials/dev/primary_white.vmat" ), szVMatBufferWhiteVisible ),
		.pInvisibleMaterial = CreateMaterial( _xor( "materials/dev/primary_white.vmat" ), szVMatBufferWhiteInvisible )
	};

	arrMaterials[ kMaterialLatex ] = CustomMaterial_t{
		.pVisibleMaterial = CreateMaterial( _xor( "materials/dev/latex.vmat" ), szVMatBufferLatexVisible ),
		.pInvisibleMaterial = CreateMaterial( _xor( "materials/dev/latex.vmat" ), szVMatBufferLatexInvisible )
	};

	arrMaterials[ kMaterialMetalic ] = CustomMaterial_t{
		.pVisibleMaterial = CreateMaterial( _xor( "materials/dev/metalic.vmat" ), szVMatBufferMetallicVisible ),
		.pInvisibleMaterial = CreateMaterial( _xor( "materials/dev/metalic.vmat" ), szVMatBufferMetallicInvisible )
	};


	m_bInitialized = true;
	return m_bInitialized;
}

auto CChams::OnDestroy( ) -> void
{
	if ( !m_bInitialized )
		return;
	m_bInitialized = false;
}

auto CChams::GetEntityType( C_BaseEntity* pBaseEntity ) -> ECachedEntityType
{
	auto eEntityType = GetEntityCache( )->GetEntityType( pBaseEntity );
	if ( eEntityType == ECachedEntityType::kUnknown )
	{
		FNV1A_t uEntityClassHash = FNV1A::Hash( pBaseEntity->GetEntityClassName( ) );
		if ( uEntityClassHash == uC_CS2HudModelArmsHash )
			eEntityType = ECachedEntityType::kC_CS2HudModelArms;

		else if ( uEntityClassHash == uC_CS2HudModelWeaponHash )
			eEntityType = ECachedEntityType::kC_CS2HudModelWeapon;

		else if ( pBaseEntity->m_iTeamNum( ) != 0 )
			eEntityType = ECachedEntityType::kPlayer;
	}

	return eEntityType;
}

auto CChams::CreateMaterial( const char* szMaterialName, const char szVmatBuffer[ ] ) -> CStrongHandle<CMaterial2>
{
	CKeyValues3* pKeyValues3 = CKeyValues3::CreateMaterialResource( );
	pKeyValues3->LoadFromBuffer( szVmatBuffer );

	CStrongHandle<CMaterial2> pCustomMaterial = {};
	FunctionsCS2::FnCreateMaterial( nullptr, &pCustomMaterial, szMaterialName, pKeyValues3, 0, 1 );

	return pCustomMaterial;
}


auto CChams::OnDrawArrayHook( void* pAnimatableSceneObjectDesc, void* pDx11, CMeshDrawPrimitive* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk ) -> void
{
	if ( !m_bInitialized ) return;
	if ( !arrMeshDraw || !arrMeshDraw->m_pMaterial || !arrMeshDraw->m_pSceneAnimatableObject ) return;
	if ( !arrMeshDraw->m_pSceneAnimatableObject->m_hEntity.IsValid( ) ) return;
	if ( FNV1A::Hash( arrMeshDraw->m_pMaterial->GetName( ) ) == uGlowMaterialHash ) return;


	C_CSPlayerPawn* pLocalPawn = C_CSPlayerPawn::GetPawnFromController( CCSPlayerController::GetLocalPlayerController( ) );
	if ( !pLocalPawn )
		return;

	Color_t colorBackup = arrMeshDraw->m_Color;

	auto pBaseEntity = GetInterfaceManager( )->GetGameEntitySystem( )->Get( arrMeshDraw->m_pSceneAnimatableObject->m_hEntity );
	if ( !pBaseEntity ) return;

	auto eEntityType = GetEntityType( pBaseEntity );
	if ( eEntityType == ECachedEntityType::kUnknown )
		return;


	if ( eEntityType == ECachedEntityType::kPlayer )
	{
		auto pPlayerPawn = GetInterfaceManager( )->GetGameEntitySystem( )->Get<C_CSPlayerPawn>( arrMeshDraw->m_pSceneAnimatableObject->m_hEntity );
		if ( !pPlayerPawn )
			return;

		if ( pPlayerPawn == pLocalPawn ) // Local Player
		{
			if ( C_GET( int, Vars.eLocalPlayerChams ) != kMaterialDisabled )
			{
				const CustomMaterial_t customMaterial = arrMaterials[ C_GET( int, Vars.eLocalPlayerChams ) ];

				arrMeshDraw->m_pMaterial = customMaterial.pVisibleMaterial;
				//arrMeshDraw->m_pMaterialCopy = customMaterial.pVisibleMaterial;
				arrMeshDraw->m_Color = C_GET( Color_t, Vars.colLocalPlayerChamsColor );
			}
		}
		else if ( pPlayerPawn->IsEnemy( pLocalPawn ) ) // Enemy
		{
			if ( C_GET( int, Vars.eEnemyInvisiblePlayerChams ) != kMaterialDisabled )
			{
				const CustomMaterial_t customInvisibleMaterial = arrMaterials[ C_GET( int, Vars.eEnemyInvisiblePlayerChams ) ];

				arrMeshDraw->m_pMaterial = customInvisibleMaterial.pInvisibleMaterial;
				//arrMeshDraw->m_pMaterialCopy = customInvisibleMaterial.pVisibleMaterial;
				arrMeshDraw->m_Color = C_GET( Color_t, Vars.colEnemyInvisiblePlayerChamsColor );


				DrawArray_o.unsafe_fastcall<void*>( pAnimatableSceneObjectDesc, pDx11, arrMeshDraw, nDataCount, pSceneView, pSceneLayer, pUnk );


				//DrawModel_o.call<void*>(
				//	arrMeshDraw[4]/*0x100000000LL*/,            // Order of the model's material? 0x100000000LL vs 0 produces 2 different 'things'
				//	( __int64 )pDx11,            // DX11 context
				//	( __int64 )arrMeshDraw,          // Material Data
				//	( __int64 )arrMeshDraw->m_pMaterial,    // 'Visible' material?
				//	( __int64 )arrMeshDraw->m_pMaterial,    // 'Invisible' material?
				//	drawParams,            // Fuck if i know
				//	true,                // Doesn't change anything if true/false
				//	result,                //  Fuck if i know
				//	( __int64 )pSceneLayer        // I would imagine this is the scene layer (200iq) );
				//);
			}

			if ( C_GET( int, Vars.eEnemyVisiblePlayerChams ) != kMaterialDisabled )
			{
				const CustomMaterial_t customVisibleMaterial = arrMaterials[ C_GET( int, Vars.eEnemyVisiblePlayerChams ) ];

				arrMeshDraw->m_pMaterial = customVisibleMaterial.pVisibleMaterial;
				arrMeshDraw->m_Color = C_GET( Color_t, Vars.colEnemyVisiblePlayerChamsColor );
			}
		}
	}
	else if ( eEntityType == ECachedEntityType::kC_CS2HudModelArms )
	{
		if ( C_GET( int, Vars.eLocalViewModelChams ) != kMaterialDisabled )
		{
			const CustomMaterial_t customMaterial = arrMaterials[ C_GET( int, Vars.eLocalViewModelChams ) ];

			arrMeshDraw->m_pMaterial = customMaterial.pVisibleMaterial;
			//arrMeshDraw->m_pMaterialCopy = customMaterial.pVisibleMaterial;
			arrMeshDraw->m_Color = C_GET( Color_t, Vars.colLocalViewModelChamsColor );
		}
	}
	else if ( eEntityType == ECachedEntityType::kC_CS2HudModelWeapon )
	{
		if ( C_GET( int, Vars.eLocalWeaponChams ) != kMaterialDisabled )
		{
			const CustomMaterial_t customMaterial = arrMaterials[ C_GET( int, Vars.eLocalWeaponChams ) ];

			arrMeshDraw->m_pMaterial = customMaterial.pVisibleMaterial;
			//arrMeshDraw->m_pMaterialCopy = customMaterial.pVisibleMaterial;
			arrMeshDraw->m_Color = C_GET( Color_t, Vars.colLocalWeaponChamsColor );
		}
	}
	else if ( eEntityType == ECachedEntityType::kWeapon )
	{

	}
}

auto GetChams( ) -> CChams*
{
	return &g_CChams;
}
