#pragma once
#include <Core/Sdk/Datatypes/StrongHandle.hpp>
#include <Core/Sdk/EntityCache/EntityCache.hpp>

class CMeshDrawPrimitive;
class CMaterial2;
class C_BaseEntity;

class CChams
{
public:
	CChams( ) = default;
	~CChams( ) = default;

public:
	auto OnInit( ) -> bool;
	auto OnDestroy( ) -> void;

private:
	auto GetEntityType( C_BaseEntity* pBaseEntity ) -> ECachedEntityType;

public:
	auto CreateMaterial( const char* szMaterialName, const char szVmatBuffer[ ] ) -> CStrongHandle<CMaterial2>;

public:
	auto OnDrawArrayHook( void* pAnimatableSceneObjectDesc, void* pDx11, CMeshDrawPrimitive* arrMeshDraw, int nDataCount, void* pSceneView, void* pSceneLayer, void* pUnk ) -> void;

private:
	bool m_bInitialized = false;
};

auto GetChams( ) -> CChams*;