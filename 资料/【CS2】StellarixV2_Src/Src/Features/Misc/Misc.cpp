#include "Misc.hpp"
#include "ModelChanger/ModelChanger.hpp"
#include "ThirdPerson/ThirdPerson.hpp"
#include <Core/Config/Variables.hpp>
#include <Core/Sdk/Datatypes/CViewSetup.hpp>
#include <Core/Utils/Logger/Logger.hpp>


static CMisc g_CMisc{ };

auto CMisc::OnInit( ) -> bool
{
    if ( !GetThirdPerson( )->OnInit( ) )
        return false;
    L_PRINT( LOG_INFO ) << _xor( "Third Person Initialized" );
    
    if ( !GetModelChanger( )->OnInit( ) )
        return false;
    L_PRINT( LOG_INFO ) << _xor( "Model Changer Initialized" );

    m_bInitialized = true;
    return m_bInitialized;
}

auto CMisc::OnDestroy( ) -> void
{
    if ( !m_bInitialized )
        return;
    m_bInitialized = false;

    GetThirdPerson( )->OnDestroy( );
}

auto CMisc::OnLevelInitHook( const char* szNewMap ) -> void
{
    GetModelChanger( )->OnLevelInitHook( szNewMap );
}

auto CMisc::OnLevelShutdownHook( ) -> void
{
    GetModelChanger( )->OnLevelShutdownHook( );
}

auto CMisc::OnFrameStageNotifyHook( int nFrameStage ) -> void
{
    if ( !m_bInitialized )
        return;

    GetModelChanger( )->OnFrameStageNotifyHook( nFrameStage );
}

auto CMisc::OnPreOverrideViewHook( CViewSetup* pViewSetup ) -> void
{
    if ( !m_bInitialized )
        return;
}

auto CMisc::OnPostOverrideViewHook( CViewSetup* pViewSetup ) -> void
{
    if ( !m_bInitialized )
        return;


    GetThirdPerson( )->OnPreOverrideViewHook( pViewSetup );


    // Aspect Ratio
    if ( C_GET( float, Vars.flAspectRatio ) != 0.f )
    {
        pViewSetup->flAspectRatio = C_GET( float, Vars.flAspectRatio );
        pViewSetup->nSomeFlags |= 2;
    }
    else
        pViewSetup->nSomeFlags &= ~2;
}

auto GetMisc( ) -> CMisc*
{
    return &g_CMisc;
}


