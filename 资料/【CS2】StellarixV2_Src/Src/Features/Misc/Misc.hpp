#pragma once

class CViewSetup;

class CMisc
{
public:
	CMisc( ) = default;
	~CMisc( ) = default;

public:
	auto OnInit( ) -> bool;
	auto OnDestroy( ) -> void;

public:
	auto OnLevelInitHook( const char* szNewMap ) -> void;
	auto OnLevelShutdownHook( ) -> void;
	auto OnFrameStageNotifyHook( int nFrameStage ) -> void;
	auto OnPreOverrideViewHook( CViewSetup* pViewSetup ) -> void;
	auto OnPostOverrideViewHook( CViewSetup* pViewSetup ) -> void;

private:
	bool m_bInitialized = false;
};

auto GetMisc( ) -> CMisc*;