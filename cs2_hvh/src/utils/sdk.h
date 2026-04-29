#pragma once
#include <cstdint>
#include <cmath>
#include <windows.h>

namespace cs2 {

// ============== Vector Math ==============
struct Vector2 {
    float x, y;
    Vector2() : x(0), y(0) {}
    Vector2(float v) : x(v), y(v) {}
    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator-(const Vector2& o) const { return {x - o.x, y - o.y}; }
    Vector2 operator+(const Vector2& o) const { return {x + o.x, y + o.y}; }
    float length() const { return sqrtf(x * x + y * y); }
};

struct Vector3 {
    float x, y, z;
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float v) : x(v), y(v), z(v) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3 operator/(float s) const { return {x / s, y / s, z / s}; }
    float dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }
    float length() const { return sqrtf(x * x + y * y + z * z); }
    float length2d() const { return sqrtf(x * x + y * y); }
    float dist_to(const Vector3& o) const { return (*this - o).length(); }
    Vector3 normalized() const {
        float len = length();
        return len > 0.001f ? *this / len : Vector3();
    }
};

struct Vector4 {
    float x, y, z, w;
};

struct Matrix3x4 {
    float m[3][4];

    Vector3 get_position() const { return {m[0][3], m[1][3], m[2][3]}; }
};

struct Matrix4x4 {
    float m[4][4];
};

// ============== Color ==============
struct Color {
    float r, g, b, a;
    Color() : r(1), g(1), b(1), a(1) {}
    Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

    static Color from_imvec4(const float* c) { return {c[0], c[1], c[2], c[3]}; }
    DWORD to_argb() const {
        int R = (int)(r * 255.0f) & 0xFF;
        int G = (int)(g * 255.0f) & 0xFF;
        int B = (int)(b * 255.0f) & 0xFF;
        int A = (int)(a * 255.0f) & 0xFF;
        return (A << 24) | (R << 16) | (G << 8) | B;
    }
};

// ============== CS2 NetVars (schema offsets from cs2-dumper 2026-04-28) ==============
// Reference: https://github.com/a2x/cs2-dumper/tree/main/output
struct NetVars {
    // CBasePlayerController (parent of CCSPlayerController)
    static constexpr uint32_t m_iszPlayerName   = 0x6F0;  // char[128]

    // CCSPlayerController
    static constexpr uint32_t m_hPlayerPawn     = 0x904;  // CHandle<C_CSPlayerPawn>

    // C_BaseEntity
    static constexpr uint32_t m_vecOrigin       = 0x80;
    static constexpr uint32_t m_bDormant        = 0x103;  // CGameSceneNode->m_bDormant
    static constexpr uint32_t m_pGameSceneNode  = 0x330;
    static constexpr uint32_t m_iHealth         = 0x34C;
    static constexpr uint32_t m_lifeState       = 0x354;  // uint8
    static constexpr uint32_t m_iTeamNum        = 0x3EB;  // uint8
    static constexpr uint32_t m_fFlags          = 0x3F8;

    // CSkeletonInstance (extends CGameSceneNode)
    static constexpr uint32_t m_modelState      = 0x150;

    // C_BaseModelEntity (extends C_BaseEntity)
    static constexpr uint32_t m_vecViewOffset   = 0xE70;

    // C_BasePlayerPawn (extends C_BaseModelEntity via C_BaseCombatCharacter)
    static constexpr uint32_t m_pWeaponServices   = 0x11E0;
    static constexpr uint32_t m_pMovementServices  = 0x1220;
    static constexpr uint32_t m_hController        = 0x13A8; // CHandle<CBasePlayerController>

    // C_CSPlayerPawn (extends C_CSPlayerPawnBase)
    static constexpr uint32_t m_pAimPunchServices  = 0x1490;
    static constexpr uint32_t m_angEyeAngles       = 0x3300;
    static constexpr uint32_t m_iIDEntIndex        = 0x33DC;

    // Component: CPlayer_MovementServices
    static constexpr uint32_t m_nButtons         = 0x50;   // CInButtonState (read as uint32)

    // Component: CCSPlayer_AimPunchServices
    static constexpr uint32_t m_aimPunchAngle     = 0x50;   // m_predictableBaseAngle (QAngle)

    // Component: CPlayer_WeaponServices
    static constexpr uint32_t m_hActiveWeapon     = 0x60;   // CHandle<C_BasePlayerWeapon>

    // Weapon: C_BasePlayerWeapon
    static constexpr uint32_t m_iClip1            = 0x16D8;

    // Bone indices (stable across Source 2)
    static constexpr int BONE_HEAD   = 6;
    static constexpr int BONE_NECK   = 5;
    static constexpr int BONE_CHEST  = 4;
    static constexpr int BONE_PELVIS = 1;
};

// ============== Global Vars ==============
struct GlobalVars {
    float realtime;
    int framecount;
    float absoluteframetime;
    float curtime;
    float frametime;
    int max_clients;
    int tick_count;
    float interval_per_tick;
};

// ============== Bone Data ==============
struct BoneData {
    Vector3 position;
    float scale;
    Vector4 rotation;
};

// ============== Entity Structures (reference layouts) ==============
struct CEntityInstance {
    uintptr_t vtable;
};

// CCSPlayerController extends CBasePlayerController
struct CCSPlayerController : CEntityInstance {
    char pad0[0x6F0];
    char m_iszPlayerName[128];      // 0x6F0
    char pad1[0x904 - 0x770];
    uint32_t m_hPlayerPawn;         // 0x904
};

// C_BaseEntity
struct C_BaseEntity {
    char pad0[0x80];
    Vector3 m_vecOrigin;            // 0x80
    char pad1[0x103 - 0x8C];
    bool m_bDormant;                // 0x103 (CGameSceneNode)
    char pad2[0x330 - 0x104];
    uintptr_t m_pGameSceneNode;     // 0x330
    char pad3[0x34C - 0x338];
    int32_t m_iHealth;              // 0x34C
    uint8_t m_lifeState;            // 0x354
    char pad4[0x3EB - 0x355];
    uint8_t m_iTeamNum;             // 0x3EB
    char pad5[0x3F8 - 0x3EC];
    uint32_t m_fFlags;              // 0x3F8
};

// C_BaseModelEntity extends C_BaseEntity
struct C_BaseModelEntity : C_BaseEntity {
    char pad6[0xE70 - 0x3FC];
    Vector3 m_vecViewOffset;        // 0xE70
};

// C_BasePlayerPawn extends C_BaseModelEntity
struct C_BasePlayerPawn : C_BaseModelEntity {
    char pad7[0x11E0 - 0xE7C];
    uintptr_t m_pWeaponServices;    // 0x11E0
    char pad8[0x1220 - 0x11E8];
    uintptr_t m_pMovementServices;  // 0x1220
    char pad9[0x13A8 - 0x1228];
    uint32_t m_hController;         // 0x13A8
};

// C_CSPlayerPawn extends C_CSPlayerPawnBase (which extends C_BasePlayerPawn)
struct C_CSPlayerPawn : C_BasePlayerPawn {
    char pad10[0x1490 - 0x13AC];
    uintptr_t m_pAimPunchServices;  // 0x1490
    char pad11[0x3300 - 0x1498];
    Vector3 m_angEyeAngles;         // 0x3300
    char pad12[0x33DC - 0x330C];
    int32_t m_iIDEntIndex;          // 0x33DC
};

// CGameSceneNode
struct CGameSceneNode {
    char pad0[0x103];
    bool m_bDormant;                // 0x103
};

// CSkeletonInstance extends CGameSceneNode
struct CSkeletonInstance : CGameSceneNode {
    char pad1[0x150 - 0x104];
    uintptr_t m_modelState;         // 0x150 -> bone data array
};

// C_BasePlayerWeapon
struct C_BasePlayerWeapon {
    char pad0[0x16D8];
    int32_t m_iClip1;               // 0x16D8
};

// ============== View Matrix (4x4) ==============
using ViewMatrix = Matrix4x4;

// ============== Screen transform ==============
inline bool world_to_screen(const Vector3& world, Vector2& screen, const ViewMatrix& vm, int w, int h) {
    float x = vm.m[0][0] * world.x + vm.m[0][1] * world.y + vm.m[0][2] * world.z + vm.m[0][3];
    float y = vm.m[1][0] * world.x + vm.m[1][1] * world.y + vm.m[1][2] * world.z + vm.m[1][3];
    float wv = vm.m[3][0] * world.x + vm.m[3][1] * world.y + vm.m[3][2] * world.z + vm.m[3][3];

    if (wv < 0.001f) return false;

    float inv_w = 1.0f / wv;
    screen.x = (w / 2.0f) + (x * inv_w * w / 2.0f);
    screen.y = (h / 2.0f) - (y * inv_w * h / 2.0f);
    return true;
}

// ============== Angle calculation for aimbot ==============
inline Vector3 calc_angle(const Vector3& src, const Vector3& dst) {
    Vector3 delta = dst - src;
    Vector3 angle;
    angle.x = -asinf(delta.z / delta.length()) * 57.2957795131f;
    angle.y = atan2f(delta.y, delta.x) * 57.2957795131f;
    angle.z = 0.0f;
    return angle;
}

inline Vector3 angle_diff(const Vector3& a, const Vector3& b) {
    Vector3 diff = a - b;
    while (diff.y > 180.0f) diff.y -= 360.0f;
    while (diff.y < -180.0f) diff.y += 360.0f;
    while (diff.x > 89.0f) diff.x -= 180.0f;
    while (diff.x < -89.0f) diff.x += 180.0f;
    return diff;
}

inline float get_fov(const Vector3& view_angle, const Vector3& aim_angle) {
    Vector3 diff = angle_diff(aim_angle, view_angle);
    return sqrtf(diff.x * diff.x + diff.y * diff.y);
}

} // namespace cs2
