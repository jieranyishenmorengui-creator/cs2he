#pragma once
#include <vector>

class CCSGOInput;
class CUserCmd;


class CAntiAim
{
public:
	CAntiAim( ) = default;
	~CAntiAim( ) = default;

public:
	auto OnCreateMovePredictHook( CCSGOInput* pInput, CUserCmd* pUserCmd ) -> void;
};

auto GetAntiAim( ) -> CAntiAim*;