#pragma once
#include "../utils/sdk.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <atomic>

namespace cs2::vischeck {

// ── Structures ─────────────────────────────────────────────

struct TriangleCombined {
    Vector3 v0, v1, v2;
    TriangleCombined() = default;
    TriangleCombined(const Vector3& a, const Vector3& b, const Vector3& c)
        : v0(a), v1(b), v2(c) {}

    struct AABB {
        Vector3 min, max;
        bool ray_intersects(const Vector3& o, const Vector3& d) const;
    };
    AABB compute_aabb() const;
};

struct BVHNode {
    TriangleCombined::AABB bounds;
    std::unique_ptr<BVHNode> left, right;
    std::vector<TriangleCombined> triangles;
    bool is_leaf() const { return !left && !right; }
};

// ── Optimized geometry loader (.opt) ───────────────────────

struct OptGeometry {
    std::vector<std::vector<TriangleCombined>> meshes;
    bool load(const std::string& path);
};

// ── Main visibility check ─────────────────────────────────

class VisCheck {
public:
    bool load_map(const std::string& opt_path);
    bool is_visible(const Vector3& eye, const Vector3& target);

private:
    OptGeometry m_geo;
    std::vector<std::unique_ptr<BVHNode>> m_roots;

    std::unique_ptr<BVHNode> build_bvh(const std::vector<TriangleCombined>& tris);
    bool intersect_bvh(const BVHNode* node, const Vector3& o, const Vector3& d,
                       float max_dist, float& hit_dist) const;
    bool ray_tri_intersect(const Vector3& o, const Vector3& d,
                           const TriangleCombined& tri, float& t) const;
};

} // namespace cs2::vischeck

// Global pointer (defined in vischeck.cpp, used by aimbot visible check)
extern std::atomic<cs2::vischeck::VisCheck*> g_pVisCheck;
