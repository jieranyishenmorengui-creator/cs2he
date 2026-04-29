#pragma once
#include <Core/Renderer/Renderer.hpp>
#include <ImGui/imgui.h>
#include <string>

class CEspText
{
public:
	CEspText( ) = default;
	CEspText( std::string szText_, ImColor imColor_, float flFontSize_ = 9.f ) :
		m_szText( std::move( szText_ ) ), m_imColor( imColor_ ), flFontSize( flFontSize_ )
	{
	}
	CEspText( ImFont* m_pImFont, std::string szText_, ImColor imColor_, float flFontSize_ = 9 ) :
		pImFont( m_pImFont ), m_szText( std::move( szText_ ) ), m_imColor( imColor_ ), flFontSize( flFontSize_ )
	{
	}
	~CEspText( ) = default;

public:
	auto GetText( ) const -> const std::string&
	{
		return m_szText;
	}
	auto GetColor( ) -> ImColor
	{
		return m_imColor;
	}
	auto GetFont( ) -> ImFont*
	{
		return pImFont;
	}
	auto GetFontSize( ) -> const float
	{
		return flFontSize;
	}

private:
	ImFont* pImFont = StellarixFonts::fontVerdana;
	float flFontSize = 9.f;
	std::string m_szText = "";
	ImColor m_imColor;
};


class CEsp
{
public:
	CEsp( ) = default;
	~CEsp( ) = default;

public:
	auto OnInit( ) -> bool;
	auto OnDestroy( ) -> void;

public:
	auto OnPresentHook( ImDrawList* pBackgroundDrawList ) -> void;

private:
	bool m_bInitialized = false;
};

auto GetEsp( ) -> CEsp*;