#pragma once
#include <string>
#include <vector>
#include <Fnv1a/Fnv1a.h>
#include <mutex>


enum class EVtable : int
{
	kPresent = 8U,
	kResizeBuffers = 13U,
	kCreateSwapChain = 10U,
	kIsRelativeMouseMode = 76U,
	kMouseInputEnabled = 19U,
	kFrameStageNotify = 36U,
};



class CBasePattern
{
public:
	CBasePattern( ) = default;
	CBasePattern( std::string szHookName_, std::string szPattern_, const wchar_t* wszModuleName_, std::uint8_t nOffset_ = 0x0, std::uint8_t nRVAOffset_ = 0x0, std::uint8_t nRIPOffset_ = 0x0 ) :
		szHookName( std::move( szHookName_ ) ), szPattern( std::move( szPattern_ ) ), wszModuleName( std::move( wszModuleName_ ) ), nOffset( nOffset_ ), nRVAOffset( nRVAOffset_ ), nRIPOffset( nRIPOffset_ )
	{
	}
	CBasePattern( std::string szHookName_, void* pObjectPtr_, int nVTableIndex_, std::uint8_t nOffset_ = 0x0, std::uint8_t nRVAOffset_ = 0x0, std::uint8_t nRIPOffset_ = 0x0 ) :
		szHookName( szHookName_ ), pObjectPtr( pObjectPtr_ ), nVTableIndex( nVTableIndex_ ), nOffset( nOffset_ ), nRVAOffset( nRVAOffset_ ), nRIPOffset( nRIPOffset_ )
	{
	}
	CBasePattern( std::string szHookName_, void* pObjectPtr_, EVtable eVTableIndex_, std::uint8_t nOffset_ = 0x0, std::uint8_t nRVAOffset_ = 0x0, std::uint8_t nRIPOffset_ = 0x0 ) :
		szHookName( szHookName_ ), pObjectPtr( pObjectPtr_ ), nVTableIndex( static_cast< int >( eVTableIndex_ ) ), nOffset( nOffset_ ), nRVAOffset( nRVAOffset_ ), nRIPOffset( nRIPOffset_ )
	{
	}

	~CBasePattern( ) = default;

public:
	auto Search( ) -> bool;

public:
	auto GetAddress( ) -> std::uint8_t*
	{
		return pFoundAddress;
	}

public:
	std::string szHookName;
	std::string szPattern;
	const wchar_t* wszModuleName;
	std::uint8_t nOffset;
	std::uint8_t* pFoundAddress = 0x0;

private:
	void* pObjectPtr = nullptr;
	int nVTableIndex;

public:
	std::uint8_t nRVAOffset = 0x0;
	std::uint8_t nRIPOffset = 0x0;
};


class CPatternManager
{
public:
	CPatternManager( ) = default;
	~CPatternManager( ) = default;

public:
	auto OnInit( ) -> bool;
	auto OnDestroy( ) -> void;

public:
	auto InitializePatterns( std::vector<CBasePattern> vecPatterns_ ) -> bool;

public:
	auto GetPattern( const FNV1A_t uNameHash ) -> CBasePattern;

private:
	std::vector<CBasePattern> m_vecPatterns;

private:
	bool m_bInitialized = false;
};

auto GetPatternManager( ) -> CPatternManager*;