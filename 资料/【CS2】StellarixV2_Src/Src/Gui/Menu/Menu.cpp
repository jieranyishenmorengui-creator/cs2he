#include "Menu.hpp"
#include <Core/Common.hpp>
#include <Core/Config/Variables.hpp>
#include <Core/Engine/Convar/Convar.hpp>
#include <Core/Engine/FuncrionCS2.hpp>
#include <Core/Interfaces/List/IEngineCVar.hpp>
#include <Core/Sdk/EntityCache/EntityCache.hpp>
#include <Core/Sdk/EntityData.hpp>
#include <StellarixUI.hpp>
#include <Features/Visuals/Effects/Effects.hpp>
#include <Features/Misc/ModelChanger/ModelChanger.hpp>

static constexpr const char* arrMiscDpiScale[ ] = {
	"75%",
	"100%",
	"125%",
	"150%",
	"175%",
	"200%"
};

static const char* arrAntiAimPitch[ ] = {
	"Disabled",
	"Offset",
	"Random",
	"Jitter",
	"Down",
	"Up"
};

static const char* arrAntiAimYawModifier[ ] = {
	"Disabled",
	"Random",
	"Jitter"
};

static constexpr const char* arrAimHitboxes[ ] = {
	"Head",
	"Chest",
	"Stomach",
	"Center",
	"Pelvis",
	"Hands",
	"Legs",
};

static constexpr const char* arrPlayerEspFlags[ ] = {
	"Ping",
	"Zoom",
	"Ammunition",
	"C4",
	"Health Kit",
	"Defuser Kit",
};

static constexpr const char* arrChamsMaterials[ ] = {
	"Disabled",
	"Illuminate",
	"Glow",
	"Primary",
	"Latex",
	"Metalic",
};


static CStellarixMenu g_CStellarixMenu{ };

auto CStellarixMenu::OnPresentHook( ImGuiIO& io, ImGuiStyle& style ) -> void
{
	DrawC4InfoPanel( );
	DrawSpectatorsList( );
	if ( !GetStellarixUI( )->bMainWindowOpened )
		return;


	const float flDpiScale = GetStellarixUI( )->flDpiScale;

	ImGui::SetNextWindowSize( m_vecMainSize * flDpiScale, ImGuiCond_Always );
	ImGui::SetNextWindowPos( io.DisplaySize * .5f, ImGuiCond_Once, ImVec2( .5f, .5f ) );
	ImGui::Begin( CHEAT_NAME, &GetStellarixUI( )->bMainWindowOpened, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground );
	{
		const ImVec2 vecMenuPos = ImGui::GetWindowPos( );
		const ImVec2 vecMenuSize = ImGui::GetWindowSize( );
		ImDrawList* pWindowDrawList = ImGui::GetWindowDrawList( );
		ImDrawList* pForegroundDrawList = ImGui::GetForegroundDrawList( );
		ImDrawList* pBackgroundDrawList = ImGui::GetBackgroundDrawList( );


		//   Background
		pBackgroundDrawList->AddRectFilled( vecMenuPos, vecMenuPos + vecMenuSize, C_GET( Color_t, Vars.colUIBackgroundColor ).GetU32( ), 10 );

		// Left Side
		pBackgroundDrawList->AddRectFilled( vecMenuPos, vecMenuPos + ImVec2( 209, 620 ) * flDpiScale, C_GET( Color_t, Vars.colUIMainColor ).GetU32( ), 10, ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersBottomLeft );

		// Top Side
		pBackgroundDrawList->AddRectFilled( vecMenuPos + ImVec2( 209, 0 ) * flDpiScale, vecMenuPos + ImVec2( 209, 0 ) * flDpiScale + ImVec2( 691, 80 ) * flDpiScale, C_GET( Color_t, Vars.colUIMainColor ).GetU32( ), 10, ImDrawFlags_RoundCornersTopRight );

		// Cheat Name
		ImVec2 vecLogoTextSize = StellarixFonts::fontInterBOLD->CalcTextSizeA( 40 * flDpiScale, FLT_MAX, 0, _xor( "STELLARIX" ) );
		pWindowDrawList->AddText( StellarixFonts::fontInterBOLD, 40 * flDpiScale, vecMenuPos + ImVec2( 209.f / 2.f, 25.f ) * flDpiScale - ImVec2( vecLogoTextSize.x / 2.f, 0 ), C_GET( Color_t, Vars.colUILogoColor ).GetU32( ), _xor( "STELLARIX" ) );

		// Spacing Line
		pBackgroundDrawList->AddLine( vecMenuPos + ImVec2( 209, 0 ) * flDpiScale, vecMenuPos + ImVec2( 0, vecMenuSize.y ) + ImVec2( 209, 0 ) * flDpiScale, ImColor( 0, 0, 0, 180 ), 1.5f );

		// User Image
		pWindowDrawList->AddImage( ( ImTextureID )( uintptr_t )StellarixShaders::shaderUserTestCat, vecMenuPos + ImVec2( 834, 15 ) * flDpiScale, vecMenuPos + ImVec2( 834, 15 ) * flDpiScale + ImVec2( 50, 50 ) * flDpiScale );

		// User Name
		ImVec2 vecUserSteamNameSize = StellarixFonts::fontInterBOLD->CalcTextSizeA( 17 * flDpiScale, FLT_MAX, 0, CheatData.szLocalPlayerName.c_str( ) );
		pWindowDrawList->AddText( StellarixFonts::fontInterBOLD, 17 * flDpiScale, vecMenuPos + ImVec2( 824.f, 22.f ) * flDpiScale - ImVec2( vecUserSteamNameSize.x, 0 ), C_GET( Color_t, Vars.colUILogoColor ).GetU32( ), CheatData.szLocalPlayerName.c_str( ) );

		// LifeTime Text
		ImVec2 vecLifeTimeSize = StellarixFonts::fontInterBOLD->CalcTextSizeA( 15 * flDpiScale, FLT_MAX, 0, _xor( "~ lifetime ~" ) );
		pWindowDrawList->AddText( StellarixFonts::fontInterBOLD, 15 * flDpiScale, vecMenuPos + ImVec2( 824.f, 42.f ) * flDpiScale - ImVec2( vecLifeTimeSize.x, 0.f ), ImColor( 145, 145, 145 ), _xor( "~ lifetime ~" ) );


		{
			ImVec2 vecSideButtonSize = ImVec2( 184, 30 ) * flDpiScale;

			ImGui::SetCursorPos( ImVec2( 12, 103 ) * flDpiScale );
			if ( ImGui::Button( _xor( "Rage" ), vecSideButtonSize ) )
				m_eCurrentMainTab = kRage;

			ImGui::SetCursorPos( ImVec2( 12, 148 ) * flDpiScale );
			if ( ImGui::Button( _xor( "Legit" ), vecSideButtonSize ) )
				m_eCurrentMainTab = kLegit;

			// Separator
			pBackgroundDrawList->AddLine( vecMenuPos + ImVec2( 12, 197 ) * flDpiScale, vecMenuPos + ImVec2( 12, 197 ) * flDpiScale + ImVec2( 184, 0 ) * flDpiScale, ImColor( 0, 0, 0, 180 ), 1.3f );

			ImGui::SetCursorPos( ImVec2( 12, 218 ) * flDpiScale );
			if ( ImGui::Button( _xor( "Visuals" ), vecSideButtonSize ) )
				m_eCurrentMainTab = kVisuals;

			ImGui::SetCursorPos( ImVec2( 12, 267 ) * flDpiScale );
			if ( ImGui::Button( _xor( "Misc" ), vecSideButtonSize ) )
				m_eCurrentMainTab = kMisc;

			ImGui::SetCursorPos( ImVec2( 12, 318 ) * flDpiScale );
			if ( ImGui::Button( _xor( "UI" ), vecSideButtonSize ) )
				m_eCurrentMainTab = kUI;

			ImGui::SetCursorPos( ImVec2( 12, 369 ) * flDpiScale );
			if ( ImGui::Button( _xor( "Configs" ), vecSideButtonSize ) )
				m_eCurrentMainTab = kConfigs;
		}



		ImGui::SetCursorPos( ImVec2( 209, 80 ) * flDpiScale );
		if ( ImGui::BeginChild( _xor( "MAIN CONTENT" ) ) )
		{
			switch ( m_eCurrentMainTab )
			{
			case kRage:
				RageTab( );
				break;
			case kLegit:
				LegitTab( );
				break;
			case kVisuals:
				VisualsTab( );
				break;
			case kMisc:
				MiscTab( );
				break;
			case kUI:
				UITab( );
				break;
			case kConfigs:
				ConfigsTab( );
				break;
			default:
				break;
			}
			//ImGui::SliderFloat( _xor( "Fps Compute Interval (sec)" ), &C_GET( float, Vars.flFpsCalculateInterval ), .1f, 5.f );
			ImGui::EndChild( );
		}

	}
	ImGui::End( );
}


auto CStellarixMenu::RageTab( ) -> void
{
	if ( ENABLE_RAGE == 0 )
		return;

	const float flSectionHeight = 250.f * GetStellarixUI( )->flDpiScale;

	ImGui::BeginGroup( );
	{
		if ( ImGui::BeginChild( _xor( "Main##Rage" ), ImVec2( ImGui::GetContentRegionAvail( ).x / 2.f, flSectionHeight ), true, ImGuiWindowFlags_MenuBar ) )
		{
			ImGui::Checkbox( _xor( "Enable##Rage" ), &C_GET( bool, Vars.bEnableRage ) );
		}
		ImGui::EndChild( );


		ImGui::SameLine( );


		if ( ImGui::BeginChild( _xor( "Selection##Rage" ), ImVec2( ImGui::GetContentRegionAvail( ).x, flSectionHeight ), true, ImGuiWindowFlags_MenuBar ) )
		{
			ImGui::MultiCombo( _xor( "Hitboxes##Rage" ), &C_GET_ARRAY( unsigned int, 50, Vars.nHitboxesRage, 0 ), arrAimHitboxes, CS_ARRAYSIZE( arrAimHitboxes ) );
		}
		ImGui::EndChild( );
	}
	ImGui::EndGroup( );



	ImGui::BeginGroup( );
	{
		if ( ImGui::BeginChild( _xor( "Anti Aim##Rage" ), ImVec2( ImGui::GetContentRegionAvail( ).x / 2.f, flSectionHeight ), true, ImGuiWindowFlags_MenuBar ) )
		{
			ImGui::Checkbox( _xor( "Enable##AntiAim" ), &C_GET( bool, Vars.bEnableAntiAim ) );

			ImGui::Combo( _xor( "Pitch##AntiAim" ), &C_GET( int, Vars.nAntiAimPitch ), arrAntiAimPitch, IM_ARRAYSIZE( arrAntiAimPitch ) );
			if ( C_GET( int, Vars.nAntiAimPitch ) == kAntiAimPitchOffset || C_GET( int, Vars.nAntiAimPitch ) == kAntiAimPitchJitter )
				ImGui::SliderFloat( _xor( "Pitch Offset##AntiAim" ), &C_GET( float, Vars.flAntiAimPitchOffset ), -90.f, 90.f, "%.2f" );


			ImGui::SliderFloat( _xor( "Yaw Offset##AntiAim" ), &C_GET( float, Vars.flAntiAimYawOffset ), -180.f, 180.f, "%.2f" );
			ImGui::Combo( _xor( "Yaw Modifier##AntiAim" ), &C_GET( int, Vars.nAntiAimYawModifier ), arrAntiAimYawModifier, IM_ARRAYSIZE( arrAntiAimYawModifier ) );
			if ( C_GET( int, Vars.nAntiAimYawModifier ) == kAntiAimYawModifierJitter )
				ImGui::SliderFloat( _xor( "Yaw Modifier Offset##AntiAim" ), &C_GET( float, Vars.flAntiAimYawModifierOffset ), -180.f, 180.f, "%.2f" );
		}
		ImGui::EndChild( );


		ImGui::SameLine( );


		if ( ImGui::BeginChild( _xor( "Selection##AntiAim" ), ImVec2( ImGui::GetContentRegionAvail( ).x, flSectionHeight ), true, ImGuiWindowFlags_MenuBar ) )
		{

		}
		ImGui::EndChild( );
	}
	ImGui::EndGroup( );
}

auto CStellarixMenu::LegitTab( ) -> void
{

}

auto CStellarixMenu::VisualsPlayerPreview( ) -> void
{

}

auto CStellarixMenu::VisualsTab( ) -> void
{
	float flSectionHeight = 250.f * GetStellarixUI( )->flDpiScale;

	ImGui::BeginGroup( );
	{
		if ( ImGui::BeginChild( _xor( "Visuals##PlayerEsp" ), ImVec2( ImGui::GetContentRegionAvail( ).x / 2.f, flSectionHeight ), true, ImGuiWindowFlags_MenuBar ) )
		{
			ImGui::SeparatorText( _xor( "Player Esp" ) );
			ImGui::Checkbox( _xor( "Enable##PlayerEsp" ), &C_GET( bool, Vars.bPlayerEsp ) );
			ImGui::Checkbox( _xor( "Player Box" ), &C_GET( bool, Vars.bPlayerEspBox ) );
			ImGui::ColorEdit4( _xor( "Player Box Color" ), &C_GET( Color_t, Vars.colPlayerEspBoxColor ) );
			ImGui::Checkbox( _xor( "Player Health Bar" ), &C_GET( bool, Vars.bPlayerEspHealthBar ) );
			ImGui::Checkbox( _xor( "Player Backtrack" ), &C_GET( bool, Vars.bPlayerEspBacktrack ) );
			ImGui::ColorEdit4( _xor( "Player Health Bar Color" ), &C_GET( Color_t, Vars.colPlayerEspHealthBarColor ) );
			ImGui::ColorEdit4( _xor( "Player Backtrack Color" ), &C_GET( Color_t, Vars.colPlayerEspBacktrackColor ) );
			if ( C_GET( bool, Vars.bPlayerEspHealthBar ) )
				ImGui::SliderFloat( _xor( "Player Health Bar Anim Speed" ), &C_GET( float, Vars.flPlayerEspHealthBarAnimSpeed ), .1f, 7.f, "%.2f" );
			ImGui::MultiCombo( _xor( "Player Flags" ), &C_GET( unsigned int, Vars.nPlayerEspFlags ), arrPlayerEspFlags, CS_ARRAYSIZE( arrPlayerEspFlags ) );
		}
		ImGui::EndChild( );


		ImGui::SameLine( );


		if ( ImGui::BeginChild( _xor( "Visuals##WeaponEsp" ), ImVec2( ImGui::GetContentRegionAvail( ).x, flSectionHeight ), true, ImGuiWindowFlags_MenuBar ) )
		{
			ImGui::SeparatorText( _xor( "Weapon Esp" ) );
			ImGui::Checkbox( _xor( "Enable##WeaponEsp" ), &C_GET( bool, Vars.bDroppedWeaponEsp ) );
			ImGui::Checkbox( _xor( "Weapon Box" ), &C_GET( bool, Vars.bDroppedWeaponEspBox ) );
			ImGui::ColorEdit4( _xor( "Weapon Color" ), &C_GET( Color_t, Vars.colDroppedWeaponColor ) );
			ImGui::ColorEdit4( _xor( "C4 Color" ), &C_GET( Color_t, Vars.colEspC4Color ) );
		}
		ImGui::EndChild( );
	}
	ImGui::EndGroup( );


	if ( ImGui::BeginChild( _xor( "Visuals##Chams" ) ) )
	{
		ImGui::ColorEdit4Combo( _xor( "Enemy Visible Chams##Color" ), &C_GET( Color_t, Vars.colEnemyVisiblePlayerChamsColor ), _xor( "##EnemyVisibleChamsMaterial" ), &C_GET( int, Vars.eEnemyVisiblePlayerChams ), arrChamsMaterials, IM_ARRAYSIZE( arrChamsMaterials ) );
		ImGui::ColorEdit4Combo( _xor( "Enemy Invisible Chams##Color" ), &C_GET( Color_t, Vars.colEnemyInvisiblePlayerChamsColor ), _xor( "##EnemyInvisibleChamsMaterial" ), &C_GET( int, Vars.eEnemyInvisiblePlayerChams ), arrChamsMaterials, IM_ARRAYSIZE( arrChamsMaterials ) );

		ImGui::SeparatorText( _xor( "Local" ) );
		ImGui::ColorEdit4Combo( _xor( "Local Player Chams##Color" ), &C_GET( Color_t, Vars.colLocalPlayerChamsColor ), _xor( "##LocalPlayerChamsMaterial" ), &C_GET( int, Vars.eLocalPlayerChams ), arrChamsMaterials, IM_ARRAYSIZE( arrChamsMaterials ) );
		ImGui::ColorEdit4Combo( _xor( "View Model Chams##Color" ), &C_GET( Color_t, Vars.colLocalViewModelChamsColor ), _xor( "##LocalPlayerViewModelChamsMaterial" ), &C_GET( int, Vars.eLocalViewModelChams ), arrChamsMaterials, IM_ARRAYSIZE( arrChamsMaterials ) );
		ImGui::ColorEdit4Combo( _xor( "Local Weapon Chams##Color" ), &C_GET( Color_t, Vars.colLocalWeaponChamsColor ), _xor( "##LocalPlayerWeaponChamsMaterial" ), &C_GET( int, Vars.eLocalWeaponChams ), arrChamsMaterials, IM_ARRAYSIZE( arrChamsMaterials ) );

	}
	ImGui::EndChild( );


}

auto CStellarixMenu::MiscTab( ) -> void
{
	if ( ImGui::Button( _xor( "Open Models Folder" ) ) )
		ShellExecuteA( NULL, "open", GetExplorerManager( )->GetCustomModelsDirectory( ).string( ).c_str( ), NULL, NULL, SW_SHOWDEFAULT );
	if ( ImGui::BeginCombo( _xor( "Agent Model" ), GetModelChanger( )->GetAgentModel( C_GET( int, Vars.iCustomAgent ) ).szName.c_str( ), ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoArrowButton ) )
	{
		if ( ImGui::Selectable( _xor( "Disabled" ), ( C_GET( int, Vars.iCustomAgent ) == -1 ) ) )
			C_GET( int, Vars.iCustomAgent ) = -1;

		for ( int i = 0; i < GetModelChanger( )->GetAgentModelsCount( ); i++ )
		{
			if ( ImGui::Selectable( GetModelChanger( )->GetAgentModel( i ).szName.c_str( ), C_GET( int, Vars.iCustomAgent ) == i ) )
			{
				C_GET( int, Vars.iCustomAgent ) = i;
				GetModelChanger( )->OnChangeModelInMenu( );
			}
		}

		ImGui::EndCombo( );
	}

	ImGui::SeparatorText( _xor( "Watermark" ) );
	ImGui::Checkbox( _xor( "Watermark" ), &C_GET( bool, Vars.bWatermark ) );

	ImGui::SeparatorText( _xor( "Bomb" ) );
	ImGui::Checkbox( _xor( "Enable##BombInfo" ), &C_GET( bool, Vars.bBombInfoPanel ) );

	ImGui::SeparatorText( _xor( "Spectators List" ) );
	ImGui::Checkbox( _xor( "Enable##SpectatorsList" ), &C_GET( bool, Vars.bSpectatorsList ) );

	ImGui::SeparatorText( _xor( "Removals" ) );
	ImGui::Checkbox( _xor( "Legs##Removals" ), &C_GET( bool, Vars.bRemoveLegs ) );
	ImGui::Checkbox( _xor( "Scope Overlay##Removals" ), &C_GET( bool, Vars.bRemoveScope ) );
	ImGui::Checkbox( _xor( "Team Intro##Removals" ), &C_GET( bool, Vars.bRemoveTeamIntro ) );

	ImGui::Separator( );
	ImGui::Checkbox( _xor( "Force Crosshair" ), &C_GET( bool, Vars.bForceCrosshair ) );

	ImGui::Separator( );
	ImGui::Checkbox( _xor( "Auto Accept" ), &C_GET( bool, Vars.bMatchAutoAccept ) );

	ImGui::SeparatorText( _xor( "View Model" ) );
	ImGui::SliderFloat( _xor( "X##ViewModelOffset" ), &C_GET( float, Vars.flViewModelOffsetX ), -10.f, 10.f, "%.1f" );
	ImGui::SliderFloat( _xor( "Y##ViewModelOffset" ), &C_GET( float, Vars.flViewModelOffsetY ), -10.f, 10.f, "%.1f" );
	ImGui::SliderFloat( _xor( "Z##ViewModelOffset" ), &C_GET( float, Vars.flViewModelOffsetZ ), -10.f, 10.f, "%.1f" );
	ImGui::SliderFloat( _xor( "ViewModel Fov##ViewModelFov" ), &C_GET( float, Vars.flViewModelFov ), 10.f, 150.f, "%.1f" );
	ImGui::Separator( );
	ImGui::SliderFloat( _xor( "Fov##CameraServicesFov" ), &C_GET( float, Vars.flFov ), 10.f, 150.f, "%.1f" );
	ImGui::Checkbox( _xor( "Force Zoom Fov" ), &C_GET( bool, Vars.bForceFovInZoom ) );
	ImGui::Separator( );
	ImGui::SliderFloat( _xor( "Aspect Ratio" ), &C_GET( float, Vars.flAspectRatio ), 0.f, 10.f, "%.2f" );


	ImGui::SeparatorText( _xor( "World" ) );
	ImGui::ColorEdit4( _xor( "Lighting Color" ), &C_GET( Color_t, Vars.colLightingColor ) );
	ImGui::ColorEdit4( _xor( "World Color" ), &C_GET( Color_t, Vars.colWorldColor ) );
	ImGui::SliderFloat( _xor( "Lighting Itensity" ), &C_GET( float, Vars.flLightingIntensity ), 0.f, 10.f, "%.1f" );
	if ( ImGui::SliderFloat( _xor( "Gamma" ), &C_GET( float, Vars.flFullScreenGamma ), .1f, 5.f, "%.1f" ) )
		CONVAR::r_fullscreen_gamma->value.fl = C_GET( float, Vars.flFullScreenGamma );


	ImGui::SeparatorText( _xor( "Hit Events" ) );
	ImGui::Checkbox( _xor( "Crosshair Marker" ), &C_GET( bool, Vars.bHitMarkerCrosshair ) );
	ImGui::Checkbox( _xor( "Global Marker" ), &C_GET( bool, Vars.bHitGlobalDamageHitMarker ) );
	ImGui::Checkbox( _xor( "Health Boost" ), &C_GET( bool, Vars.bHitHealthBoostEffect ) );

	if ( ImGui::Button( _xor( "Open HitSounds Folder" ) ) )
		ShellExecuteA( NULL, "open", GetExplorerManager( )->GetHitSoundsDirectory( ).string( ).c_str( ), NULL, NULL, SW_SHOWDEFAULT );
	if ( ImGui::Button( _xor( "Refresh HitSounds" ) ) )
		GetEffects( )->LoadSounds( );
	if ( ImGui::BeginCombo( _xor( "Hit Sound" ), GetEffects( )->GetHitSound( C_GET( int, Vars.iHitSound ) ).szName.c_str( ), ImGuiComboFlags_HeightLarge | ImGuiComboFlags_NoArrowButton ) )
	{
		int iLastHitSoundIndex = C_GET( int, Vars.iHitSound );

		if ( ImGui::Selectable( _xor( "Disabled" ), ( C_GET( int, Vars.iHitSound ) == -1 ) ) )
			C_GET( int, Vars.iHitSound ) = -1;

		for ( int i = 0; i < GetEffects( )->GetSoundsCount( ); i++ )
		{
			if ( ImGui::Selectable( GetEffects( )->GetHitSound( i ).szName.c_str( ), C_GET( int, Vars.iHitSound ) == i ) )
				C_GET( int, Vars.iHitSound ) = i;
		}

		ImGui::EndCombo( );

		if ( iLastHitSoundIndex != C_GET( int, Vars.iHitSound ) )
			GetEffects( )->PlayHitSound( );
	}

	ImGui::SeparatorText( _xor( "Beam" ) );
	ImGui::Checkbox( _xor( "Enable##Beam" ), &C_GET( bool, Vars.bBulletBeam ) );
	ImGui::ColorEdit4( _xor( "Beam Color" ), &C_GET( Color_t, Vars.colBeamColor ) );

	ImGui::SeparatorText( _xor( "Player Trail" ) );
	ImGui::Checkbox( _xor( "Enable##PlayerTrail" ), &C_GET( bool, Vars.bPlayerTrail ) );
	ImGui::ColorEdit4( _xor( "Trail Color" ), &C_GET( Color_t, Vars.colPlayerTrailColor ) );


	ImGui::SeparatorText( _xor( "Third Person" ) );
	ImGui::SliderInt( _xor( "Third Person Distance" ), &C_GET( int, Vars.iThirdPersonDistance ), 10, 250 );
	ImGui::HotKey( _xor( "Third Person" ), &C_GET( KeyBind_t, Vars.bindThirdPerson ) );
}

auto CStellarixMenu::UITab( ) -> void
{
	ImGui::SeparatorText( _xor( "Colors##UI" ) );
	ImGui::ColorEdit4( _xor( "UI Background Color" ), &C_GET( Color_t, Vars.colUIBackgroundColor ) );
	ImGui::ColorEdit4( _xor( "UI Main Color" ), &C_GET( Color_t, Vars.colUIMainColor ) );
	ImGui::ColorEdit4( _xor( "UI Logo Color" ), &C_GET( Color_t, Vars.colUILogoColor ) );
	ImGui::ColorEdit4( _xor( "UI Text Color" ), &C_GET( Color_t, Vars.colUITextColor ) );
	ImGui::ColorEdit4( _xor( "UI Watermark Text Color" ), &C_GET( Color_t, Vars.colUIWatermarkTextColor ) );
	ImGui::ColorEdit4( _xor( "UI Watermark Background Color" ), &C_GET( Color_t, Vars.colUIWatermarkBackgroundColor ) );
	ImGui::ColorEdit4( _xor( "UI Cursor Color" ), &C_GET( Color_t, Vars.colUICursorColor ) );

	ImGui::SeparatorText( _xor( "Colors##UI" ) );
	if ( ImGui::BeginCombo( _xor( "Dpi scale" ), arrMiscDpiScale[ C_GET( int, Vars.iDpiScale ) ] ) )
	{
		for ( int i = 0; i < IM_ARRAYSIZE( arrMiscDpiScale ); i++ )
		{
			if ( ImGui::Selectable( arrMiscDpiScale[ i ], ( C_GET( int, Vars.iDpiScale ) == i ) ) )
				C_GET( int, Vars.iDpiScale ) = i;
		}

		ImGui::EndCombo( );
	}

	ImGui::SeparatorText( _xor( "Binds##UI" ) );
	ImGui::HotKey( _xor( "Unload" ), &C_GET( unsigned int, Vars.nCheatUnloadKey ) );
	ImGui::HotKey( _xor( "Open Menu" ), &C_GET( unsigned int, Vars.nOpenMenuKey ) );
}

auto CStellarixMenu::ConfigsTab( ) -> void
{
	ImGui::Columns( 2, _xor( "#CONFIG" ), false );
	{
		ImGui::PushItemWidth( -1 );

		// check selected configuration for magic value
		if ( m_nSelectedConfig == ~1U )
		{
			// set default configuration as selected on first use
			for ( std::size_t i = 0U; i < CONFIG::vecFileNames.size( ); i++ )
			{
				if ( CRT::StringCompare( CONFIG::vecFileNames[ i ], _xor( L_DEFAULT_CONFIG L_CONFIG_EXTENSION ) ) == 0 )
					m_nSelectedConfig = i;
			}
		}

		if ( ImGui::BeginListBox( _xor( "##config.list" ), CONFIG::vecFileNames.size( ), 10 ) )
		{
			for ( std::size_t i = 0U; i < CONFIG::vecFileNames.size( ); i++ )
			{
				const wchar_t* wszFileName = CONFIG::vecFileNames[ i ];

				char szFileName[ MAX_PATH ] = {};
				CRT::StringUnicodeToMultiByte( szFileName, CS_ARRAYSIZE( szFileName ), wszFileName );

				if ( ImGui::Selectable( szFileName, ( m_nSelectedConfig == i ) ) )
					m_nSelectedConfig = i;
			}

			ImGui::EndListBox( );
		}

		ImGui::PopItemWidth( );
	}
	ImGui::NextColumn( );
	{
		ImGui::PushItemWidth( -1 );
		if ( ImGui::InputTextWithHint( _xor( "##config.file" ), _xor( "Create new..." ), m_szConfigFile, sizeof( m_szConfigFile ), ImGuiInputTextFlags_EnterReturnsTrue ) )
		{
			// check if the filename isn't empty
			if ( const std::size_t nConfigFileLength = CRT::StringLength( m_szConfigFile ); nConfigFileLength > 0U )
			{
				CRT::WString_t wszConfigFile( m_szConfigFile );

				if ( CONFIG::CreateFile( wszConfigFile.Data( ) ) )
					// set created config as selected @todo: dependent on current 'C::CreateFile' behaviour, generally it must be replaced by search
					m_nSelectedConfig = CONFIG::vecFileNames.size( ) - 1U;

				// clear string
				CRT::MemorySet( m_szConfigFile, 0U, sizeof( m_szConfigFile ) );

				// Refresh Configs
				CONFIG::Refresh( );
			}
		}
		if ( ImGui::IsItemHovered( ) )
			ImGui::SetTooltip( _xor( "Press ENTER to create new configuration" ) );

		if ( ImGui::Button( _xor( "Save" ), ImVec2( -1, 20 * GetStellarixUI( )->flDpiScale ) ) )
			CONFIG::SaveFile( m_nSelectedConfig );

		if ( ImGui::Button( _xor( "Load" ), ImVec2( -1, 20 * GetStellarixUI( )->flDpiScale ) ) )
			CONFIG::LoadFile( m_nSelectedConfig );

		if ( ImGui::Button( _xor( "Remove" ), ImVec2( -1, 20 * GetStellarixUI( )->flDpiScale ) ) )
			ImGui::OpenPopup( _xor( "Confirmation Config" ) );

		if ( ImGui::Button( _xor( "Refresh" ), ImVec2( -1, 20 * GetStellarixUI( )->flDpiScale ) ) )
			CONFIG::Refresh( );

		ImGui::PopItemWidth( );
	}
	ImGui::Columns( 1 );

	if ( ImGui::BeginPopupModal( _xor( "Confirmation Config" ), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove ) )
	{
		CRT::String_t<MAX_PATH> szCurrentConfig( CONFIG::vecFileNames[ m_nSelectedConfig ] );
		ImGui::Text( _xor( "Are you sure you want to remove \"%s\" configuration?" ), szCurrentConfig );
		ImGui::Spacing( );

		if ( ImGui::Button( _xor( "No" ), ImVec2( ImGui::GetContentRegionAvail( ).x / 2.f, 0 ) ) )
			ImGui::CloseCurrentPopup( );

		ImGui::SameLine( );

		if ( ImGui::Button( _xor( "Yes" ), ImVec2( ImGui::GetContentRegionAvail( ).x, 0 ) ) )
		{
			CONFIG::RemoveFile( m_nSelectedConfig );

			// reset selected configuration index
			m_nSelectedConfig = ~0U;

			ImGui::CloseCurrentPopup( );
		}

		ImGui::EndPopup( );
	}

}


auto CStellarixMenu::DrawC4InfoPanel( ) -> void
{
	if ( !C_GET( bool, Vars.bBombInfoPanel ) )
		return;

	const auto BombInfo = FunctionsCS2::GetBombInfo( );
	if ( !BombInfo.bPlanted && !GetStellarixUI( )->bMainWindowOpened )
		return;


	const float flDpiScale = GetStellarixUI( )->flDpiScale;
	ImColor bombDamageColor = C_GET( Color_t, Vars.colUITextColor ).GetU32( );
	if ( BombInfo.iBombDamage >= LocalPlayerData.iHealth )
		bombDamageColor = ImColor( 255, 0, 0, 255 );
	else
		bombDamageColor = ImColor( 0, 255, 0, 255 );


	ImGui::SetNextWindowSize( m_vecBombInfoSize * flDpiScale, ImGuiCond_Always );
	ImGui::Begin( _xor( "Bomb Info" ), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground );
	{
		const ImVec2 vecPos = ImGui::GetWindowPos( );
		const ImVec2 vecSize = ImGui::GetWindowSize( );
		ImDrawList* pWindowDrawList = ImGui::GetWindowDrawList( );
		ImDrawList* pForegroundDrawList = ImGui::GetForegroundDrawList( );
		ImDrawList* pBackgroundDrawList = ImGui::GetBackgroundDrawList( );

		// Background
		pBackgroundDrawList->AddRectFilled( vecPos, vecPos + vecSize, C_GET( Color_t, Vars.colUIBackgroundColor ).GetU32( ), 5 );

		// Site Text
		pWindowDrawList->AddText( StellarixFonts::fontInterBOLD, 19 * flDpiScale, vecPos + ImVec2( 5.f, 20.f ) * flDpiScale, C_GET( Color_t, Vars.colUITextColor ).GetU32( ), ( std::string( _xor( "Site: " ) ) + std::string( BombInfo.szBombSite ) ).c_str( ) );

		// Damage Text
		pWindowDrawList->AddText( StellarixFonts::fontInterBOLD, 15 * flDpiScale, vecPos + ImVec2( 5.f, 49.f ) * flDpiScale, bombDamageColor, ( std::string( _xor( "Damage: " ) ) + std::to_string( BombInfo.iBombDamage ) ).c_str( ) );

		// Defuser Name
		if ( !BombInfo.szDefuserName.empty( ) )
			pWindowDrawList->AddText( StellarixFonts::fontInterBOLD, 12 * flDpiScale, vecPos + ImVec2( 5.f, 74.f ) * flDpiScale, C_GET( Color_t, Vars.colUITextColor ).GetU32( ), BombInfo.szDefuserName.c_str( ) );

		// Timer
		const float flRadius = ( 85.f / 2.f );
		ImGui::DrawCircle( pWindowDrawList, vecPos + ImVec2( 108.f, 12.f ) * flDpiScale + ImVec2( flRadius, flRadius ) * flDpiScale, BombInfo.flBombTime, BombInfo.flTimerLength, flRadius * flDpiScale, true );
	}
	ImGui::End( );
}

auto CStellarixMenu::DrawSpectatorsList( ) -> void
{
	if ( !C_GET( bool, Vars.bSpectatorsList ) )
		return;

	const float flDpiScale = GetStellarixUI( )->flDpiScale;
	const float flSpectatorNameSize = 15.5f * flDpiScale;
	const float flPadding = 6.f * flDpiScale;
	const float flWindowName_ContentPadding = 5.f * flDpiScale;
	int iSpectatorsCount = 0;
	CCSPlayerController* pLocalController = CCSPlayerController::GetLocalPlayerController( );
	C_CSPlayerPawn* pLocalPawn = C_CSPlayerPawn::GetPawnFromController( pLocalController );


	ImGui::SetNextWindowSize( m_vecSpectatorsListSize, ImGuiCond_Always );
	ImGui::Begin( _xor( "Spectators List" ), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground );
	{
		const ImVec2 vecPos = ImGui::GetWindowPos( );
		const ImVec2 vecSize = ImGui::GetWindowSize( );
		ImDrawList* pWindowDrawList = ImGui::GetWindowDrawList( );
		ImDrawList* pForegroundDrawList = ImGui::GetForegroundDrawList( );
		ImDrawList* pBackgroundDrawList = ImGui::GetBackgroundDrawList( );


		ImVec2 imSpectatorsTextSize = StellarixFonts::fontVerdana->CalcTextSizeA( 18 * flDpiScale, FLT_MAX, 0, _xor( "Spectators" ) );
		pWindowDrawList->AddText( StellarixFonts::fontVerdana, 18 * flDpiScale, ImVec2( vecPos.x + flPadding, vecPos.y + flPadding ), ImColor( 255, 255, 255 ), _xor( "Spectators" ) );
		pWindowDrawList->AddLine( ImVec2( vecPos.x, vecPos.y + flPadding + imSpectatorsTextSize.y + 1 ), ImVec2( vecPos.x + vecSize.x, vecPos.y + flPadding + imSpectatorsTextSize.y + 1 ), C_GET( Color_t, Vars.colUIMainColor ).GetU32( ), 1.7f * flDpiScale );

		float flMaxNameLength = imSpectatorsTextSize.x;

		// Background
		pBackgroundDrawList->AddRectFilled( vecPos, vecPos + vecSize, C_GET( Color_t, Vars.colUIBackgroundColor ).GetU32( ), 5 );

		if ( pLocalPawn )
		{
			auto& mapCachedEntities = GetEntityCache( )->SpectatorsCachedEntities->GetEntities( );
			if ( !mapCachedEntities.empty( ) )
			{
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
					if ( pCachedPlayer->GetPlayerController( ) == pLocalController )
						continue;
					if ( pCachedPlayer->IsAlive( ) )
						continue;
					auto pObserverServices = pCachedPlayer->GetPlayerObserverServices( );
					if ( !pObserverServices )
						continue;

					C_BaseEntity* pObserverTargetPawn = GetInterfaceManager( )->GetGameEntitySystem( )->Get( pObserverServices->m_hObserverTarget( ) );
					if ( !pObserverTargetPawn )
						continue;
					if ( pObserverTargetPawn->GetRefEHandle( ).GetEntryIndex( ) != pLocalPawn->GetRefEHandle( ).GetEntryIndex( ) )
						continue;

					ImVec2 imPlayerNameSize = StellarixFonts::fontVerdana->CalcTextSizeA( flSpectatorNameSize, FLT_MAX, 0, pCachedPlayer->GetPlayerName( ).c_str( ) );
					flMaxNameLength = MATH::Max<float>( imPlayerNameSize.x, flMaxNameLength );
					pWindowDrawList->AddText( StellarixFonts::fontVerdana, flSpectatorNameSize, ImVec2( vecPos.x + flPadding, vecPos.y + flPadding + imSpectatorsTextSize.y + flWindowName_ContentPadding + imPlayerNameSize.y * iSpectatorsCount ), ImColor( 255, 255, 255 ), pCachedPlayer->GetPlayerName( ).c_str( ) );
					iSpectatorsCount++;
				}
			}
		}

		const float flAnimSpeed = ImGui::GetIO( ).DeltaTime * 5;
		m_vecSpectatorsListSize = ImLerp<ImVec2>( m_vecSpectatorsListSize, ImVec2( flPadding * 2 + flMaxNameLength, flPadding * 2 + flWindowName_ContentPadding + imSpectatorsTextSize.y + flSpectatorNameSize * iSpectatorsCount ), flAnimSpeed );
	}
	ImGui::End( );
}

auto GetStellarixMenu( ) -> CStellarixMenu*
{
	return &g_CStellarixMenu;
}
