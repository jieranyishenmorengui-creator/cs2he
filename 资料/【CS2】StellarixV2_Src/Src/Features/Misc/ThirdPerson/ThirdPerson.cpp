#include "ThirdPerson.hpp"
#include <Core/Config/Variables.hpp>
#include <Core/Sdk/Datatypes/CViewSetup.hpp>
#include <Core/Sdk/Datatypes/QAngle.hpp>
#include <Core/Sdk/Datatypes/Vector.hpp>
#include <Core/Sdk/EntityData.hpp>
#include <Core/Utils/InputManager/InputManager.hpp>
#include <Core/Sdk/Globals.hpp>

static CThirdPerson g_CThirdPerson{ };

auto CThirdPerson::OnInit( ) -> bool
{
	m_bInitialized = true;
	return m_bInitialized;
}

auto CThirdPerson::OnDestroy( ) -> void
{
	if ( !m_bInitialized )
		return;
	m_bInitialized = false;
}

auto CThirdPerson::OnPreOverrideViewHook( CViewSetup* pViewSetup ) -> void
{
	if ( !m_bInitialized )
		return;

	CCSPlayerController* pLocalController = CCSPlayerController::GetLocalPlayerController( );
	C_CSPlayerPawn* pLocalPawn = C_CSPlayerPawn::GetPawnFromController( pLocalController );
	if ( !pLocalController || !pLocalPawn )
		return;

	static float flAnimationProgress = 0.f;
	static auto fnLambdaBezier = [ ]( const float t )
	{
		return t * t * ( 3.0f - 2.0f * t );
	};

	const bool bThirdPersonActive = GetInputManager( )->GetBindState( C_GET( KeyBind_t, Vars.bindThirdPerson ) );
	if ( bThirdPersonActive )
		flAnimationProgress = MATH::Clamp( flAnimationProgress + CheatData.flGVFrameTime * 2.5f, 0.f, 1.f );
	else
		flAnimationProgress = MATH::Clamp( flAnimationProgress + CheatData.flGVFrameTime * -2.5f, 0.f, 1.f );



	if ( !pLocalController->m_bPawnIsAlive( ) )
	{
		auto pCPlayer_ObserverServices = pLocalPawn->m_pObserverServices( );
		if ( pCPlayer_ObserverServices && pCPlayer_ObserverServices->m_iObserverMode( ) == ObserverMode_t::OBS_MODE_IN_EYE )
		{
			pCPlayer_ObserverServices->m_iObserverMode( ) = bThirdPersonActive ? ObserverMode_t::OBS_MODE_CHASE : ObserverMode_t::OBS_MODE_IN_EYE;
			pCPlayer_ObserverServices->m_flObserverChaseDistance( ) = static_cast< float >( C_GET( int, Vars.iThirdPersonDistance ) ) * fnLambdaBezier( flAnimationProgress );
			pCPlayer_ObserverServices->m_flObserverChaseDistanceCalcTime( ) = 0.f;
		}

		return;
	}


	if ( flAnimationProgress > 0.f )
	{
		QAngle_t angAjusted = LocalPlayerData.angViewAngle;
		angAjusted.x *= -1;

		pViewSetup->vecOrigin = MATH::CalculateCameraPosition(
			LocalPlayerData.vecEyePosition,
			-static_cast< float >( C_GET( int, Vars.iThirdPersonDistance ) ) * fnLambdaBezier( flAnimationProgress ),
			angAjusted
		);

		QAngle_t p = MATH::NormalizeAngles(
			MATH::CalcAngle(
				pViewSetup->vecOrigin,
				LocalPlayerData.vecEyePosition
			)
		);

		pViewSetup->angView = QAngle_t{
			p.x,
			p.y,
		};
	}
}

auto GetThirdPerson( ) -> CThirdPerson*
{
	return &g_CThirdPerson;
}
