#pragma once
#include <ImGui/imgui.h>
#include <ImGui/extension.hpp>


enum EMainContentTabs : int
{
	kRage = 0,
	kLegit,
	kVisuals,
	kMisc,
	kUI,
	kConfigs
};

class CStellarixMenu
{
public:
	CStellarixMenu( ) = default;
	~CStellarixMenu( ) = default;

public:
	auto OnPresentHook( ImGuiIO& io, ImGuiStyle& style ) -> void;

public:
	auto RageTab( ) -> void;
	auto LegitTab( ) -> void;
	auto VisualsPlayerPreview( ) -> void;
	auto VisualsTab( ) -> void;
	auto MiscTab( ) -> void;
	auto UITab( ) -> void;
	auto ConfigsTab( ) -> void;

public:
	auto DrawC4InfoPanel( ) -> void;
	auto DrawSpectatorsList( ) -> void;

private:
	ImVec2 m_vecMainSize = ImVec2( 900, 620 );
	ImVec2 m_vecBombInfoSize = ImVec2( 200, 110 );
	ImVec2 m_vecSpectatorsListSize = ImVec2( 200, 10 );

private:
	EMainContentTabs m_eCurrentMainTab = EMainContentTabs::kRage;

private:
	char m_szConfigFile[ 256U ] = { };
	unsigned long long m_nSelectedConfig = ~1U;
};

auto GetStellarixMenu( ) -> CStellarixMenu*;