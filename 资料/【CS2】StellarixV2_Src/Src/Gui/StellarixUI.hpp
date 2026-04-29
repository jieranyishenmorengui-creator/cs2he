#pragma once
#include <windows.h>
#include <corecrt_math.h>
#include <Imgui/imgui.h>


class CStellarixUI
{
public:
	CStellarixUI( ) = default;
	~CStellarixUI( ) = default;

public:
	auto OnInit( ) -> bool;
	auto OnDestroy( ) -> void;

public:
	auto OnPresentHook( ImGuiIO& io, ImGuiStyle& style ) -> void;

public:
	auto ChangeMouseCursor( ImGuiIO& io ) -> void;
	auto SwitchMenuState( ) -> void;

public:
	auto GetUiDpi( const int iScaleTarget ) -> float;

public:
	auto DrawCustomMouseCursor( ImVec2 vecPosition, float flRotationAngleDegrees, ImColor color ) -> void;

public:
	bool bMainWindowOpened = false;
	bool bMainActive = false;

public:
	float flDpiScale = 1.f;

private:
	bool m_bInitialized = false;
};

auto GetStellarixUI( ) -> CStellarixUI*;