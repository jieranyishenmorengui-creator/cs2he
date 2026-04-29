#pragma once
#include <d3d11.h>
#include <ImGui/imgui.h>
#include <string>
#include <deque>
#include <numeric>


class CStellarixWatermark
{
public:
	CStellarixWatermark( ) = default;
	~CStellarixWatermark( ) = default;

public:
	auto OnPresentHook( ImGuiIO& io, ImGuiStyle& style ) -> void;

public:
	auto DrawWatermarkElement( ImDrawList* imDrawList, ImVec2 vecPos, float flFontSize, ID3D11ShaderResourceView* shaderIcon, std::string szText ) -> ImVec2;
	auto DrawWatermarkElement( ImDrawList* imDrawList, ImVec2 vecPos, float flFontSize, const char* szIcon, std::string szText ) -> ImVec2;

private:
	ImVec2 m_vecWatermarkSize = ImVec2( 220, 33 );
};

auto GetStellarixWatermark( ) -> CStellarixWatermark*;