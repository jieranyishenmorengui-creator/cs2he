#include "LoadScreen.hpp"
#include <algorithm>
#include <Core/Config/Variables.hpp>
#include <Core/Renderer/Renderer.hpp>
#include <StellarixUI.hpp>


static CLoadScreen g_CLoadScreen{ };

auto CLoadScreen::OnPresentHook( ImGuiIO& io, ImGuiStyle& style, ImDrawList* pBackgroundDrawList ) -> void
{
	if ( !m_bInProcess )
		return;

	if ( m_vecAnimatedText.empty( ) )
	{
		for ( auto symbol : std::string( CHEAT_NAME ) )
			m_vecAnimatedText.push_back( std::to_string( symbol ) );

		return;
	}


	const float flAnimTimeScale = m_flAnimationTextEndTime * .25f;

	/*  BACKGROUND DIM  */
	pBackgroundDrawList->AddRectFilled( ImVec2( 0, 0 ), io.DisplaySize, ImColor( 0.f, 0.f, 0.f, std::clamp( .45f * ( m_flTimer / flAnimTimeScale ), 0.f, .45f ) ) );


	std::string strPreviewText = "";
	for ( int index = 0; index < m_vecAnimatedText.size( ); index++ )
	{
		if ( m_flTimer >= index * ( flAnimTimeScale / m_vecAnimatedText.size( ) ) )
			strPreviewText += CHEAT_NAME[ index ];
	}


	float flTextSize = 290 * GetStellarixUI( )->flDpiScale;
	const ImVec2 vecSomeOffset = ImVec2( 0, -50 );
	const ImVec2 vecTextSize = StellarixFonts::fontVerdana->CalcTextSizeA( flTextSize, FLT_MAX, 0, strPreviewText.c_str( ) );
	pBackgroundDrawList->AddText( StellarixFonts::fontVerdana, flTextSize, io.DisplaySize / 2 - vecTextSize / 2 + vecSomeOffset + ImVec2( -5, 5 ), C_GET( Color_t, Vars.colUIMainColor ).GetU32( ), strPreviewText.c_str( ) );
	pBackgroundDrawList->AddText( StellarixFonts::fontVerdana, flTextSize, io.DisplaySize / 2 - vecTextSize / 2 + vecSomeOffset, ImColor( 255, 255, 255, 255 ), strPreviewText.c_str( ) );


	m_flTimer += io.DeltaTime < 1.f ? io.DeltaTime : 0.1f;
	if ( m_flTimer >= m_flDurationTime )
		m_bInProcess = false;
}

auto GetLoadScreen( ) -> CLoadScreen*
{
	return &g_CLoadScreen;
}
