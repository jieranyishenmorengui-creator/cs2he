#include "PatternManager.hpp"
#include <Core/Memory/Memory.hpp>
#include <Core/Utils/Logger/Logger.hpp>
#include <Core/Interfaces/InterfaceManager.hpp>
#include <Core/Interfaces/List/ISwapChain.hpp>


static CPatternManager g_CPatternManager{ };

auto CBasePattern::Search( ) -> bool
{
	pFoundAddress = 0x0;

	if ( szPattern.empty( ) && pObjectPtr )
		pFoundAddress = reinterpret_cast< uint8_t* >( GetMemoryManager( )->GetVFunc( pObjectPtr, nVTableIndex ) );
	else if ( !szPattern.empty( ) )
		pFoundAddress = GetMemoryManager( )->FindPattern( wszModuleName, szPattern.c_str( ) );

	if ( !pFoundAddress )
	{
		L_PRINT( LOG_ERROR ) << _xor( "Failed To Find Pattern: [" ) << szHookName.c_str( ) << _xor( "]" );
		return false;
	}
	L_PRINT( LOG_INFO ) << LOGGING::SetColor( LOG_COLOR_FORE_YELLOW ) << _xor( "Find Pattern: [" ) << szHookName.c_str( ) << _xor( "] -> " ) << reinterpret_cast< int >( pFoundAddress );


	if ( nRVAOffset != 0x0 || nRIPOffset != 0x0 )
		pFoundAddress = GetMemoryManager( )->ResolveRelativeAddress( pFoundAddress, nRVAOffset, nRIPOffset );

	pFoundAddress += nOffset;
	return true;
}

auto CPatternManager::OnInit( ) -> bool
{
	std::vector<CBasePattern> vecFirstPatterns =
	{
		{ _xor( "Interface::ISwapChainDx11" ), _xor( "48 89 2D ? ? ? ? 48 89 2D ? ? ? ? 48 C7 05 ? ? ? ? ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 89 2D" ), RENDERSYSTEM_DLL, 0x8, 0x3, 0x7 },
		{ _xor( "Interface::CGameEntitySystem" ), _xor( "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B F8 48 85 C0" ), CLIENT_DLL, 0x0, 0x3, 0x7 },
		{ _xor( "Interface::IGlobalVars" ), _xor( "48 89 15 ? ? ? ? 48 89 42" ), CLIENT_DLL, 0x0, 0x3, 0x7 }
	};
	if ( !InitializePatterns( vecFirstPatterns ) )
		return false;


	// Initialize `GetInterfaceManager( )->pDevice` and `pDXGISwapChain`
	GetInterfaceManager( )->GetSwapChain( );

	IDXGIDevice* pDXGIDevice = NULL;
	GetInterfaceManager( )->pDevice->QueryInterface( IID_PPV_ARGS( &pDXGIDevice ) );
	IDXGIAdapter* pDXGIAdapter = NULL;
	pDXGIDevice->GetAdapter( &pDXGIAdapter );
	IDXGIFactory* pIDXGIFactory = NULL;
	pDXGIAdapter->GetParent( IID_PPV_ARGS( &pIDXGIFactory ) );


	std::vector<CBasePattern> vecSecondPatterns =
	{
		{ _xor( "Hook::Present" ), GetInterfaceManager( )->GetSwapChain( )->pDXGISwapChain, EVtable::kPresent },
		{ _xor( "Hook::ResizeBuffers" ), GetInterfaceManager( )->GetSwapChain( )->pDXGISwapChain, EVtable::kResizeBuffers },
		{ _xor( "Hook::CreateSwapChain" ), pIDXGIFactory, EVtable::kCreateSwapChain },
		{ _xor( "Hook::MouseInputEnabled" ), _xor( "40 53 48 83 EC 20 80 B9 ? ? ? ? ? 48 8B D9 75 78" ), CLIENT_DLL },
		{ _xor( "Hook::IsRelativeMouseMode" ), GetInterfaceManager( )->GetInputSystem( ), EVtable::kIsRelativeMouseMode }, // _xor( "48 89 5C 24 20 56 48" )

		{ _xor( "Hook::CreateMoveClient" ), _xor( "85 D2 0F 85 ? ? ? ? 48 8B C4 44 88 40" ), CLIENT_DLL },
		{ _xor( "Hook::CreateMovePredict" ), _xor( "48 8B C4 4C 89 40 ? 48 89 48 ? 55 53 41 54" ), CLIENT_DLL },
		{ _xor( "Hook::DrawArray" ), _xor( "48 8B C4 53 57 41 54" ), SCENESYSTEM_DLL },
		{ _xor( "Hook::DrawAggregateSceneObjectArray" ), _xor( "48 8B C4 48 89 50 ? 48 89 48 ? 55 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 48 89 58 ? 48 89 70" ), SCENESYSTEM_DLL },
		//{ _xor( "Hook::DrawModel" ), _xor( "4C 89 4C 24 ? 4C 89 44 24 ? 48 89 4C 24 ? 53 55 56 57 41 54 41 56" ), SCENESYSTEM_DLL },
		{ _xor( "Hook::FrameStageNotify" ), _xor( "48 89 5C 24 ? 57 48 81 EC ? ? ? ? 48 8B F9 8B DA" ), CLIENT_DLL },
		{ _xor( "Hook::GetMatrixForView" ), _xor( "40 53 48 81 EC ? ? ? ? 49 8B C1" ), CLIENT_DLL },
		{ _xor( "Hook::OverrideView" ), _xor( "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC ? 48 8B FA E8" ), CLIENT_DLL },
		{ _xor( "Hook::ValidateCamera" ), _xor( "85 D2 0F 85 ? ? ? ? 48 89 5C 24 ? 56" ), CLIENT_DLL },
		{ _xor( "Hook::SetupViewModel" ), _xor( "40 55 53 56 41 56 41 57 48 8B EC" ), CLIENT_DLL },
		{ _xor( "Hook::CalculateFov" ), _xor( "40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 85 C0 74 ? 48 8B C8 48 83 C4" ), CLIENT_DLL },

		{ _xor( "Hook::OnAddEntity" ), _xor( "48 89 74 24 10 57 48 83 EC 20 41 B9 FF 7F 00 00 41 8B C0 41 23 C1 48 8B F2 41 83 F8 FF 48 8B F9 44 0F 45 C8 41 81 F9 00 40 00 00 73 0D" ), CLIENT_DLL },
		{ _xor( "Hook::OnRemoveEntity" ), _xor( "48 89 74 24 10 57 48 83 EC 20 41 B9 FF 7F 00 00 41 8B C0 41 23 C1 48 8B F2 41 83 F8 FF 48 8B F9 44 0F 45 C8 41 81 F9 00 40 00 00 73 08" ), CLIENT_DLL },
		{ _xor( "Hook::OnLevelInit" ), _xor( "40 55 56 41 56 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 0D" ), CLIENT_DLL },
		{ _xor( "Hook::OnLevelShutdown" ), _xor( "48 83 EC ? 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? 45 33 C9 45 33 C0 48 8B 01 FF 50 ? 48 85 C0 74 ? 48 8B 0D ? ? ? ? 48 8B D0 4C 8B 01 41 FF 50 ? 48 83 C4" ), CLIENT_DLL },
		//{ _xor( "Hook::OnDisconnect" ), _xor( "48 89 5C 24 ? 57 48 83 EC ? 8B DA 48 8D 4C 24 ? 33 D2" ), CLIENT_DLL },
		
		{ _xor( "Hook::GetLocalPlayerController" ), _xor( "48 83 EC ? 83 F9 ? 75 ? 48 8B 0D ? ? ? ? 48 8D 54 24 ? 48 8B 01 FF 90 ? ? ? ? 8B 08 48 63 C1 48 8D 0D ? ? ? ? 48 8B 04 C1 48 83 C4 ? C3 CC CC CC CC CC CC CC CC CC CC CC CC CC 48 83 EC ? 83 F9" ), CLIENT_DLL },
		{ _xor( "Hook::GetLocalPlayerPawn" ), _xor( "40 53 48 83 EC ? 33 C9 E8 ? ? ? ? 48 8B D8 48 85 C0 74 ? 48 8B 00 48 8B CB FF 90 ? ? ? ? 84 C0 74 ? 48 8B C3" ), CLIENT_DLL },
		{ _xor( "Hook::GetBaseEntity" ), _xor( "4C 8D 49 10 81 FA ?? ?? 00 00 77 ?? 8B CA C1 F9 09" ), CLIENT_DLL },
		{ _xor( "Hook::ComputeHitboxSurroundingBox" ), _xor( "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B EA" ), CLIENT_DLL },
		{ _xor( "Hook::SetModel" ), _xor( "40 53 48 83 EC 20 48 8B D9 4C 8B C2 48 8B 0D ? ? ? ? 48 8D 54 24" ), CLIENT_DLL },
		{ _xor( "Hook::RenderLegs" ), _xor( "40 55 53 56 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? F2 0F 10 42" ), CLIENT_DLL },  // @IDA STR: "FirstpersonLegsPrepass"
		{ _xor( "Hook::RenderLighting" ), _xor( "48 89 5C 24 ? 48 89 6C 24 ? 48 89 54 24" ), SCENESYSTEM_DLL },
		{ _xor( "Hook::RenderScopeOverlay" ), _xor( "48 8B C4 53 57 48 83 EC ? 48 8B FA" ), CLIENT_DLL },
		{ _xor( "Hook::RenderCrosshair" ), _xor( "48 89 5C 24 ? 57 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 85 C0 0F 84" ), CLIENT_DLL },
		{ _xor( "Hook::DrawTeamIntro" ), _xor( "48 83 EC ? 45 0F B6 08" ), CLIENT_DLL },

		{ _xor( "Hook::CreateMaterial" ), _xor( "48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 81 EC ? ? ? ? 48 8B 05" ), MATERIAL_SYSTEM2_DLL },
		{ _xor( "Hook::LoadKeyValues" ), _xor( "48 89 5C 24 ? 48 89 6C 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 45 33 E4" ), TIER0_DLL },
		{ _xor( "Hook::SetTypeKV3" ), _xor( "40 53 48 83 EC ? 4C 8B 11 41 B9" ), CLIENT_DLL },
		{ _xor( "CCSGOInput::GetViewAngle" ), _xor( "4C 8B C1 85 D2 74 08 48 8D 05 ? ? ? ? C3" ), CLIENT_DLL },
		{ _xor( "CCSGOInput::SetViewAngle" ), _xor( "85 D2 75 ? 48 63 81" ), CLIENT_DLL },
		{ _xor( "Hook::PopupEventHandle" ), _xor( "40 56 57 41 57 48 83 EC ? 48 8B 3D ? ? ? ? 4D 85 C0" ), CLIENT_DLL },
		{ _xor( "Hook::SetReadyPlayer" ), _xor( "40 53 48 83 EC ? 48 8B DA 48 8D 15 ? ? ? ? 48 8B CB FF 15" ), CLIENT_DLL },
		{ _xor( "Hook::TestFunc" ), _xor( "40 55 53 48 8D 6C 24 ? 48 81 EC ? ? ? ? 80 B9" ), CLIENT_DLL },
		{ _xor( "Hook::ShowMessageBox" ), _xor( "44 88 4C 24 ? 53 41 56" ), CLIENT_DLL },
		{ _xor( "Hook::ScreenTransform" ), _xor( "48 89 74 24 ? 57 48 83 EC ? 48 8B 05 ? ? ? ? 48 8B FA" ), CLIENT_DLL },

		{ _xor( "IEngineClient::IsInGame" ), _xor( "48 8B ? ? ? ? ? 48 85 C0 74 15 80 B8 ? ? ? ? ? 75 0C 83 B8 ? ? ? ? 06" ), ENGINE2_DLL },
		{ _xor( "IEngineClient::IsConnected" ), _xor( "48 8B 05 ? ? ? ? 48 85 C0 74 ? 80 B8 ? ? ? ? ? 75 ? 83 B8 ? ? ? ? ? 7C" ), ENGINE2_DLL },

		{ _xor( "Hook::GetEyePosition" ), _xor( "48 89 5C 24 ? 56 48 83 EC ? 48 8B D9 48 8B F2 48 8B 89" ), CLIENT_DLL },
		{ _xor( "Hook::GetActiveWeapon" ), _xor( "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B 59 ? 48 8B F9 48 8B CB" ), CLIENT_DLL },
		{ _xor( "Hook::RunCommand" ), _xor( "48 8B C4 48 81 EC ? ? ? ? 48 89 58 ? 48 89 68 ? 48 8B EA 48 89 70 ? 48 8B F1" ), CLIENT_DLL },
		{ _xor( "Hook::GetLocalPlayerIndex" ), _xor( "40 53 48 83 EC ? 48 8B 05 ? ? ? ? 48 8D 0D ? ? ? ? 48 8B DA FF 90 ? ? ? ? 48 8B C3 48 83 C4 ? 5B C3 CC CC CC CC CC CC CC CC CC CC 48 8B 05" ), ENGINE2_DLL },
		{ _xor( "Hook::MarkInterpolationLatchFlagsDirty" ), _xor( "40 53 56 57 48 83 EC ? 80 3D" ), CLIENT_DLL },

		{ _xor( "Hook::GetGameParticleSystem" ), _xor( "48 8B 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 40 53" ), CLIENT_DLL },
		{ _xor( "Hook::CacheParticleEffect" ), _xor( "4C 8B DC 53 48 81 EC ? ? ? ? F2 0F 10 05" ), CLIENT_DLL },
		{ _xor( "Hook::CreateParticleEffect" ), _xor( "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? F3 0F 10 1D ? ? ? ? 41 8B F8 8B DA 4C 8D 05" ), CLIENT_DLL },
		{ _xor( "Hook::UnknownParticleFunction" ), _xor( "48 89 74 24 ? 57 48 83 EC ? 4C 8B D9 49 8B F9 33 C9 41 8B F0 83 FA ? 0F 84" ), CLIENT_DLL },

		{ _xor( "CMaterial2::FindParameter" ), _xor( "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B 59 ? 48 8B F2 48 63 79 ? 48 C1 E7 ? 48 03 FB 48 3B DF" ), MATERIAL_SYSTEM2_DLL },
		{ _xor( "CMaterial2::UpdateParameter" ), _xor( "48 89 7C 24 ? 41 56 48 83 EC ? 8B 81" ), MATERIAL_SYSTEM2_DLL }
	};
	m_bInitialized = InitializePatterns( vecSecondPatterns );
	return m_bInitialized;
}

auto CPatternManager::OnDestroy( ) -> void
{
	if ( !m_bInitialized )
		return;
	m_bInitialized = false;
}

auto CPatternManager::InitializePatterns( std::vector<CBasePattern> vecPatterns_ ) -> bool
{
	bool bResult = true;

	for ( auto& basePattern : vecPatterns_ )
	{
		bResult &= basePattern.Search( );
		m_vecPatterns.push_back( basePattern );
	}

	return bResult;
}

auto CPatternManager::GetPattern( const FNV1A_t uNameHash ) -> CBasePattern
{
	auto it = std::find_if( m_vecPatterns.begin( ), m_vecPatterns.end( ),
							[&]( const CBasePattern& e )
	{
		return FNV1A::Hash( e.szHookName.c_str( ) ) == uNameHash;
	} );

	if ( it != m_vecPatterns.end( ) )
		return *it;
	else
		L_PRINT( LOG_WARNING ) << _xor( "Failed To Get Pattern: [" ) << uNameHash << _xor( "]" );

	return CBasePattern( );
}

auto GetPatternManager( ) -> CPatternManager*
{
	return &g_CPatternManager;
}
