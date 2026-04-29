#pragma once
#include "imgui.h"
#include <Core/Sdk/Datatypes/Color.hpp>
#include <vector>
#include <string>


struct KeyBind_t;

// extended imgui functionality
namespace ImGui
{
	inline float Lerp( float a, float b, float t )
	{
		return a + t * ( b - a );
	}

	/* @section: main */
	void HelpMarker( const char* szDescription );
	bool HotKey( const char* szLabel, unsigned int* pValue );
	bool HotKey( const char* szLabel, KeyBind_t* pKeyBind, const bool bAllowSwitch = true );
	bool MultiCombo( const char* szLabel, unsigned int* pFlags, const char* const* arrItems, int nItemsCount );
	bool BeginListBox( const char* szLabel, int nItemsCount, int nHeightInItems = -1 );
	bool ColorEdit3( const char* szLabel, Color_t* pColor, ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_DisplayHex );
	bool ColorEdit4( const char* szLabel, Color_t* pColor, ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_AlphaBar );

	bool ColorEdit4Combo( const char* szLabel, Color_t* pColor, const char* label, int* current_item, const char* const items[ ], int items_count, int height_in_items = -1, ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_AlphaBar );

	void DrawCircle( ImDrawList* imDrawList, ImVec2 vecPosition, float flValue, float flMaxValue, float flRadius, bool bNeedDrawText=false );

	bool HotKeyList( const char* szLabel, std::vector<KeyBind_t>* pKeyBindList, int* iSelectedBind, int iMaxBinds = 10, const bool bAllowSwitch = true );
	bool HotKeyList( std::string szLabel, std::vector<KeyBind_t>* pKeyBindList, int* iSelectedBind, int iMaxBinds = 10, const bool bAllowSwitch = true );

	bool HotKeyListPopup( const char* szLabel, std::vector<KeyBind_t>* pKeyBindList, int* iSelectedBind, int iMaxBinds = 10, ImGuiMouseButton_ buttonbCheck = ImGuiMouseButton_Right, bool bAllowSwitch = true );
}