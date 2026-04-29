#include "ModelChanger.hpp"
#include <Core/Common.hpp>
#include <Core/Config/Variables.hpp>
#include <Core/Hooks/List/Hook_FrameStageNotify.hpp>
#include <Core/Interfaces/List/IEngineClient.hpp>
#include <Core/Interfaces/List/IResourceSystem.hpp>
#include <Core/Sdk/EntityData.hpp>
#include <Core/Utils/ExplorerManager/ExplorerManager.hpp>
#include <Core/Utils/Logger/Logger.hpp>
#include <Fnv1a/Fnv1a.h>
#include <shared_mutex>
#include <Core/Sdk/Globals.hpp>


static CModelChanger g_CModelChanger{ };

auto CModelChanger::OnInit( ) -> bool
{
	std::filesystem::create_directory( std::filesystem::path( GetExplorerManager( )->GetCustomModelsDirectory( ).string( ) + _xor( "\\" ) + _xor( "models" ) ) );
	if ( !std::filesystem::exists( GetExplorerManager( )->GetCustomModelsDirectory( ) ) || !std::filesystem::is_directory( GetExplorerManager( )->GetCustomModelsDirectory( ) ) )
	{
		L_PRINT( LOG_ERROR ) << _xor( "Invalid Custom Models Folder: " ) << GetExplorerManager( )->GetCustomModelsDirectory( ).c_str( );
		return false;
	}

	auto bResult = LoadCustomModels( );

	bNeedSetModel = true;
	return bResult;
}

auto CModelChanger::OnDestroy( ) -> void
{
	std::lock_guard lock( m_Lock );
	m_vecAgentModels.clear( );
}

auto CModelChanger::OnLevelInitHook( const char* szNewMap ) -> void
{
	bNeedSetModel = true;
}

auto CModelChanger::OnLevelShutdownHook( ) -> void
{
	bNeedSetModel = true;
}

auto CModelChanger::OnFrameStageNotifyHook( int nFrameStage ) -> void
{
	if ( !bNeedSetModel )
		return;

	if ( nFrameStage != FRAME_RENDER_END )
		return;

	C_CSPlayerPawn* pLocalPawn = C_CSPlayerPawn::GetPawnFromController( CCSPlayerController::GetLocalPlayerController( ) );
	ChangeModel( pLocalPawn );
}

auto CModelChanger::OnSetModel( char* szModelName ) -> bool
{
	return true;
}

auto CModelChanger::OnChangeModelInMenu( ) -> void
{
	bNeedSetModel = true;
}

auto CModelChanger::ChangeModel( C_BaseModelEntity* pC_BaseModelEntity ) -> void
{
	if ( !pC_BaseModelEntity )
		return;

	if ( C_GET( int, Vars.iCustomAgent ) == -1 )
		return;

	CustomModel_t selectedModel = GetAgentModel( C_GET( int, Vars.iCustomAgent ) );
	if ( !pC_BaseModelEntity->m_pGameSceneNode( ) )
		return;

	if ( !pC_BaseModelEntity->m_pGameSceneNode( )->GetSkeletonInstance( ) )
		return;

	GetInterfaceManager( )->GetResourceSystem( )->PrecacheResource( selectedModel.szPath.c_str( ) );
	pC_BaseModelEntity->SetModel( selectedModel.szPath.c_str( ) );

	bNeedSetModel = false;
}

auto CModelChanger::LoadCustomModels( ) -> bool
{
	std::lock_guard lock( m_Lock );

	m_vecAgentModels.clear( );
	// Loop through sub-directories and load all vmdl_c files.
	for ( const auto& entry : std::filesystem::recursive_directory_iterator( GetExplorerManager( )->GetCustomModelsDirectory( ) ) )
	{
		if ( entry.is_regular_file( ) && entry.path( ).extension( ) == _xor( ".vmdl_c" ) )
		{
			CustomModel_t CustomModel;
			CustomModel.szName = FormatName( entry.path( ).stem( ).string( ) ); // Get file name without extension
			CustomModel.szPath = std::filesystem::relative( entry.path( ), GetExplorerManager( )->GetCustomModelsDirectory( ).parent_path( ) ).string( ); // Get relative path

			// Replace ".vmdl_c" with ".vmdl"
			if ( CustomModel.szPath.size( ) >= 7 && CustomModel.szPath.substr( CustomModel.szPath.size( ) - 7 ) == _xor( ".vmdl_c" ) )
			{
				CustomModel.szPath.replace( CustomModel.szPath.size( ) - 7, 7, _xor( ".vmdl" ) );
			}

			L_PRINT( LOG_INFO ) << _xor( "Model: " ) << CustomModel.szName.c_str( ) << _xor( " - " ) << CustomModel.szPath.c_str( );
			m_vecAgentModels.push_back( std::move( CustomModel ) );
		}
	}

	if ( m_vecAgentModels.empty( ) )
		L_PRINT( LOG_WARNING ) << _xor( "No custom models found! Custom models need to be placed in 'Counter-Strike Global Offensive\\game\\csgo\\characters\\models'" );

	return true;
}

auto CModelChanger::GetAgentModel( int iModelIndex ) -> CustomModel_t
{
	std::lock_guard lock( m_Lock );

	if ( iModelIndex == -1 )
	{
		CustomModel_t CustomModel;
		CustomModel.szName = _xor( "Disabled" );
		return CustomModel;
	}

	if ( iModelIndex < 0 || iModelIndex >= m_vecAgentModels.size( ) )
		return CustomModel_t( );

	return m_vecAgentModels[ iModelIndex ];
}

auto CModelChanger::ToProperCase( std::string str ) -> std::string
{
	bool capitalize = true;
	for ( char& ch : str )
	{
		if ( std::isspace( ch ) )
		{
			capitalize = true;
		}
		else if ( capitalize )
		{
			ch = std::toupper( ch );
			capitalize = false;
		}
		else
		{
			ch = std::tolower( ch );
		}
	}
	return str;
}

auto CModelChanger::FormatName( std::string name ) -> std::string
{
	std::replace( name.begin( ), name.end( ), '_', ' ' ); // Replace underscores with spaces
	return ToProperCase( name );
}

auto GetModelChanger( ) -> CModelChanger*
{
	return &g_CModelChanger;
}
