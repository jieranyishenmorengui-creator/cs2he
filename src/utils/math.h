#pragma once
#include <cmath>
#include <random>
#include "sdk.h"

namespace cs2::math {

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;

inline float deg2rad(float deg) { return deg * DEG2RAD; }
inline float rad2deg(float rad) { return rad * RAD2DEG; }

inline Vector3 vector_rotate(const Vector3& in, const Vector3& rot) {
    float sy = sinf(deg2rad(rot.y));
    float cy = cosf(deg2rad(rot.y));
    float sp = sinf(deg2rad(rot.x));
    float cp = cosf(deg2rad(rot.x));
    float sr = sinf(deg2rad(rot.z));
    float cr = cosf(deg2rad(rot.z));
    return {
        in.x * (cr * cy - sr * sp * sy) - in.y * (sr * cy + cr * sp * sy) + in.z * (cp * sy),
        in.x * (cr * sy + sr * sp * cy) + in.y * (sr * sy - cr * sp * cy) - in.z * (cp * cy),
        in.x * (sr * cp) + in.y * (cr * cp) + in.z * sp
    };
}

inline float clamp(float val, float lo, float hi) {
    return val < lo ? lo : (val > hi ? hi : val);
}

inline float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline int random_int(int min, int max) {
    static std::mt19937 rng(std::random_device{}());
    return std::uniform_int_distribution<int>(min, max)(rng);
}

} // namespace cs2::math
