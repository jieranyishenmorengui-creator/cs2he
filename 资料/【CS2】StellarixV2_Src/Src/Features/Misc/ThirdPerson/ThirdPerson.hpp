#pragma once

class CViewSetup;

class CThirdPerson
{
public:
	CThirdPerson( ) = default;
	~CThirdPerson( ) = default;

public:
	auto OnInit( ) -> bool;
	auto OnDestroy( ) -> void;

public:
	auto OnPreOverrideViewHook( CViewSetup* pViewSetup ) -> void;

private:
	bool m_bInitialized = false;
};

auto GetThirdPerson( ) -> CThirdPerson*;