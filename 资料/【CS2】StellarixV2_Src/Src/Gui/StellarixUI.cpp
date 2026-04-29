#include "StellarixUI.hpp"
#include <Core/Common.hpp>
#include <Menu/Menu.hpp>
#include <Watermark/Watermark.hpp>
#include <Core/Config/Variables.hpp>


static CStellarixUI g_CStellarixUI{ };

auto CStellarixUI::OnInit( ) -> bool
{
	m_bInitialized = true;
	return m_bInitialized;
}

auto CStellarixUI::OnDestroy( ) -> void
{
	if ( !m_bInitialized )
		return;
	m_bInitialized = false;
}

auto CStellarixUI::OnPresentHook( ImGuiIO& io, ImGuiStyle& style ) -> void
{
	if ( !m_bInitialized )
		return;

	flDpiScale = GetUiDpi( C_GET( int, Vars.iDpiScale ) );

	GetStellarixMenu( )->OnPresentHook( io, style );
	GetStellarixWatermark( )->OnPresentHook( io, style );
	ChangeMouseCursor( io );
}

auto CStellarixUI::ChangeMouseCursor( ImGuiIO& io ) -> void
{
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	if ( bMainWindowOpened )
	{
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;

		ImGui::SetMouseCursor( ImGuiMouseCursor_None );
		DrawCustomMouseCursor( ImGui::GetMousePos( ), 320.f, C_GET( Color_t, Vars.colUICursorColor ).GetU32( ) );
	}
}

auto CStellarixUI::SwitchMenuState( ) -> void
{
	bMainWindowOpened = !bMainWindowOpened;
}

auto CStellarixUI::GetUiDpi( const int iScaleTarget ) -> float
{
	switch ( ( EMiscDpiScale )iScaleTarget )
	{
	case EMiscDpiScale::MISC_DPISCALE_75:
		return .75f;
	case EMiscDpiScale::MISC_DPISCALE_DEFAULT:
		return 1.f;
	case EMiscDpiScale::MISC_DPISCALE_125:
		return 1.25f;
	case EMiscDpiScale::MISC_DPISCALE_150:
		return 1.5f;
	case EMiscDpiScale::MISC_DPISCALE_175:
		return 1.75f;
	case EMiscDpiScale::MISC_DPISCALE_200:
		return 2.f;
	default:
		return 1.f;
	}
}

auto CStellarixUI::DrawCustomMouseCursor( ImVec2 vecPosition, float flRotationAngleDegrees, ImColor color ) -> void
{
	float flArrowBaseWidth = 11 * flDpiScale;
	float flArrowHeight = 12 * flDpiScale;
	float flArrowMiddleWidth = 8 * flDpiScale;
	float flOutlineThickness = 2 * flDpiScale;


	float flAngleRadians = flRotationAngleDegrees * ( 3.14159f / 180.0f );
	ImVec2 vecCenter = ImVec2( vecPosition.x, vecPosition.y + flArrowHeight / 2 );

	ImVec2 vecVerts[ 4 ];
	vecVerts[ 0 ] = vecPosition;
	vecVerts[ 1 ] = ImVec2( vecPosition.x - flArrowBaseWidth / 2, vecPosition.y + flArrowHeight );
	vecVerts[ 2 ] = ImVec2( vecPosition.x, vecPosition.y + flArrowMiddleWidth );
	vecVerts[ 3 ] = ImVec2( vecPosition.x + flArrowBaseWidth / 2, vecPosition.y + flArrowHeight );


	auto rotateVertex = [&]( ImVec2 _vecPoint, ImVec2 _vecCenter, float _flAngle ) -> ImVec2
	{
		float translated_x = _vecPoint.x - _vecCenter.x;
		float translated_y = _vecPoint.y - _vecCenter.y;

		float rotated_x = translated_x * cos( _flAngle ) - translated_y * sin( _flAngle );
		float rotated_y = translated_x * sin( _flAngle ) + translated_y * cos( _flAngle );

		return ImVec2( rotated_x + _vecCenter.x, rotated_y + _vecCenter.y );
	};

	vecVerts[ 0 ] = rotateVertex( vecVerts[ 0 ], vecCenter, flAngleRadians );
	vecVerts[ 1 ] = rotateVertex( vecVerts[ 1 ], vecCenter, flAngleRadians );
	vecVerts[ 2 ] = rotateVertex( vecVerts[ 2 ], vecCenter, flAngleRadians );
	vecVerts[ 3 ] = rotateVertex( vecVerts[ 3 ], vecCenter, flAngleRadians );

	auto pForegroundDrawList = ImGui::GetForegroundDrawList( );
	pForegroundDrawList->AddPolyline( vecVerts, 4, color, true, flOutlineThickness );
	pForegroundDrawList->AddConvexPolyFilled( vecVerts, 4, color );
}

auto GetStellarixUI( ) -> CStellarixUI*
{
	return &g_CStellarixUI;
}
