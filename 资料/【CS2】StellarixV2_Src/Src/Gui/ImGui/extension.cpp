#include "extension.hpp"
#include "imgui_internal.h"
#include <vector>
#include <string>
#include <Core/Config/Variables.hpp>
#include <Core/Memory/Memory.hpp>
#include <map>

static constexpr const char* arrKeyNames[ ] = {
	"",
	"Mouse 1", "Mouse 2", "Cancel", "Mouse 3", "Mouse 4", "Mouse 5", "",
	"Backspace", "Tab", "", "", "Clear", "Enter", "", "",
	"Shift", "Control", "Alt", "Pause", "Caps", "", "", "", "", "", "",
	"Escape", "", "", "", "", "Space", "Page Up", "Page Down",
	"End", "Home", "Left", "Up", "Right", "Down", "", "", "",
	"Print", "Insert", "Delete", "",
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"", "", "", "", "", "", "",
	"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K",
	"L", "M", "N", "O", "P", "Q", "R", "S", "T", "U",
	"V", "W", "X", "Y", "Z", "LWin", "RWin", "", "", "",
	"Num0", "Num1", "Num2", "Num3", "Num4", "Num5",
	"Num6", "Num7", "Num8", "Num9",
	"*", "+", "", "-", ".", "/",
	"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
	"F9", "f10", "F11", "F12", "F13", "F14", "F15", "F16",
	"F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24",
	"", "", "", "", "", "", "", "",
	"Num Lock", "Scroll Lock",
	"", "", "", "", "", "", "",
	"", "", "", "", "", "", "",
	"LShift", "RShift", "LCtrl",
	"RCtrl", "LMenu", "RMenu"
};

ImGuiKey ImGuiKeyEventToImGuiKey( WPARAM wParam )
{
	switch ( wParam )
	{
	case VK_TAB: return ImGuiKey_Tab;
	case VK_LEFT: return ImGuiKey_LeftArrow;
	case VK_RIGHT: return ImGuiKey_RightArrow;
	case VK_UP: return ImGuiKey_UpArrow;
	case VK_DOWN: return ImGuiKey_DownArrow;
	case VK_PRIOR: return ImGuiKey_PageUp;
	case VK_NEXT: return ImGuiKey_PageDown;
	case VK_HOME: return ImGuiKey_Home;
	case VK_END: return ImGuiKey_End;
	case VK_INSERT: return ImGuiKey_Insert;
	case VK_DELETE: return ImGuiKey_Delete;
	case VK_BACK: return ImGuiKey_Backspace;
	case VK_SPACE: return ImGuiKey_Space;
	case VK_RETURN: return ImGuiKey_Enter;
	case VK_ESCAPE: return ImGuiKey_Escape;
		//case VK_OEM_7: return ImGuiKey_Apostrophe;
	case VK_OEM_COMMA: return ImGuiKey_Comma;
		//case VK_OEM_MINUS: return ImGuiKey_Minus;
	case VK_OEM_PERIOD: return ImGuiKey_Period;
		//case VK_OEM_2: return ImGuiKey_Slash;
		//case VK_OEM_1: return ImGuiKey_Semicolon;
		//case VK_OEM_PLUS: return ImGuiKey_Equal;
		//case VK_OEM_4: return ImGuiKey_LeftBracket;
		//case VK_OEM_5: return ImGuiKey_Backslash;
		//case VK_OEM_6: return ImGuiKey_RightBracket;
		//case VK_OEM_3: return ImGuiKey_GraveAccent;
	case VK_CAPITAL: return ImGuiKey_CapsLock;
	case VK_SCROLL: return ImGuiKey_ScrollLock;
	case VK_NUMLOCK: return ImGuiKey_NumLock;
	case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
	case VK_PAUSE: return ImGuiKey_Pause;
	case VK_NUMPAD0: return ImGuiKey_Keypad0;
	case VK_NUMPAD1: return ImGuiKey_Keypad1;
	case VK_NUMPAD2: return ImGuiKey_Keypad2;
	case VK_NUMPAD3: return ImGuiKey_Keypad3;
	case VK_NUMPAD4: return ImGuiKey_Keypad4;
	case VK_NUMPAD5: return ImGuiKey_Keypad5;
	case VK_NUMPAD6: return ImGuiKey_Keypad6;
	case VK_NUMPAD7: return ImGuiKey_Keypad7;
	case VK_NUMPAD8: return ImGuiKey_Keypad8;
	case VK_NUMPAD9: return ImGuiKey_Keypad9;
	case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
	case VK_DIVIDE: return ImGuiKey_KeypadDivide;
	case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
	case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
	case VK_ADD: return ImGuiKey_KeypadAdd;
	case VK_LSHIFT: return ImGuiKey_LeftShift;
	case VK_LCONTROL: return ImGuiKey_LeftCtrl;
	case VK_LMENU: return ImGuiKey_LeftAlt;
	case VK_LWIN: return ImGuiKey_LeftSuper;
	case VK_RSHIFT: return ImGuiKey_RightShift;
	case VK_RCONTROL: return ImGuiKey_RightCtrl;
	case VK_RMENU: return ImGuiKey_RightAlt;
	case VK_RWIN: return ImGuiKey_RightSuper;
	case VK_APPS: return ImGuiKey_Menu;
	case '0': return ImGuiKey_0;
	case '1': return ImGuiKey_1;
	case '2': return ImGuiKey_2;
	case '3': return ImGuiKey_3;
	case '4': return ImGuiKey_4;
	case '5': return ImGuiKey_5;
	case '6': return ImGuiKey_6;
	case '7': return ImGuiKey_7;
	case '8': return ImGuiKey_8;
	case '9': return ImGuiKey_9;
	case 'A': return ImGuiKey_A;
	case 'B': return ImGuiKey_B;
	case 'C': return ImGuiKey_C;
	case 'D': return ImGuiKey_D;
	case 'E': return ImGuiKey_E;
	case 'F': return ImGuiKey_F;
	case 'G': return ImGuiKey_G;
	case 'H': return ImGuiKey_H;
	case 'I': return ImGuiKey_I;
	case 'J': return ImGuiKey_J;
	case 'K': return ImGuiKey_K;
	case 'L': return ImGuiKey_L;
	case 'M': return ImGuiKey_M;
	case 'N': return ImGuiKey_N;
	case 'O': return ImGuiKey_O;
	case 'P': return ImGuiKey_P;
	case 'Q': return ImGuiKey_Q;
	case 'R': return ImGuiKey_R;
	case 'S': return ImGuiKey_S;
	case 'T': return ImGuiKey_T;
	case 'U': return ImGuiKey_U;
	case 'V': return ImGuiKey_V;
	case 'W': return ImGuiKey_W;
	case 'X': return ImGuiKey_X;
	case 'Y': return ImGuiKey_Y;
	case 'Z': return ImGuiKey_Z;
	case VK_F1: return ImGuiKey_F1;
	case VK_F2: return ImGuiKey_F2;
	case VK_F3: return ImGuiKey_F3;
	case VK_F4: return ImGuiKey_F4;
	case VK_F5: return ImGuiKey_F5;
	case VK_F6: return ImGuiKey_F6;
	case VK_F7: return ImGuiKey_F7;
	case VK_F8: return ImGuiKey_F8;
	case VK_F9: return ImGuiKey_F9;
	case VK_F10: return ImGuiKey_F10;
	case VK_F11: return ImGuiKey_F11;
	case VK_F12: return ImGuiKey_F12;
	case VK_F13: return ImGuiKey_F13;
	case VK_F14: return ImGuiKey_F14;
	case VK_F15: return ImGuiKey_F15;
	case VK_F16: return ImGuiKey_F16;
	case VK_F17: return ImGuiKey_F17;
	case VK_F18: return ImGuiKey_F18;
	case VK_F19: return ImGuiKey_F19;
	case VK_F20: return ImGuiKey_F20;
	case VK_F21: return ImGuiKey_F21;
	case VK_F22: return ImGuiKey_F22;
	case VK_F23: return ImGuiKey_F23;
	case VK_F24: return ImGuiKey_F24;
	case VK_BROWSER_BACK: return ImGuiKey_AppBack;
	case VK_BROWSER_FORWARD: return ImGuiKey_AppForward;
	default: break;
	}

	return ImGuiKey_None;
}

bool ImGui::HotKey( const char* szLabel, unsigned int* pValue )
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* pWindow = g.CurrentWindow;

	if ( pWindow->SkipItems )
		return false;

	ImGuiIO& io = g.IO;
	const ImGuiStyle& style = g.Style;
	const ImGuiID nIndex = pWindow->GetID( szLabel );

	const float flWidth = CalcItemWidth( );
	const ImVec2 vecLabelSize = CalcTextSize( szLabel, nullptr, true );

	const ImRect rectFrame( pWindow->DC.CursorPos + ImVec2( vecLabelSize.x > 0.0f ? style.ItemInnerSpacing.x + GetFrameHeight( ) : 0.0f, 0.0f ), pWindow->DC.CursorPos + ImVec2( flWidth, vecLabelSize.x > 0.0f ? vecLabelSize.y + style.FramePadding.y : 0.f ) );
	const ImRect rectTotal( rectFrame.Min, rectFrame.Max );

	const ImRect bb( pWindow->DC.CursorPos, pWindow->DC.CursorPos + ImVec2( flWidth, vecLabelSize.y + style.FramePadding.y * 2.0f + 5.f ) );

	ItemSize( rectTotal, style.FramePadding.y );
	if ( !ItemAdd( rectTotal, nIndex, &rectFrame ) )
		return false;

	const bool bHovered = ItemHoverable( rectFrame, nIndex, ImGuiItemFlags_None );
	if ( bHovered )
	{
		SetHoveredID( nIndex );
		g.MouseCursor = ImGuiMouseCursor_TextInput;
	}

	const bool hovered = /*ItemHoverable( text, id ) || */ ItemHoverable( rectTotal, nIndex, g.LastItemData.ItemFlags );

	struct TabAnimation
	{
		ImColor ComboFrame;
		ImColor Border;
		ImColor Text;
		float OpenAlpha;
	};

	static std::map<ImGuiID, TabAnimation> TabMap;
	auto TabItemMap = TabMap.find( nIndex );
	if ( TabItemMap == TabMap.end( ) )
	{
		TabMap.insert( { nIndex, TabAnimation( ) } );
		TabItemMap = TabMap.find( nIndex );
	}

	//TabItemMap->second.ComboFrame = ImLerp(TabItemMap->second.ComboFrame.Value, popup_open ? C_GET(ColorPickerVar_t, Vars.colAccent2).colValue.GetVec4(0.6f) : C_GET(ColorPickerVar_t, Vars.colPrimtv3).colValue.GetVec4(1.0f), g.IO.DeltaTime * 8.f * C_GET(float, Vars.flAnimationSpeed));
	TabItemMap->second.Text = ImColor( 230, 230, 230 ); // ImLerp( TabItemMap->second.Text, hovered || g.ActiveId == id ? C_GET( ColorPickerVar_t, Vars.colPrimtv0 ).colValue.GetVec4( 0.8f ) : C_GET( ColorPickerVar_t, Vars.colPrimtv2 ).colValue.GetVec4( 0.8f ), g.IO.DeltaTime * 8.f * C_GET( float, Vars.flAnimationSpeed ) );
	//TabItemMap->second.Border = ImLerp(TabItemMap->second.Border.Value, popup_open ? C_GET(ColorPickerVar_t, Vars.colPrimtv4).colValue.GetVec4(0.8f) : C_GET(ColorPickerVar_t, Vars.colPrimtv4).colValue.GetVec4(0.35f), g.IO.DeltaTime * 8.f * C_GET(float, Vars.flAnimationSpeed));
	//TabItemMap->second.OpenAlpha = ImLerp(TabItemMap->second.OpenAlpha, popup_open ? 1.f : 0.f, g.IO.DeltaTime * 8.f * C_GET(float, Vars.flAnimationSpeed));




	const bool bClicked = bHovered && io.MouseClicked[ 0 ];
	const bool bDoubleClicked = bHovered && io.MouseDoubleClicked[ 0 ];
	if ( bClicked || bDoubleClicked )
	{
		if ( g.ActiveId != nIndex )
		{
			CRT::MemorySet( io.MouseDown, 0, sizeof( io.MouseDown ) );
			for ( int n = ImGuiKey_NamedKey_BEGIN; n < ImGuiKey_NamedKey_END; n++ )
			{
				io.KeysData[ n ].Down = false;
			}

			*pValue = 0U;
		}

		SetActiveID( nIndex, pWindow );
		FocusWindow( pWindow );
	}

	bool bValueChanged = false;
	if ( unsigned int nKey = *pValue; g.ActiveId == nIndex )
	{
		for ( int n = 0; n < IM_ARRAYSIZE( io.MouseDown ); n++ )
		{
			if ( IsMouseDown( n ) )
			{
				switch ( n )
				{
				case 0:
					nKey = VK_LBUTTON;
					break;
				case 1:
					nKey = VK_RBUTTON;
					break;
				case 2:
					nKey = VK_MBUTTON;
					break;
				case 3:
					nKey = VK_XBUTTON1;
					break;
				case 4:
					nKey = VK_XBUTTON2;
					break;
				}

				bValueChanged = true;
				ClearActiveID( );
			}
		}

		// TODO: need fix
		if ( !bValueChanged )
		{
			for ( int n = VK_BACK; n <= VK_RMENU; n++ )
			{
				ImGuiKey imGuiKey = ImGuiKeyEventToImGuiKey( n );
				if ( imGuiKey < ImGuiKey_NamedKey_BEGIN || imGuiKey > ImGuiKey_NamedKey_END )
					continue;

				if ( IsKeyDown( ImGuiKeyEventToImGuiKey( n ) ) )
				{
					nKey = ( int )n;
					bValueChanged = true;
					ClearActiveID( );
				}
			}
		}

		if ( IsKeyPressed( ImGuiKey_Escape ) )
		{
			*pValue = 0U;
			ClearActiveID( );
		}
		else
			*pValue = nKey;
	}

	std::string szBuffer = "  ";
	if ( *pValue != 0 && g.ActiveId != nIndex )
		szBuffer += arrKeyNames[ *pValue ];
	else if ( g.ActiveId == nIndex )
		szBuffer += _xor( "Press Key" );
	else
		szBuffer += _xor( "None" );
	szBuffer += "  ";

	// modified by asphyxia
	PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( style.FramePadding.x, -1 ) );

	const ImVec2 vecBufferSize = CalcTextSize( szBuffer.c_str( ), NULL, true );
	RenderFrame( ImVec2( rectFrame.Max.x - vecBufferSize.x, rectTotal.Min.y ), ImVec2( rectFrame.Max.x, rectTotal.Min.y + style.FramePadding.y + vecBufferSize.y ), GetColorU32( ( bHovered || bClicked || bDoubleClicked ) ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg ), true, style.FrameRounding );
	pWindow->DrawList->AddText( ImVec2( rectFrame.Max.x - vecBufferSize.x, rectTotal.Min.y + style.FramePadding.y ), GetColorU32( g.ActiveId == nIndex ? ImGuiCol_Text : ImGuiCol_TextDisabled ), szBuffer.c_str( ) );

	if ( vecLabelSize.x > 0.f )
		RenderText( ImVec2( bb.Min.x + 10.f, bb.GetCenter( ).y - CalcTextSize( szLabel, NULL, true ).y / 2.f ), szLabel );

	//pWindow->DrawList->AddText( ImVec2( bb.Min.x + 10.f, bb.GetCenter( ).y - CalcTextSize( szLabel ).y / 2.f ), ImColor( TabItemMap->second.Text ), szLabel );

	PopStyleVar( );
	return bValueChanged;
}

bool ImGui::HotKey( const char* szLabel, KeyBind_t* pKeyBind, const bool bAllowSwitch )
{
	const bool bValueChanged = HotKey( szLabel, &pKeyBind->uKey );
	if ( bAllowSwitch )
	{
		std::string szUniqueID = std::string( _xor( "key##" ) ) + std::string( szLabel );

		if ( IsItemClicked( ImGuiMouseButton_Right ) )
			OpenPopup( szUniqueID.c_str( ) );

		if ( BeginPopup( szUniqueID.c_str( ) ) )
		{
			SetNextItemWidth( 200 );
			if ( Combo( _xor( "##keybind.mode" ), reinterpret_cast< int* >( &pKeyBind->nMode ), _xor( "Hold\0Toggle\0" ) ) )
				CloseCurrentPopup( );

			EndPopup( );
		}
	}
	Checkbox( ( std::string( _xor( "Show in Bindlist##" ) ) + std::string( szLabel ) ).c_str( ), &pKeyBind->bShowInBindlist );


	return bValueChanged;
}

bool ImGui::MultiCombo( const char* szLabel, unsigned int* pFlags, const char* const* arrItems, int nItemsCount )
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* pWindow = g.CurrentWindow;

	if ( pWindow->SkipItems )
		return false;

	IM_ASSERT( nItemsCount < 32 ); // bitflags shift overflow, decrease items count or change variable type

	const ImGuiStyle& style = g.Style;
	const ImVec2 vecLabelSize = CalcTextSize( szLabel, nullptr, true );
	const float flActiveWidth = CalcItemWidth( ) - ( vecLabelSize.x > 0.0f ? style.ItemInnerSpacing.x + GetFrameHeight( ) : 0.0f );

	std::vector<const char*> vecActiveItems = {};

	// collect active items
	for ( int i = 0; i < nItemsCount; i++ )
	{
		if ( *pFlags & ( 1 << i ) )
			vecActiveItems.push_back( arrItems[ i ] );
	}

	// fuck it, stl still haven't boost::join, fmt::join replacement
	std::string strBuffer = {};
	for ( std::size_t i = 0U; i < vecActiveItems.size( ); i++ )
	{
		strBuffer.append( vecActiveItems[ i ] );

		if ( i < vecActiveItems.size( ) - 1U )
			strBuffer.append( ", " );
	}

	if ( strBuffer.empty( ) )
		strBuffer.assign( "none" );
	else
	{
		const char* szWrapPosition = g.Font->CalcWordWrapPositionA( GetCurrentWindow( )->FontWindowScale, strBuffer.data( ), strBuffer.data( ) + strBuffer.length( ), flActiveWidth - style.FramePadding.x * 2.0f );
		const std::size_t nWrapLength = szWrapPosition - strBuffer.data( );

		if ( nWrapLength > 0U && nWrapLength < strBuffer.length( ) )
		{
			strBuffer.resize( nWrapLength );
			strBuffer.append( "..." );
		}
	}

	bool bValueChanged = false;
	if ( BeginCombo( szLabel, strBuffer.c_str( ) ) )
	{
		for ( int i = 0; i < nItemsCount; i++ )
		{
			const int nCurrentFlag = ( 1 << i );
			if ( Selectable( arrItems[ i ], ( *pFlags & nCurrentFlag ), ImGuiSelectableFlags_DontClosePopups ) )
			{
				// flip bitflag
				*pFlags ^= nCurrentFlag;
				bValueChanged = true;
			}
		}

		EndCombo( );
	}

	return bValueChanged;
}

bool ImGui::BeginListBox( const char* szLabel, int nItemsCount, int nHeightInItems )
{
	float height = GetTextLineHeightWithSpacing( ) * ( ( nHeightInItems < 0 ? ImMin( nItemsCount, 7 ) : nHeightInItems ) + 0.25f ) + GetStyle( ).FramePadding.y * 2.0f;
	return BeginListBox( szLabel, ImVec2( 0.0f, height ) );
}

bool ImGui::ColorEdit3( const char* szLabel, Color_t* pColor, ImGuiColorEditFlags flags )
{
	return ColorEdit4( szLabel, pColor, flags | ImGuiColorEditFlags_NoAlpha );
}

bool ImGui::ColorEdit4( const char* szLabel, Color_t* pColor, ImGuiColorEditFlags flags )
{
	float arrColor[ 4 ];
	pColor->BaseAlpha( arrColor );

	if ( ColorEdit4( szLabel, &arrColor[ 0 ], flags ) )
	{
		*pColor = Color_t::FromBase4( arrColor );
		return true;
	}

	return false;
}

bool ImGui::ColorEdit4Combo( const char* szLabel, Color_t* pColor, const char* label, int* current_item, const char* const items[ ], int items_count, int height_in_items, ImGuiColorEditFlags flags )
{
	bool bColorEdited = ColorEdit4( szLabel, pColor, flags );
	SameLine( );
	bool bComboEdited = Combo( label, current_item, items, items_count, height_in_items );

	return bColorEdited || bComboEdited;
}

void ImGui::DrawCircle( ImDrawList* imDrawList, ImVec2 vecPosition, float flValue, float flMaxValue, float flRadius, bool bNeedDrawText )
{
	float v1 = flValue / flMaxValue;
	float difference = v1 - 1.0f;

	const ImVec4 col_a_vec = ImGui::ColorConvertU32ToFloat4( IM_COL32( 255, 0, 0, 255 ) );
	const ImVec4 col_b_vec = ImGui::ColorConvertU32ToFloat4( IM_COL32( 0, 255, 0, 255 ) );

	// Perform LERP on each color component (R, G, B, A)
	const ImVec4 lerped_col = ImVec4(
		Lerp( col_a_vec.x, col_b_vec.x, v1 ), // Red
		Lerp( col_a_vec.y, col_b_vec.y, v1 ), // Green
		Lerp( col_a_vec.z, col_b_vec.z, v1 ), // Blue
		Lerp( col_a_vec.w, col_b_vec.w, v1 )  // Alpha
	);

	imDrawList->PathArcTo( vecPosition, flRadius, ( -( MATH::_2PI / 4.0f ) ) + ( MATH::_2PI / flMaxValue ) * ( flMaxValue - flValue ), MATH::_2PI - ( MATH::_2PI / 4.0f ), 200 - 1 );
	imDrawList->PathStroke( ImGui::ColorConvertFloat4ToU32( lerped_col ), ImDrawFlags_None, 2.0f );
	if ( bNeedDrawText )
	{
		std::string szText = std::to_string( ( int )flValue );
		imDrawList->AddText( { ( vecPosition.x - ImGui::CalcTextSize( szText.c_str( ) ).x / 2.0f ) , ( vecPosition.y - ImGui::CalcTextSize( szText.c_str( ) ).y / 2.0f ) }, ImGui::ColorConvertFloat4ToU32( { 1,1,1,1.0f } ), szText.c_str( ) );
	}
}

bool ImGui::HotKeyList( const char* szLabel, std::vector<KeyBind_t>* pKeyBindList, int* iSelectedBind, int iMaxBinds, const bool bAllowSwitch )
{
	if ( *iSelectedBind > pKeyBindList->size( ) )
		*iSelectedBind = 0;

	if ( ImGui::Button( ( std::string( "Add Bind##" ) + std::string( szLabel ) ).c_str( ), ImVec2( ImGui::GetContentRegionAvail( ).x / 2.f, 0 ) ) )
	{
		if ( pKeyBindList->size( ) + 1 <= iMaxBinds )
			pKeyBindList->push_back( KeyBind_t( ) );
	}

	ImGui::SameLine( );

	if ( ImGui::Button( ( std::string( "Remove Bind##" ) + std::string( szLabel ) ).c_str( ), ImVec2( ImGui::GetContentRegionAvail( ).x, 0 ) ) )
	{
		if ( !pKeyBindList->empty( ) && *iSelectedBind < pKeyBindList->size( ) )
		{
			pKeyBindList->erase( pKeyBindList->begin( ) + *iSelectedBind );
			*iSelectedBind--;
		}
	}

	if ( *iSelectedBind >= pKeyBindList->size( ) || *iSelectedBind < 0 )
		*iSelectedBind = 0;

	if ( !pKeyBindList->empty( ) )
	{
		KeyBind_t keyBind = ( *pKeyBindList )[ *iSelectedBind ];
		std::string szKeyBindKey = keyBind.uKey > 0 && keyBind.uKey < sizeof( arrKeyNames ) ? arrKeyNames[ keyBind.uKey ] : "None";

		if ( ImGui::BeginCombo( ( std::string( "##KeyBindsCombo" ) + std::string( szLabel ) ).c_str( ), ( std::string( "Bind `" ) + szKeyBindKey + std::string( "`" ) ).c_str( ) ) )
		{
			for ( int i = 0; i < pKeyBindList->size( ); i++ )
			{
				keyBind = ( *pKeyBindList )[ i ];
				szKeyBindKey = keyBind.uKey > 0 && keyBind.uKey < sizeof( arrKeyNames ) ? arrKeyNames[ keyBind.uKey ] : "None";

				if ( ImGui::Selectable( ( std::string( "Bind `" ) + szKeyBindKey + std::string( "`" ) + "##" + std::string( szLabel ) + std::to_string( ( *pKeyBindList )[ i ].uKey ) + std::to_string( i ) ).c_str( ), *iSelectedBind == i ) )
					*iSelectedBind = i;
			}

			ImGui::EndCombo( );
		}
		return ImGui::HotKey( ( std::string( _xor( "Bind##" ) ) + std::string( szLabel ) ).c_str( ), &( *pKeyBindList )[ *iSelectedBind ] );
	}

	return false;
}

bool ImGui::HotKeyList( std::string szLabel, std::vector<KeyBind_t>* pKeyBindList, int* iSelectedBind, int iMaxBinds, const bool bAllowSwitch )
{
	return HotKeyList( szLabel.c_str( ), pKeyBindList, iSelectedBind, iMaxBinds, bAllowSwitch );
}

bool ImGui::HotKeyListPopup( const char* szLabel, std::vector<KeyBind_t>* pKeyBindList, int* iSelectedBind, int iMaxBinds, ImGuiMouseButton_ buttonbCheck, bool bAllowSwitch )
{
	bool bChanged = false;
	std::string szUniqueID = std::string( _xor( "HotKeyListPopup##" ) ) + std::string( szLabel );

	if ( IsItemClicked( buttonbCheck ) )
		OpenPopup( szUniqueID.c_str( ) );

	SetNextWindowSize( ImVec2( 250, 0 ), ImGuiCond_Appearing ); // Set specific size
	if ( BeginPopup( szUniqueID.c_str( ) ) )
	{
		SetNextItemWidth( 200 );
		bChanged = HotKeyList( szLabel, pKeyBindList, iSelectedBind, iMaxBinds, bAllowSwitch );

		//CloseCurrentPopup( );

		EndPopup( );
	}

	return bChanged;
}
