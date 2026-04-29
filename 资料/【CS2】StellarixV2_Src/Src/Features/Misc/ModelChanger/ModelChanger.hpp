#pragma once
#include <string>
#include <vector>
#include <mutex>

class C_BaseModelEntity;

struct CustomModel_t
{
	std::string szName;
	std::string szPath;
};


class CModelChanger
{
public:
	CModelChanger( ) = default;
	~CModelChanger( ) = default;

public:
	auto OnInit( ) -> bool;
	auto OnDestroy( ) -> void;

public:
	auto OnLevelInitHook( const char* szNewMap ) -> void;
	auto OnLevelShutdownHook( ) -> void;
	auto OnFrameStageNotifyHook( int nFrameStage ) -> void;
	auto OnSetModel( char* szModelName ) -> bool;

public:
	auto OnChangeModelInMenu( ) -> void;

public:
	auto ChangeModel( C_BaseModelEntity* pC_BaseModelEntity ) -> void;

public:
	auto LoadCustomModels( ) -> bool;
	auto GetAgentModel( int iModelIndex ) -> CustomModel_t;
	auto GetAgentModelsCount( ) -> int
	{
		return m_vecAgentModels.size( );
	}

private:
	auto ToProperCase( std::string str ) -> std::string;
	auto FormatName( std::string name ) -> std::string;

private:
	std::vector<CustomModel_t> m_vecAgentModels{};

private:
	std::string szDefaultModelName = "";
	bool bNeedSetModel = true;

private:
	std::mutex m_Lock;
};

auto GetModelChanger( ) -> CModelChanger*;