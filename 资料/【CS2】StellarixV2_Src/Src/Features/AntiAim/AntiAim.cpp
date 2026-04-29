#include "AntiAim.hpp"
#include <Core/Config/Variables.hpp>
#include <Core/Sdk/Datatypes/CUserCmd.hpp>
#include <random>


static CAntiAim g_CAntiAim{ };

auto CAntiAim::OnCreateMovePredictHook( CCSGOInput* pInput, CUserCmd* pUserCmd ) -> void
{
	if ( !C_GET( bool, Vars.bEnableAntiAim ) )
		return;

	if ( pUserCmd->buttonState.nButtonstate1 & IN_USE )
		return;

	CCSPlayerController* pLocalController = CCSPlayerController::GetLocalPlayerController( );
	if ( !pLocalController )
		return;
	if ( !pLocalController->m_bPawnIsAlive( ) )
		return;

	if ( auto pMutableBase = pUserCmd->csgoUserCmd.mutable_base( ); pMutableBase )
	{
		if ( auto pMutableViewAngles = pMutableBase->mutable_viewangles( ); pMutableViewAngles )
		{
			float flPitch = 0.f;
			float flYaw = pMutableViewAngles->has_y( ) ? pMutableViewAngles->y( ) : 0.f;


			unsigned seed = std::chrono::system_clock::now( ).time_since_epoch( ).count( );
			std::mt19937 generator( seed );

			static int iPitchJitterSide = -1;
			static int iYawJitterSide = -1;
			iPitchJitterSide *= -1;
			iYawJitterSide *= -1;


			std::uniform_real_distribution<float> distributionPitch( -89.9f, 89.9f );
			std::uniform_real_distribution<float> distributionYaw( -180.f, 180.f );

			switch ( C_GET( int, Vars.nAntiAimPitch ) )
			{
			case kAntiAimPitchOffset:
				flPitch += C_GET( float, Vars.flAntiAimPitchOffset );
				break;
			case kAntiAimPitchRandom:
				flPitch += distributionPitch( generator );
				break;
			case kAntiAimPitchJitter:
				flPitch = C_GET( float, Vars.flAntiAimPitchOffset ) * iPitchJitterSide;
				break;
			case kAntiAimPitchDown:
				flPitch = 89.9f;
				break;
			case kAntiAimPitchUp:
				flPitch = -89.9f;
				break;
			default:
				break;
			}

			flYaw += C_GET( float, Vars.flAntiAimYawOffset );
			switch ( C_GET( int, Vars.nAntiAimYawModifier ) )
			{
			case kAntiAimYawModifierRandom:
				flYaw += distributionYaw( generator );
				break;
			case kAntiAimYawModifierJitter:
				flYaw += C_GET( float, Vars.flAntiAimYawModifierOffset ) * iYawJitterSide;
				break;
			default:
				break;
			}

			if ( pMutableViewAngles->has_x( ) )
				pMutableViewAngles->set_x( flPitch );

			if ( pMutableViewAngles->has_y( ) )
				pMutableViewAngles->set_y( flYaw );
		}
	}
}

auto GetAntiAim( ) -> CAntiAim*
{
	return &g_CAntiAim;
}
