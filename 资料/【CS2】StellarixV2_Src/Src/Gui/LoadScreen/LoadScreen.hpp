#pragma once
#include <Imgui/imgui.h>
#include <vector>
#include <string>
#include <Core/Common.hpp>


class CLoadScreen
{
public:
	auto OnPresentHook( ImGuiIO& io, ImGuiStyle& style, ImDrawList* pBackgroundDrawList ) -> void;

public:
	auto IsActive( ) -> bool
	{
		return m_bInProcess;
	}

private:
	std::vector<std::string> m_vecAnimatedText;
	float m_flTimer = 0.f;
	float m_flDurationTime = 6.f;
	float m_flAnimationTextEndTime = m_flDurationTime * .9f;
	bool m_bInProcess = true;

};

auto GetLoadScreen( ) -> CLoadScreen*;