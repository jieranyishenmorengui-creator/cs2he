#pragma once
#include <Core/Sdk/Datatypes/CUserCmd.hpp>

struct QAngle_t;

class CMovement
{
public:
	CMovement( ) = default;
	~CMovement( ) = default;

public:
	auto MovementCorrect( CBaseUserCmdPB* pBaseUserCmd, QAngle_t angle ) -> void;
	auto FixCmdButtons( CUserCmd* pUserCmd, CBaseUserCmdPB* pBaseUserCmd ) -> void;
};

auto GetMovement( ) -> CMovement*;