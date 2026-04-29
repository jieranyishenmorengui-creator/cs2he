#include "Esp.hpp"
#include <Core/Config/Variables.hpp>
#include <Core/Engine/Convar/Convar.hpp>
#include <Core/Interfaces/List/IEngineCVar.hpp>
#include <Core/Sdk/EntityCache/EntityCache.hpp>
#include <ImGui/extension.hpp>

static CEsp g_CEsp{};

__forceinline auto RenderPlayerEsp( ImDrawList* pBackgroundDrawList, CCachedPlayer* pCachedPlayer ) -> void
{
	if ( !C_GET( bool, Vars.bPlayerEsp ) )
		return;

	const float flHealthBarSize = 3.7f;
	const float flFontSize = 10.5f;
	const float flWeaponIconsFontSize = 15.f;
	const float flActiveWeaponIconsFontSize = 12.f;
	const float flPlayerNameFontSize = 14.f;
	const float flPlayerFlagsFontSize = 11.f;
	const float flMaxDistanceFactor = 3000.f;
	const ImVec2 vecHealthBarOffset = { 5.f, 0.f };

	const int iMaxRecordsTick = TIME_TO_TICKS( CONVAR::sv_maxunlag->value.fl );
	const float flLastValidSimTime = TIME_TO_TICKS( CONVAR::sv_maxunlag->value.fl );

	CCSPlayerController* pLocalController = CCSPlayerController::GetLocalPlayerController( );

	pCachedPlayer->OnInit( );
	pCachedPlayer->OnUpdateHealthAnimation( ImGui::GetIO( ).DeltaTime );

	if ( !pCachedPlayer->IsValid( ) ) return;
	if ( pCachedPlayer->GetPlayerController( ) == pLocalController ) return;

	pCachedPlayer->CalculateBoundingBox( );
	if ( !pCachedPlayer->OnScreen( ) ) return;
	if ( !pCachedPlayer->IsAlive( ) )
	{
		if ( auto pObserverServices = pCachedPlayer->GetPlayerObserverServices( ); pObserverServices )
		{
			if ( !pObserverServices->m_hObserverTarget( ).IsValid( ) )
			{
				const ImVec2 vecBoxMin = ImVec2( pCachedPlayer->vecEntityBoundingBox[ 0 ], pCachedPlayer->vecEntityBoundingBox[ 1 ] );
				const float flBoxSize = MATH::Max<float>( pCachedPlayer->vecEntityBoundingBox[ 2 ] - pCachedPlayer->vecEntityBoundingBox[ 0 ], 20 );

				pBackgroundDrawList->AddImage( ( ImTextureID )( uintptr_t )StellarixShaders::shaderGhost, vecBoxMin, vecBoxMin + ImVec2( flBoxSize, flBoxSize ), ImVec2( 0, 0 ), ImVec2( 1, 1 ), ImColor( 96, 151, 181 ) );
			}
		}
		return;
	}
	if ( !pCachedPlayer->GetPlayerPawn( )->IsEnemy( pLocalController ) ) return;

	//int iDistanceFactor = static_cast< int >( 255.f * ( 1.f - pCachedPlayer.GetDistanceToLocalPlayer( ) / flMaxDistanceFactor ) );
	CLagCompRecord LastValidLagRecord = pCachedPlayer->GetLastValidRecord( );

	const ImVec2 vecBoxMin = ImVec2( pCachedPlayer->vecEntityBoundingBox[ 0 ], pCachedPlayer->vecEntityBoundingBox[ 1 ] );
	const ImVec2 vecBoxMax = ImVec2( pCachedPlayer->vecEntityBoundingBox[ 2 ], pCachedPlayer->vecEntityBoundingBox[ 3 ] );
	const float flBoxHeight = vecBoxMax.y - vecBoxMin.y;
	const float flCenterX = vecBoxMin.x + ( ( vecBoxMax.x - vecBoxMin.x ) / 2.f );

	ImVec2 vecHealthTextSize = StellarixFonts::fontVerdana->CalcTextSizeA( flFontSize, FLT_MAX, 0, std::to_string( pCachedPlayer->GetCurrentHealth( ) ).c_str( ) ) + ImVec2( 2, 2 );  // + Some Offset

	ImVec2 vecHealthBarFullMin = vecBoxMin - ImVec2( flHealthBarSize / 2.f, 0 ) - vecHealthBarOffset;
	ImVec2 vecHealthBarFullMax = ImVec2( vecBoxMin.x, vecBoxMax.y ) - vecHealthBarOffset;
	ImVec2 vecHealthBarCurrentMin = ImVec2( vecBoxMin.x - flHealthBarSize / 2.f, vecBoxMax.y - ( flBoxHeight * ( pCachedPlayer->GetAnimatedHealth( ) / pCachedPlayer->GetMaxHealth( ) ) ) ) - vecHealthBarOffset;


	bool bHaveC4Bomb = false;
	CCSPlayer_WeaponServices* pWeaponServices = pCachedPlayer->GetPlayerWeaponServices( );
	//CBaseHandle hActiveWeapon = FunctionsCS2::FnGetActiveWeapon( pWeaponServices ) ? FunctionsCS2::FnGetActiveWeapon( pWeaponServices )->GetRefEHandle( ) : CBaseHandle( );
	CBaseHandle hActiveWeapon = pWeaponServices ? pWeaponServices->m_hActiveWeapon( ) : CBaseHandle( );

	auto vecNameTextSize = StellarixFonts::fontVerdana->CalcTextSizeA( flPlayerNameFontSize, FLT_MAX, 0, pCachedPlayer->GetPlayerName( ).c_str( ) ) + ImVec2( 2, 2 );  // + Some Offset
	pBackgroundDrawList->AddTextOutline( StellarixFonts::fontVerdana, flPlayerNameFontSize, ImVec2( flCenterX - ( vecNameTextSize.x / 2.f ), vecBoxMin.y - vecNameTextSize.y ) - ImVec2( 0, -1 ), ImColor( 255, 255, 255 ), pCachedPlayer->GetPlayerName( ).c_str( ) );


	if ( C_GET( bool, Vars.bPlayerEspHealthBar ) )
	{
		pBackgroundDrawList->AddRectFilled( vecHealthBarFullMin - ImVec2( 1, 1 ), vecHealthBarFullMax + ImVec2( 1, 1 ), ImColor( 0, 0, 0, 170 ), 1.f, ImDrawFlags_RoundCornersAll );
		pBackgroundDrawList->AddRectFilled( vecHealthBarCurrentMin, vecHealthBarFullMax, C_GET( Color_t, Vars.colPlayerEspHealthBarColor ).GetU32( ), 1.f, ImDrawFlags_RoundCornersAll );

		if ( pCachedPlayer->GetCurrentHealth( ) != pCachedPlayer->GetMaxHealth( ) )
			pBackgroundDrawList->AddTextOutline( StellarixFonts::fontVerdana, flFontSize, ImVec2( vecHealthBarCurrentMin.x - vecHealthTextSize.x, vecHealthBarCurrentMin.y ), ImColor( 255, 255, 255 ), std::to_string( pCachedPlayer->GetCurrentHealth( ) ).c_str( ) );
	}

	if ( C_GET( bool, Vars.bPlayerEspBox ) )
	{
		pBackgroundDrawList->AddRect( vecBoxMin, vecBoxMax, ImColor( 0, 0, 0 ), 0.35f, ImDrawFlags_RoundCornersAll, 1.5f );
		pBackgroundDrawList->AddRect( vecBoxMin, vecBoxMax, C_GET( Color_t, Vars.colPlayerEspBoxColor ).GetU32( ), 0.35f, ImDrawFlags_RoundCornersAll, 1.f );
	}

	if ( C_GET( bool, Vars.bPlayerEspBacktrack ) )
	{
		ImVec2 imScreenBacktrackPos;
		if ( GetRenderer( )->WorldToScreen( Vector_t( LastValidLagRecord.VecOrigin.x, LastValidLagRecord.VecOrigin.y, LastValidLagRecord.VecOrigin.z + 50 ), &imScreenBacktrackPos ) )
			pBackgroundDrawList->AddCircleFilled( imScreenBacktrackPos, 5, C_GET( Color_t, Vars.colPlayerEspBacktrackColor ).GetU32( ), 15 );
	}

	if ( C_GET( unsigned int, Vars.nPlayerEspFlags ) & kFlagsAmmunition && pWeaponServices )
	{
		auto& vecAllWeapons = pWeaponServices->m_hMyWeapons( );
		if ( vecAllWeapons.pElements )
		{
			static std::vector<CCachedWeapon> vecPlayerAmmunition;
			vecPlayerAmmunition.clear( );

			for ( int index = 0; index < vecAllWeapons.nSize; index++ )
			{
				auto hWeaponHandle = vecAllWeapons.pElements[ index ];
				if ( !hWeaponHandle.IsValid( ) )
					continue;

				auto pIndexCachedWeapon = CCachedWeapon( hWeaponHandle );
				pIndexCachedWeapon.OnInit( );
				if ( !pIndexCachedWeapon.IsValid( ) )
					continue;
				if ( pIndexCachedWeapon.IsC4( ) )
					bHaveC4Bomb = true;

				if ( pIndexCachedWeapon.GetCSWeaponBase( )->GetRefEHandle( ) == hActiveWeapon )
				{
					const ImVec2 vecActiveWeaponIconSize = StellarixFonts::fontWeaponIcons->CalcTextSizeA( flActiveWeaponIconsFontSize, FLT_MAX, 0, pIndexCachedWeapon.GetWeaponIcon( ) );
					const ImVec2 vecActiveWeaponTextSize = StellarixFonts::fontVerdana->CalcTextSizeA( flActiveWeaponIconsFontSize, FLT_MAX, 0, pIndexCachedWeapon.GetWeaponName( ).c_str( ) );
					const ImVec2 vecActiveWeaponIconPos = ImVec2( flCenterX - ( vecActiveWeaponIconSize.x / 2.f ), vecBoxMax.y + vecActiveWeaponTextSize.y + 1 );
					const ImVec2 vecActiveWeaponTextPos = ImVec2( flCenterX - ( vecActiveWeaponTextSize.x / 2.f ), vecBoxMax.y );

					pBackgroundDrawList->AddTextOutline( StellarixFonts::fontVerdana, flActiveWeaponIconsFontSize, vecActiveWeaponTextPos, ImColor( 235, 235, 235 ), pIndexCachedWeapon.GetWeaponName( ).c_str( ) );
					pBackgroundDrawList->AddTextOutline( StellarixFonts::fontWeaponIcons, flActiveWeaponIconsFontSize, vecActiveWeaponIconPos, ImColor( 235, 235, 235 ), pIndexCachedWeapon.GetWeaponIcon( ) );
					continue;
				}

				if ( pIndexCachedWeapon.IsGrenade( ) )
					vecPlayerAmmunition.push_back( pIndexCachedWeapon );
			}

			for ( int index = 0; index < vecPlayerAmmunition.size( ); index++ )
			{
				auto cachedWeapon = vecPlayerAmmunition.at( index );
				ImVec2 vecWeaponTextSize = StellarixFonts::fontWeaponIcons->CalcTextSizeA( flWeaponIconsFontSize, FLT_MAX, 0, cachedWeapon.GetWeaponIcon( ) ) + ImVec2( 1, 1 );
				ImVec2 vecWeaponTextPos = ImVec2( flCenterX - ( ( vecPlayerAmmunition.size( ) / 2.f ) * vecWeaponTextSize.x ) + index * vecWeaponTextSize.x, vecBoxMin.y - vecWeaponTextSize.y - vecNameTextSize.y - 2 );

				pBackgroundDrawList->AddTextOutline( StellarixFonts::fontWeaponIcons, flWeaponIconsFontSize, vecWeaponTextPos, ImColor( 235, 235, 235 ), cachedWeapon.GetWeaponIcon( ) );
			}
		}
	}


	std::vector<CEspText> vecPlayerFlags;
	if ( C_GET( unsigned int, Vars.nPlayerEspFlags ) & kFlagsPing )
		vecPlayerFlags.push_back( CEspText( std::to_string( pCachedPlayer->GetPlayerPing( ) ) + _xor( "ms" ), ImColor( 255, 255, 255 ), flPlayerFlagsFontSize ) );
	if ( C_GET( unsigned int, Vars.nPlayerEspFlags ) & kFlagsZoom && pCachedPlayer->GetPlayerZoom( ) )
		vecPlayerFlags.push_back( CEspText( "ZOOM", ImColor( 0, 229, 225 ), flPlayerFlagsFontSize ) );
	if ( C_GET( unsigned int, Vars.nPlayerEspFlags ) & kFlagsC4Owner && bHaveC4Bomb )
		vecPlayerFlags.push_back( CEspText( StellarixFonts::fontWeaponIcons, StellarixFonts::GunWeaponIcon( _xor( "weapon_c4" ) ), C_GET( Color_t, Vars.colEspC4Color ).GetU32( ), flPlayerFlagsFontSize + 3 ) );
	if ( C_GET( unsigned int, Vars.nPlayerEspFlags ) & kFlagsHelmetKit && pCachedPlayer->HasHelmet( ) )
		vecPlayerFlags.push_back( CEspText( "HK", ImColor( 200, 200, 200 ), flPlayerFlagsFontSize ) );
	if ( C_GET( unsigned int, Vars.nPlayerEspFlags ) & kFlagsDefuserKit && pCachedPlayer->HasDefuser( ) )
		vecPlayerFlags.push_back( CEspText( StellarixFonts::fontWeaponIcons, StellarixFonts::GunWeaponIcon( _xor( "weapon_defuse_kit" ) ), ImColor( 156, 240, 60 ), flPlayerFlagsFontSize + 4 ) );

	float flFlagsTopPadding = 0.f;
	for ( CEspText visualFlag : vecPlayerFlags )
	{
		pBackgroundDrawList->AddTextOutline( visualFlag.GetFont( ), visualFlag.GetFontSize( ), ImVec2( vecBoxMax.x + 1, vecBoxMin.y + flFlagsTopPadding ), visualFlag.GetColor( ), visualFlag.GetText( ).c_str( ) );
		flFlagsTopPadding += visualFlag.GetFontSize( );
	}
}

__forceinline auto RenderDroppedWeaponEsp( ImDrawList* pBackgroundDrawList, CCachedWeapon* pCachedWeapon ) -> void
{
	if ( !C_GET( bool, Vars.bDroppedWeaponEsp ) )
		return;

	const float flWeaponIconsFontSize = 15.5f;
	const float flMaxAlphaDistance = 2500.f;

	pCachedWeapon->OnInit( );
	pCachedWeapon->OnUpdate( );
	pCachedWeapon->CalculateBoundingBox( );

	if ( !pCachedWeapon->IsValid( ) ) return;
	if ( !pCachedWeapon->OnScreen( ) ) return;
	if ( pCachedWeapon->HasOwner( ) ) return;

	const std::string szWeaponIcon = std::string( pCachedWeapon->GetWeaponIcon( ) );
	const std::string szWeaponName = pCachedWeapon->GetWeaponName( );

	int iAlphaWithDistance = static_cast< int >( 255.f * ( 1.f - pCachedWeapon->GetDistanceToLocalPlayer( ) / flMaxAlphaDistance ) );
	if ( iAlphaWithDistance < 100 )
		iAlphaWithDistance = 100;
	ImColor imTextColor = C_GET( Color_t, Vars.colDroppedWeaponColor ).GetU32( iAlphaWithDistance / 255.f );
	if ( pCachedWeapon->IsC4( ) )
		imTextColor = C_GET( Color_t, Vars.colEspC4Color ).GetU32( );

	const ImVec2 vecBoxMin = { pCachedWeapon->vecEntityBoundingBox[ 0 ], pCachedWeapon->vecEntityBoundingBox[ 1 ] };
	const ImVec2 vecBoxMax = { pCachedWeapon->vecEntityBoundingBox[ 2 ], pCachedWeapon->vecEntityBoundingBox[ 3 ] };
	const ImVec2 vecBoxSize = vecBoxMax - vecBoxMin;
	const ImVec2 vecWeaponNameSize = StellarixFonts::fontVerdana->CalcTextSizeA( flWeaponIconsFontSize, FLT_MAX, 0, szWeaponName.c_str( ) );
	const ImVec2 vecWeaponIconSize = StellarixFonts::fontWeaponIcons->CalcTextSizeA( flWeaponIconsFontSize, FLT_MAX, 0, szWeaponIcon.c_str( ) );


	if ( C_GET( bool, Vars.bDroppedWeaponEspBox ) )
	{
		pBackgroundDrawList->AddRect( vecBoxMin, vecBoxMax, ImColor( 0.f, 0.f, 0.f, iAlphaWithDistance / 255.f ), 0.35f, ImDrawFlags_RoundCornersAll, 1.5f );
		pBackgroundDrawList->AddRect( vecBoxMin, vecBoxMax, imTextColor, 0.35f, ImDrawFlags_RoundCornersAll, 1.f );
	}

	if ( !szWeaponIcon.empty( ) )
		pBackgroundDrawList->AddTextOutline( StellarixFonts::fontWeaponIcons, flWeaponIconsFontSize, ImVec2( vecBoxMin.x + vecBoxSize.x / 2.f - vecWeaponIconSize.x / 2.f, vecBoxMax.y ), imTextColor, szWeaponIcon.c_str( ) );
	pBackgroundDrawList->AddTextOutline( StellarixFonts::fontVerdana, flWeaponIconsFontSize, ImVec2( vecBoxMin.x + vecBoxSize.x / 2.f - vecWeaponNameSize.x / 2.f, vecBoxMax.y + flWeaponIconsFontSize ), imTextColor, szWeaponName.c_str( ) );
}


auto CEsp::OnInit( ) -> bool
{
	m_bInitialized = true;
	return m_bInitialized;
}

auto CEsp::OnDestroy( ) -> void
{
	if ( !m_bInitialized )
		return;
	m_bInitialized = false;
}

auto CEsp::OnPresentHook( ImDrawList* pBackgroundDrawList ) -> void
{
	auto& mapCachedEntities = GetEntityCache( )->VisualsCachedEntities->GetEntities( );
	if ( !mapCachedEntities.empty( ) )
	{
		GetEntityCache( )->VisualsCachedEntities->Lock( );
		for ( auto& pairEntity : mapCachedEntities )
		{
			CBaseHandle hCachedEntityHandle = pairEntity.first;
			CCachedEntities::VariantCachedEntities& variantCachedEntity = pairEntity.second.first;
			ECachedEntityType nCachedEntityType = pairEntity.second.second;

			if ( !hCachedEntityHandle.IsValid( ) )
				continue;

			if ( nCachedEntityType == ECachedEntityType::kUnknown )
				continue;

			CCachedPlayer* pCachedPlayer = std::get_if<CCachedPlayer>( &variantCachedEntity );
			if ( pCachedPlayer )
			{
				RenderPlayerEsp( pBackgroundDrawList, pCachedPlayer );
				continue;
			}

			CCachedWeapon* pCachedWeapon = std::get_if<CCachedWeapon>( &variantCachedEntity );
			if ( pCachedWeapon )
			{
				RenderDroppedWeaponEsp( pBackgroundDrawList, pCachedWeapon );
				continue;
			}

			CCachedEntity* pCachedEntity = std::get_if<CCachedEntity>( &variantCachedEntity );
			if ( pCachedEntity )
			{
				switch ( nCachedEntityType )
				{
				case ECachedEntityType::kC_PlantedC4:
					if ( C_GET( bool, Vars.bBombInfoPanel ) )
					{
						const auto BombInfo = FunctionsCS2::GetBombInfo( );
						if ( BombInfo.bPlanted )
						{
							ImVec2 imC4ScreenPos;
							if ( GetRenderer( )->WorldToScreen( BombInfo.vecPosition, &imC4ScreenPos ) )
							{
								const float flRadius = ( 57.f / 2.f );
								pBackgroundDrawList->AddCircleFilled( imC4ScreenPos, flRadius + 10.f, ImColor( 0, 0, 0, 220 ), 50 );
								ImGui::DrawCircle( pBackgroundDrawList, imC4ScreenPos, BombInfo.flBombTime, BombInfo.flTimerLength, flRadius );

								const float flC4FontSize = flRadius / 1.5f;
								ImVec2 imC4TextSize = StellarixFonts::fontWeaponIcons->CalcTextSizeA( flC4FontSize * 1.5f, FLT_MAX, 0, StellarixFonts::GunWeaponIcon( _xor( "weapon_c4" ) ) );
								pBackgroundDrawList->AddText( StellarixFonts::fontWeaponIcons, flC4FontSize * 1.5f, imC4ScreenPos - imC4TextSize / 2.f, C_GET( Color_t, Vars.colEspC4Color ).GetU32( ), StellarixFonts::GunWeaponIcon( "weapon_c4" ) );
							}
						}
					}
					break;
				default:
					break;
				}

				continue;
			}
		}
		GetEntityCache( )->VisualsCachedEntities->UnLock( );
	}
}

auto GetEsp( ) -> CEsp*
{
	return &g_CEsp;
}
