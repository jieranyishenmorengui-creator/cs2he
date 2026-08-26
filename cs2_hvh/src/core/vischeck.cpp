#include "vischeck.h"
#include <algorithm>
#include <limits>
#include <cmath>
#include <cstring>
#include <iostream>

// Global instance pointer (declared in main.cpp, defined here)
std::atomic<cs2::vischeck::VisCheck*> g_pVisCheck{nullptr};

namespace cs2::vischeck {

// ═══════════════════════════════════════════════════════════════
//  AABB
// ═══════════════════════════════════════════════════════════════

bool TriangleCombined::AABB::ray_intersects(const Vector3& o, const Vector3& d) const {
    float tmin = std::numeric_limits<float>::lowest();
    float tmax = std::numeric_limits<float>::max();

    const float* oa = &o.x;
    const float* da = &d.x;
    const float* mina = &min.x;
    const float* maxa = &max.x;

    for (int i = 0; i < 3; ++i) {
        float inv = 1.0f / da[i];
        float t0 = (mina[i] - oa[i]) * inv;
        float t1 = (maxa[i] - oa[i]) * inv;
        if (inv < 0) std::swap(t0, t1);
        tmin = std::max(tmin, t0);
        tmax = std::min(tmax, t1);
    }
    return tmax >= tmin && tmax >= 0;
}

TriangleCombined::AABB TriangleCombined::compute_aabb() const {
    AABB aabb;
    aabb.min.x = std::min({v0.x, v1.x, v2.x});
    aabb.min.y = std::min({v0.y, v1.y, v2.y});
    aabb.min.z = std::min({v0.z, v1.z, v2.z});
    aabb.max.x = std::max({v0.x, v1.x, v2.x});
    aabb.max.y = std::max({v0.y, v1.y, v2.y});
    aabb.max.z = std::max({v0.z, v1.z, v2.z});
    return aabb;
}

// ═══════════════════════════════════════════════════════════════
//  .opt loader
// ═══════════════════════════════════════════════════════════════

bool OptGeometry::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    meshes.clear();
    size_t nm;
    f.read(reinterpret_cast<char*>(&nm), sizeof(nm));
    meshes.resize(nm);

    for (size_t m = 0; m < nm; ++m) {
        size_t nt;
        f.read(reinterpret_cast<char*>(&nt), sizeof(nt));
        meshes[m].resize(nt);
        for (size_t t = 0; t < nt; ++t) {
            f.read(reinterpret_cast<char*>(&meshes[m][t].v0), sizeof(Vector3));
            f.read(reinterpret_cast<char*>(&meshes[m][t].v1), sizeof(Vector3));
            f.read(reinterpret_cast<char*>(&meshes[m][t].v2), sizeof(Vector3));
        }
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════
//  BVH
// ═══════════════════════════════════════════════════════════════

static constexpr size_t LEAF_THRESH = 4;

std::unique_ptr<BVHNode> VisCheck::build_bvh(const std::vector<TriangleCombined>& tris) {
    auto node = std::make_unique<BVHNode>();
    if (tris.empty()) return node;

    // Compute bounds
    TriangleCombined::AABB bounds = tris[0].compute_aabb();
    for (size_t i = 1; i < tris.size(); ++i) {
        auto b = tris[i].compute_aabb();
        bounds.min.x = std::min(bounds.min.x, b.min.x);
        bounds.min.y = std::min(bounds.min.y, b.min.y);
        bounds.min.z = std::min(bounds.min.z, b.min.z);
        bounds.max.x = std::max(bounds.max.x, b.max.x);
        bounds.max.y = std::max(bounds.max.y, b.max.y);
        bounds.max.z = std::max(bounds.max.z, b.max.z);
    }
    node->bounds = bounds;

    if (tris.size() <= LEAF_THRESH) {
        node->triangles = tris;
        return node;
    }

    // Split along longest axis
    Vector3 diff = bounds.max - bounds.min;
    int axis = (diff.x > diff.y && diff.x > diff.z) ? 0 :
               (diff.y > diff.z) ? 1 : 2;

    auto sorted = tris;
    std::sort(sorted.begin(), sorted.end(), [axis](const TriangleCombined& a, const TriangleCombined& b) {
        auto ba = a.compute_aabb(), bb = b.compute_aabb();
        float ca = ((&ba.min.x)[axis] + (&ba.max.x)[axis]) * 0.5f;
        float cb = ((&bb.min.x)[axis] + (&bb.max.x)[axis]) * 0.5f;
        return ca < cb;
    });

    size_t mid = sorted.size() / 2;
    node->left  = build_bvh({sorted.begin(), sorted.begin() + mid});
    node->right = build_bvh({sorted.begin() + mid, sorted.end()});
    return node;
}

bool VisCheck::intersect_bvh(const BVHNode* node, const Vector3& o, const Vector3& d,
                              float max_dist, float& hit_dist) const {
    if (!node->bounds.ray_intersects(o, d))
        return false;

    if (node->is_leaf()) {
        bool hit = false;
        for (auto& tri : node->triangles) {
            float t;
            if (ray_tri_intersect(o, d, tri, t) && t < max_dist && t < hit_dist) {
                hit_dist = t;
                hit = true;
            }
        }
        return hit;
    }

    bool h = false;
    if (node->left)  h |= intersect_bvh(node->left.get(),  o, d, max_dist, hit_dist);
    if (node->right) h |= intersect_bvh(node->right.get(), o, d, max_dist, hit_dist);
    return h;
}

// ═══════════════════════════════════════════════════════════════
//  Möller–Trumbore ray-triangle intersection
// ═══════════════════════════════════════════════════════════════

bool VisCheck::ray_tri_intersect(const Vector3& o, const Vector3& d,
                                  const TriangleCombined& tri, float& t) const {
    constexpr float EPS = 1e-7f;
    Vector3 e1 = tri.v1 - tri.v0;
    Vector3 e2 = tri.v2 - tri.v0;
    Vector3 h = d.cross(e2);
    float a = e1.dot(h);
    if (a > -EPS && a < EPS) return false;

    float f = 1.0f / a;
    Vector3 s = o - tri.v0;
    float u = f * s.dot(h);
    if (u < 0 || u > 1) return false;

    Vector3 q = s.cross(e1);
    float v = f * d.dot(q);
    if (v < 0 || u + v > 1) return false;

    t = f * e2.dot(q);
    return t > EPS;
}

// ═══════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════

bool VisCheck::load_map(const std::string& opt_path) {
    if (!m_geo.load(opt_path)) {
        std::cerr << "[VisCheck] Failed to load " << opt_path << std::endl;
        return false;
    }
    m_roots.clear();
    for (auto& mesh : m_geo.meshes)
        m_roots.push_back(build_bvh(mesh));
    // printf("[VisCheck] Loaded %s (%zu meshes, %zu BVH roots)\n",
    // opt_path.c_str(), m_geo.meshes.size(), m_roots.size());
    return true;
}

bool VisCheck::is_visible(const Vector3& eye, const Vector3& target) {
    Vector3 dir = target - eye;
    float dist = dir.length();
    if (dist < 0.1f) return true;
    dir = dir / dist;

    float hit = std::numeric_limits<float>::max();
    for (auto& root : m_roots) {
        if (intersect_bvh(root.get(), eye, dir, dist, hit) && hit < dist)
            return false;
    }
    return true;
}

} // namespace cs2::vischeck
