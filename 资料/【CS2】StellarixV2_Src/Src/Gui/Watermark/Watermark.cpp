#include "Watermark.hpp"
#include <Core/Common.hpp>
#include <StellarixUI.hpp>
#include <string>
#include <Core/Config/Variables.hpp>
#include <Core/Renderer/Renderer.hpp>
#include <Assets/Fonts/Awesome5/Awesome5_Name.hpp>
#include <Core/Sdk/EntityData.hpp>
#include <Core/Interfaces/List/IEngineClient.hpp>
#include <Core/Sdk/Globals.hpp>


static CStellarixWatermark g_CStellarixWatermark{ };

auto CStellarixWatermark::OnPresentHook( ImGuiIO& io, ImGuiStyle& style ) -> void
{
	auto pBackgroundDrawList = ImGui::GetBackgroundDrawList( );

	if ( !C_GET( bool, Vars.bWatermark ) )
		return;
	if ( C_GET( bool, Vars.bWatermarkFixedPos ) )
		ImGui::SetNextWindowPos( ImVec2( 5, 5 ), ImGuiCond_Always );


	CCSPlayerController* pLocalController = CCSPlayerController::GetLocalPlayerController( );
	const char* szWatermarkSettingsPopup = _xor( "WatermarkSettingsPopup" );

	ImGui::SetNextWindowSize( m_vecWatermarkSize, ImGuiCond_Always );
	ImGui::Begin( _xor( "Watermark" ), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground );
	{
		const ImVec2 vecMenuPos = ImGui::GetWindowPos( );
		ImDrawList* pWindowDrawList = ImGui::GetWindowDrawList( );
		ImDrawList* pForegroundDrawList = ImGui::GetForegroundDrawList( );

		ImVec2 vecPreviewElement;
		if ( C_GET( bool, Vars.bWatermarkUserName ) )
			vecPreviewElement = DrawWatermarkElement( pWindowDrawList, vecMenuPos, 17.f * GetStellarixUI( )->flDpiScale, ICON_FA_USER_ALT, CheatData.szLocalPlayerName );
		if ( C_GET( bool, Vars.bWatermarkFps ) )
			vecPreviewElement = DrawWatermarkElement( pWindowDrawList, ImVec2( vecPreviewElement.x + 2, vecMenuPos.y ), 17.f * GetStellarixUI( )->flDpiScale, ICON_FA_CHART_BAR, std::to_string( CheatData.iFps ) + std::string( _xor( " fps" ) ) );
		if ( C_GET( bool, Vars.bWatermarkClock ) )
			vecPreviewElement = DrawWatermarkElement( pWindowDrawList, ImVec2( vecPreviewElement.x + 2, vecMenuPos.y ), 17.f * GetStellarixUI( )->flDpiScale, ICON_FA_CLOCK, CheatData.szCurrentTime );
		if ( C_GET( bool, Vars.bWatermarkPing ) )
			vecPreviewElement = DrawWatermarkElement( pWindowDrawList, ImVec2( vecPreviewElement.x + 2, vecMenuPos.y ), 17.f * GetStellarixUI( )->flDpiScale, ICON_FA_SIGNAL, std::to_string( pLocalController ? pLocalController->m_iPing( ) : 0 ) + std::string( _xor( " ping" ) ) );
		if ( C_GET( bool, Vars.bWatermarkConfig ) )
			vecPreviewElement = DrawWatermarkElement( pWindowDrawList, ImVec2( vecPreviewElement.x + 2, vecMenuPos.y ), 17.f * GetStellarixUI( )->flDpiScale, ICON_FA_FILE_ALT, CONFIG::szCurrentConfig );

		m_vecWatermarkSize = ImLerp( m_vecWatermarkSize, vecPreviewElement - vecMenuPos, io.DeltaTime * 4.f );


		if ( ImGui::IsKeyReleased( ImGuiKey_MouseRight ) && ImGui::IsMouseHoveringRect( vecMenuPos, vecPreviewElement ) )
			ImGui::OpenPopup( szWatermarkSettingsPopup );

		// Watermark Settings
		ImGui::SetNextWindowSize( ImVec2( 250, 0 ) * GetStellarixUI( )->flDpiScale, ImGuiCond_Appearing ); // Set specific size
		if ( ImGui::BeginPopup( szWatermarkSettingsPopup, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBackground ) )
		{
			const ImVec2 vecWatermarkPopupPos = ImGui::GetWindowPos( );
			const ImVec2 vecWatermarkPopupSize = ImGui::GetWindowSize( );

			// Background
			pBackgroundDrawList->AddRectFilled( vecWatermarkPopupPos, vecWatermarkPopupPos + vecWatermarkPopupSize, C_GET( Color_t, Vars.colUIWatermarkBackgroundColor ).GetU32( ), 5 );


			ImGui::Checkbox( _xor( "UserName##Watermark" ), &C_GET( bool, Vars.bWatermarkUserName ) );
			ImGui::Checkbox( _xor( "Fps##Watermark" ), &C_GET( bool, Vars.bWatermarkFps ) );
			ImGui::Checkbox( _xor( "Clock##Watermark" ), &C_GET( bool, Vars.bWatermarkClock ) );
			ImGui::Checkbox( _xor( "Ping##Watermark" ), &C_GET( bool, Vars.bWatermarkPing ) );
			ImGui::Checkbox( _xor( "Config##Watermark" ), &C_GET( bool, Vars.bWatermarkConfig ) );

			ImGui::Separator( );
			ImGui::Checkbox( _xor( "Fixed##Watermark" ), &C_GET( bool, Vars.bWatermarkFixedPos ) );

			ImGui::EndPopup( );
		}
	}
	ImGui::End( );
}

auto CStellarixWatermark::DrawWatermarkElement( ImDrawList* imDrawList, ImVec2 vecPos, float flFontSize, ID3D11ShaderResourceView* shaderIcon, std::string szText ) -> ImVec2
{
	float flMargin = 4.f;
	float flPadding = 6.f;
	float flSpaceIconText = 5.f;
	float flElementWidth = flPadding * 2 + flFontSize + flSpaceIconText + StellarixFonts::fontVerdana->CalcTextSizeA( flFontSize, FLT_MAX, 0, szText.c_str( ) ).x;
	float flElementHeight = flPadding * 2 + flFontSize;

	ImVec2 vecRectStart = vecPos + ImVec2( flMargin, flMargin );
	ImVec2 vecRectEnd = vecRectStart + ImVec2( flElementWidth, flElementHeight );


	imDrawList->AddRectFilled( vecRectStart, vecRectEnd, C_GET( Color_t, Vars.colUIWatermarkBackgroundColor ).GetU32( ), 10 );
	//imDrawList->AddImage( ( ImTextureID )( uintptr_t )shaderIcon, vecRectStart + ImVec2( flPadding, flPadding ), vecRectStart + ImVec2( flFontSize, flFontSize ) + ImVec2( flPadding, flPadding ), ImVec2( 0, 0 ), ImVec2( 1, 1 ), C_GET( Color_t, Vars.colUIWatermarkTextColor ).GetU32( ) );
	imDrawList->AddText( StellarixFonts::fontVerdana, flFontSize, vecRectStart + ImVec2( flPadding, flPadding ) + ImVec2( flFontSize + flSpaceIconText, 0 ), C_GET( Color_t, Vars.colUIWatermarkTextColor ).GetU32( ), szText.c_str( ) );
	imDrawList->AddText( StellarixFonts::fontVerdana, flFontSize, vecRectStart + ImVec2( flPadding, flPadding ) + ImVec2( flFontSize + flSpaceIconText, 0 ), C_GET( Color_t, Vars.colUIWatermarkTextColor ).GetU32( ), szText.c_str( ) );

	return vecRectEnd;
}

auto CStellarixWatermark::DrawWatermarkElement( ImDrawList* imDrawList, ImVec2 vecPos, float flFontSize, const char* szIcon, std::string szText ) -> ImVec2
{
	const float flGlobalMargin = 4.f;
	const float flPadding = 8.f;
	const float flSpaceIconText = 5.f;
	float flIconSize = flFontSize * .85f;

	ImVec2 vecIconSize = StellarixFonts::fontAwesome5->CalcTextSizeA( flIconSize, FLT_MAX, 0, szIcon );
	ImVec2 vecElementSize = ImVec2( flPadding + vecIconSize.x + flSpaceIconText + flPadding, flPadding + flPadding ) + StellarixFonts::fontVerdana->CalcTextSizeA( flFontSize, FLT_MAX, 0, szText.c_str( ) );


	const ImVec2 vecRectStart = vecPos + ImVec2( flGlobalMargin, flGlobalMargin );
	const ImVec2 vecRectEnd = vecRectStart + vecElementSize;


	imDrawList->AddRectFilled( vecRectStart, vecRectEnd, C_GET( Color_t, Vars.colUIWatermarkBackgroundColor ).GetU32( ), 10 );
	imDrawList->AddText( StellarixFonts::fontAwesome5, flIconSize, vecRectStart + ImVec2( flPadding, vecElementSize.y / 2.f ) - ImVec2( 0, vecIconSize.y / 2.f ), C_GET( Color_t, Vars.colUIWatermarkTextColor ).GetU32( ), szIcon );
	imDrawList->AddText( StellarixFonts::fontVerdana, flFontSize, vecRectStart + ImVec2( flPadding, flPadding ) + ImVec2( flFontSize + flSpaceIconText, 0 ), C_GET( Color_t, Vars.colUIWatermarkTextColor ).GetU32( ), szText.c_str( ) );


	return vecRectEnd;
}

auto GetStellarixWatermark( ) -> CStellarixWatermark*
{
	return &g_CStellarixWatermark;
}
