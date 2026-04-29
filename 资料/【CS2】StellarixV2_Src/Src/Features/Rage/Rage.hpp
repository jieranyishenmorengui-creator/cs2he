#pragma once
#include <vector>

class CCSGOInput;
class CUserCmd;



struct RageConfig_t
{
	float flFov = 0.f;

	std::vector<int> vecAimHitboxes{ };
	std::vector<int> vecAimMultipointHitboxes{ };

	unsigned int nHitboxesRageBackup;
	unsigned int nMultipointHitboxesRageBackup;
};
inline RageConfig_t RageConfig;


class CRage
{
public:
	CRage( ) = default;
	~CRage( ) = default;

public:
	auto OnCreateMovePredictHook( CCSGOInput* pInput, CUserCmd* pUserCmd ) -> void;

public:
	auto UpdateAimHitboxes( std::vector<int>& vecHitboxes, unsigned int nSelected, unsigned int& nBackup ) -> void;
};

auto GetRage( ) -> CRage*;