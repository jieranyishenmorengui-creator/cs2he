#include "esp.h"
#include "aimbot.h"
#include "../core/vischeck.h"
#include "../core/memory.h"
#include "../core/offsets.h"
#include "../core/overlay.h"
#include "../core/renderer.h"
#include "../utils/debug_log.h"
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cwchar>

namespace cs2::esp {

using namespace memory;

struct ESPEntity {
    uintptr_t pawn;
    Vector3 origin;
    Vector3 head_pos;
    Vector2 screen_origin;
    Vector2 screen_head;
    int health;
    int team;
    float distance;
    bool valid;
    bool spotted;
    std::string name;
    std::string weapon_name;
    int bone_positions[BoneIndex::MAX_BONES * 2];
    bool bones_valid;
};

// ── Bone connections (matching IMXNOOBX/cs2-external-esp) ────────
static const int BONE_CONNECTIONS[][2] = {
    // Spine (pelvis → spine_1 → spine_2 → chest → neck → head)
    {BoneIndex::PELVIS,    BoneIndex::SPINE_1},
    {BoneIndex::SPINE_1,   BoneIndex::SPINE_2},
    {BoneIndex::SPINE_2,   BoneIndex::CHEST},
    {BoneIndex::CHEST,     BoneIndex::NECK},
    {BoneIndex::NECK,      BoneIndex::HEAD},
    // Left arm (from neck)
    {BoneIndex::NECK,      BoneIndex::SHOULDER_L},
    {BoneIndex::SHOULDER_L,BoneIndex::ELBOW_L},
    {BoneIndex::ELBOW_L,   BoneIndex::HAND_L},
    // Right arm (from neck)
    {BoneIndex::NECK,      BoneIndex::SHOULDER_R},
    {BoneIndex::SHOULDER_R,BoneIndex::ELBOW_R},
    {BoneIndex::ELBOW_R,   BoneIndex::HAND_R},
    // Left leg (from pelvis)
    {BoneIndex::PELVIS,    BoneIndex::HIP_L},
    {BoneIndex::HIP_L,     BoneIndex::KNEE_L},
    {BoneIndex::KNEE_L,    BoneIndex::FOOT_HEEL_L},
    // Right leg (from pelvis)
    {BoneIndex::PELVIS,    BoneIndex::HIP_R},
    {BoneIndex::HIP_R,     BoneIndex::KNEE_R},
    {BoneIndex::KNEE_R,    BoneIndex::FOOT_HEEL_R},
};
static constexpr int NUM_BONE_CONNS = sizeof(BONE_CONNECTIONS) / sizeof(BONE_CONNECTIONS[0]);

// ── Weapon name via item definition index ───────────────────────────
static const char* weapon_id_to_name(uint16_t id) {
    switch (id) {
    case 1:  return "DEAGLE";   case 2:  return "ELITE";
    case 3:  return "FIVESEVEN"; case 4:  return "GLOCK";
    case 7:  return "AK47";     case 8:  return "AUG";
    case 9:  return "AWP";      case 10: return "FAMAS";
    case 11: return "G3SG1";    case 13: return "GALIL";
    case 14: return "M249";     case 16: return "M4A4";
    case 17: return "MAC10";    case 19: return "P90";
    case 23: return "MP5SD";    case 24: return "UMP45";
    case 25: return "XM1014";   case 26: return "BIZON";
    case 27: return "MAG7";     case 28: return "NEGEV";
    case 29: return "SAWEDOFF"; case 30: return "TEC9";
    case 31: return "ZEUS";     case 32: return "P2000";
    case 33: return "MP7";      case 34: return "MP9";
    case 35: return "NOVA";     case 36: return "P250";
    case 38: return "SCAR20";   case 39: return "SG556";
    case 40: return "SSG08";    case 60: return "M4A1S";
    case 61: return "USPS";     case 63: return "CZ75";
    case 64: return "REVOLVER"; default: return "";
    }
}

// Resolve weapon name using a cached entity-list base (avoids re-reading dwEntityList)
static std::string get_weapon_name_cached(uintptr_t pawn, uintptr_t entListBase) {
    using namespace cs2::offsets;
    uintptr_t svc = read<uintptr_t>(pawn + NetVars::m_pWeaponServices);
    if (!svc) return "";
    uint32_t h = read<uint32_t>(svc + NetVars::m_hActiveWeapon);
    if (!h) return "";
    uint32_t idx = h & 0x7FFF;
    if (!idx || !entListBase) return "";

    uintptr_t entry = read<uintptr_t>(entListBase + 8 * (idx >> 9) + 16);
    if (!entry) return "";
    uintptr_t weapon = read<uintptr_t>(entry + 112 * (idx & 0x1FF));
    if (!weapon) return "";

    uint16_t def = read<uint16_t>(weapon + 0x1180 + 0x50 + 0x1BA);
    return weapon_id_to_name(def);
}

// ── Per-entity raw data (no screen coords yet) ────────────────
struct RawEntity {
    uintptr_t pawn;
    Vector3   origin;
    Vector3   headPos;
    int       health;
    int       team;
    float     distance;
        bool      spotted;
    std::string name;
    std::string weapon_name;

    // Cached head bone (used if skeleton is off)
    bool      headFromBone;

    // Skeleton: world-space bone positions (extracted in phase 1)
    int       boneCount;
    Vector3   boneWorld[BoneIndex::MAX_BONES];
};

// ═════════════════════════════════════════════════════════════════════
//  ESP main entry (TWO-PHASE: collect → W2S)
// ═════════════════════════════════════════════════════════════════════
void run(const ESPConfig& cfg) {
    using namespace cs2::renderer;
    if (!cfg.enabled || !overlay::is_ready()) return;
    using namespace cs2::offsets;

    int sw = overlay::get_width();
    int sh = overlay::get_height();

    // 游戏时间(用于插值)
    float g_curtime = 0.f;
    if (g_offsets.dwGlobalVars) {
        uintptr_t gv = read<uintptr_t>(g_offsets.dwGlobalVars);
        if (gv) g_curtime = read<float>(gv + 0x0C);
    }

    // ═════════════════════════════════════════════════════════════
    //  Phase 1: positions direct (fresh every frame),
    //           heavy data from entity cache (throttled)
    // ═════════════════════════════════════════════════════════════
    uintptr_t elb = read<uintptr_t>(g_offsets.dwEntityList);
    uintptr_t local_ctrl = read<uintptr_t>(g_offsets.dwLocalPlayerController);
    if (!local_ctrl) return;
    uint32_t lh = read<uint32_t>(local_ctrl + NetVars::m_hPawn);
    if (!lh) return;
    uintptr_t local_pawn = get_entity_from_handle(lh);
    if (!local_pawn) return;
    uint8_t local_team = read<uint8_t>(local_pawn + NetVars::m_iTeamNum);
    Vector3 local_origin = read<Vector3>(local_pawn + NetVars::m_vOldOrigin);
    Vector3 local_eye = local_origin + read<Vector3>(local_pawn + NetVars::m_vecViewOffset);

    // ESP自己的vischeck缓存 (节流)
    static std::unordered_map<uintptr_t, bool> s_esp_vis;
    static int s_vis_tick = 0;
    bool vis_tick = (++s_vis_tick % 3 == 0); // 每3帧刷新一次射线

    struct RawEntity {
        uintptr_t pawn;
        Vector3   origin;
        Vector3   headPos;
        int       health;
        int       team;
        float     distance;
        bool      spotted;
        std::string name;
        std::string weapon_name;
        int       boneCount;
        Vector3   boneWorld[BoneIndex::MAX_BONES];
    };
    std::vector<RawEntity> rawList;

    for (int i = 1; i < 64; ++i) {
        uintptr_t ch = read<uintptr_t>(elb + 8 * (i >> 9) + 0x10);
        if (!ch) continue;
        uintptr_t controller = read<uintptr_t>(ch + 112 * (i & 0x1FF));
        if (!controller || controller == local_ctrl) continue;

        // Quick existence check: pawn handle
        uint32_t ph = read<uint32_t>(controller + NetVars::m_hPawn);
        if (!ph) continue;

        uintptr_t pawn = 0;
        {
            uint32_t pIdx = ph & 0x7FFF;
            uintptr_t pChunk = read<uintptr_t>(elb + 8 * (pIdx >> 9) + 0x10);
            if (pChunk) pawn = read<uintptr_t>(pChunk + 112 * (pIdx & 0x1FF));
        }
        if (!pawn || pawn == local_pawn) continue;

        // Dormant先查(省后续RPM)
        uintptr_t sn = read<uintptr_t>(pawn + NetVars::m_pGameSceneNode);
        if (!sn || read<uint8_t>(sn + 0x103)) continue;

        if (cfg.team_check) {
            uint8_t team = read<uint8_t>(pawn + NetVars::m_iTeamNum);
            if (team == local_team) continue;
        }

        // 存活+位置
        int hp = read<int32_t>(pawn + NetVars::m_iHealth);
        if (hp <= 0 || hp > 200) continue;
        uint8_t life = read<uint8_t>(pawn + NetVars::m_lifeState);
        if (life != 0) continue;
        Vector3 raw_origin = read<Vector3>(pawn + NetVars::m_vOldOrigin);

        // Head bone raw (for interpolation)
        Vector3 raw_head = raw_origin + Vector3(0, 0, 72.0f);
        if (sn) {
            uintptr_t ba = read<uintptr_t>(sn + NetVars::m_modelState + NetVars::m_pBones);
            if (ba) {
                Vector3 hb = read<Vector3>(ba + BoneIndex::HEAD * 0x20);
                if (hb.length() > 1.0f) raw_head = hb;
            }
        }

        // ── 64→144Hz线性插值(origin+head同步) ──────────────
        struct Interp { Vector3 origin, head; };
        static std::unordered_map<uintptr_t, Interp> s_prev, s_curr;
        static std::unordered_map<uintptr_t, float> s_switch_t;
        auto& cur = s_curr[pawn];
        if ((raw_origin - cur.origin).length() > 0.1f ||
            (raw_head - cur.head).length() > 0.1f) {
            s_prev[pawn] = cur;
            cur = {raw_origin, raw_head};
            s_switch_t[pawn] = g_curtime;
        }
        float t = (g_curtime - s_switch_t[pawn]) / 0.015625f;
        auto& prv = s_prev[pawn];
        bool interp = (t < 1.f) && prv.origin.length() > 0.1f;
        Vector3 origin  = interp ? prv.origin + (cur.origin - prv.origin) * t : cur.origin;
        Vector3 headPos = interp ? prv.head   + (cur.head - prv.head) * t   : cur.head;

        float dist = local_origin.dist_to(origin);
        int team = read<uint8_t>(pawn + NetVars::m_iTeamNum);
        // Visible: ESP自己做射线(节流), 否则用m_bSpotted
        bool visible;
        auto* vc = g_pVisCheck.load();
        if (cfg.esp_vischeck && vc && sn) {
            if (vis_tick) {
                // 多点射线: 头/胸/骨盆任一可见即绿
                uintptr_t ba = read<uintptr_t>(sn + NetVars::m_modelState + NetVars::m_pBones);
                bool any_vis = false;
                if (ba) {
                    static const int SB[] = {
                        BoneIndex::HEAD, BoneIndex::ELBOW_L, BoneIndex::ELBOW_R,
                        BoneIndex::FOOT_HEEL_L, BoneIndex::FOOT_HEEL_R
                    };
                    for (int k = 0; k < 5; ++k) {
                        Vector3 pt = read<Vector3>(ba + SB[k] * 0x20);
                        if (pt.length() > 1.0f && vc->is_visible(local_eye, pt)) { any_vis = true; break; }
                    }
                }
                visible = any_vis;
                s_esp_vis[pawn] = visible;
            } else {
                auto it = s_esp_vis.find(pawn);
                visible = (it != s_esp_vis.end()) ? it->second : false;
            }
        } else {
            visible = read<uint8_t>(pawn + NetVars::m_entitySpottedState + NetVars::m_bSpotted) != 0;
        }

        // Name from controller (batch read)
        std::string entName;
        {
            uint8_t cbuf[0x60];
            if (read(controller + 0x6BC, cbuf, 0x58)) {
                const char* ns = (const char*)(cbuf + 0x38);
                size_t nl = strnlen(ns, 32);
                if (nl > 0) entName.assign(ns, nl);
            }
        }

        // Weapon
        std::string weaponName;
        if (cfg.show_weapon) {
            weaponName = get_weapon_name_cached(pawn, elb);
        }

        // Skeleton bones (direct read when on)
        int boneCount = 0;
        Vector3 boneWorld[30]{};
        if (cfg.show_skeleton && sn) {
            uintptr_t ba = read<uintptr_t>(sn + NetVars::m_modelState + NetVars::m_pBones);
            if (ba) {
                uint8_t raw[30 * 0x20];
                if (read(ba, raw, sizeof(raw))) {
                    boneCount = 30;
                    for (int b = 0; b < 30; ++b)
                        boneWorld[b] = *(Vector3*)(raw + b * 0x20);
                }
            }
        }
        rawList.push_back(RawEntity{
            pawn, origin, headPos, hp, team, dist, visible,
            std::move(entName), std::move(weaponName),
            boneCount, {}
        });
        if (boneCount > 0)
            memcpy(rawList.back().boneWorld, boneWorld, sizeof(Vector3) * boneCount);
    }

    // ═════════════════════════════════════════════════════════════
    //  PHASE 2: Read ViewMatrix NOW (freshest possible), then W2S
    // ═════════════════════════════════════════════════════════════
    ViewMatrix vm = read<ViewMatrix>(g_offsets.dwViewMatrix);

    // Smoothing state (persists across frames)
    static std::unordered_map<uintptr_t, Vector2> s_smoothFoot;
    static std::unordered_map<uintptr_t, Vector2> s_smoothHead;
    float alpha = 1.0f - std::clamp(cfg.smooth_factor, 0.0f, 0.95f);

    std::vector<ESPEntity> entities;

    for (auto& raw : rawList) {
        Vector2 foot, head2d;
        if (!world_to_screen(raw.origin, foot, vm, sw, sh)) continue;
        if (!world_to_screen(raw.headPos, head2d, vm, sw, sh)) continue;

        // ── Smoothing (EMA) ──────────────────────────────────
        if (cfg.smooth_factor > 0.0f) {
            auto itF = s_smoothFoot.find(raw.pawn);
            if (itF != s_smoothFoot.end()) {
                foot.x = itF->second.x + (foot.x - itF->second.x) * alpha;
                foot.y = itF->second.y + (foot.y - itF->second.y) * alpha;
            }
            s_smoothFoot[raw.pawn] = foot;

            auto itH = s_smoothHead.find(raw.pawn);
            if (itH != s_smoothHead.end()) {
                head2d.x = itH->second.x + (head2d.x - itH->second.x) * alpha;
                head2d.y = itH->second.y + (head2d.y - itH->second.y) * alpha;
            }
            s_smoothHead[raw.pawn] = head2d;
        }

        ESPEntity ent;
        ent.pawn          = raw.pawn;
        ent.origin        = raw.origin;
        ent.head_pos      = raw.headPos;
        ent.screen_origin = foot;
        ent.screen_head   = head2d;
        ent.health        = raw.health;
        ent.team          = raw.team;
        ent.distance      = raw.distance;
        ent.valid         = true;
        ent.spotted       = raw.spotted;
        ent.name          = std::move(raw.name);
        ent.weapon_name   = std::move(raw.weapon_name);

        // W2S skeleton bones (+ sanity check to avoid twitching)
        // Initialize all bone positions to -1 (invalid sentinel)
        memset(ent.bone_positions, -1, sizeof(ent.bone_positions));
        ent.bones_valid = false;
        int valid_bones = 0;

        if (raw.boneCount > 0) {
            for (int b = 0; b < raw.boneCount; ++b) {
                Vector3& bp = raw.boneWorld[b];
                // Skip zero-length bones
                if (bp.length() < 0.001f) continue;
                // Reject bones > 500 units from origin (avoids twitch spikes)
                if ((bp - raw.origin).length() > 500.0f) continue;
                Vector2 sp;
                if (world_to_screen(bp, sp, vm, sw, sh)) {
                    // Reject off-screen projections (more than 2× screen size away)
                    if (sp.x < -sw * 2 || sp.x > sw * 3 ||
                        sp.y < -sh * 2 || sp.y > sh * 3) continue;
                    ent.bone_positions[b * 2 + 0] = (int)sp.x;
                    ent.bone_positions[b * 2 + 1] = (int)sp.y;
                    valid_bones++;
                }
            }
            if (valid_bones > 0)
                ent.bones_valid = true;
        }

        entities.push_back(ent);
    }

    // ── Clean up smoothing + cache for dead entities ────────────
    {
        auto prune = [&](auto& map) {
            for (auto it = map.begin(); it != map.end(); ) {
                bool alive = false;
                for (auto& e : entities)
                    if (e.pawn == it->first) { alive = true; break; }
                if (alive) ++it;
                else       it = map.erase(it);
            }
        };
        if (cfg.smooth_factor > 0.0f) { prune(s_smoothFoot); prune(s_smoothHead); }
        prune(s_esp_vis);
    }

    std::sort(entities.begin(), entities.end(),
        [](auto& a, auto& b) { return a.distance > b.distance; });

    // ═════════════════════════════════════════════════════════════
    //  DRAW
    // ═════════════════════════════════════════════════════════════
    for (auto& ent : entities) {
        auto& foot = ent.screen_origin;
        auto& head = ent.screen_head;

        float h  = foot.y - head.y;
        float w  = std::max(h * 0.5f, 1.0f);
        float x  = head.x - w * 0.5f;

        // Visible color when spotted
        bool is_visible = cfg.show_visible_color && ent.spotted;
        Color col = ent.team == local_team ? cfg.team_color :
                    (is_visible ? cfg.visible_color : cfg.enemy_color);
        col.a *= cfg.global_alpha;
        Color shadow = cfg.shadow_color;
        shadow.a *= cfg.shadow_alpha * cfg.global_alpha;

        if (cfg.show_box) {
            switch (cfg.box_type) {
            case 0:
                if (cfg.filled)
                    draw_filled_rect(x, head.y, w, h, Color(col.r, col.g, col.b, 0.15f * cfg.global_alpha));
                draw_rect(x - 1, head.y - 1, w + 2, h + 2, shadow);
                draw_rect(x, head.y, w, h, col);
                break;
            case 1:
                draw_corner_box(x, head.y, w, h, col);
                break;
            case 2:
                draw_rect(x - 1, head.y - 1, w + 2, h + 2, shadow);
                draw_rect(x + 1, head.y + 1, w - 2, h - 2, shadow);
                draw_rect(x, head.y, w, h, col);
                break;
            }
        }

        // Head circle (toggleable)
        if (cfg.show_head_circle)
            draw_circle(head.x, head.y, w / 6.0f, cfg.head_circle_color, 15, cfg.head_circle_thickness);

        if (cfg.show_line)
            draw_line((float)sw * 0.5f, (float)sh, foot.x, foot.y, col, 1.5f);

        if (cfg.show_health)
            draw_health_bar(x, head.y, 4, h, ent.health);

        float ts = cfg.text_scale;

        if (cfg.show_name) {
            // Build label: old char-by-char path (proven)
            std::wstring label;
            if (!ent.name.empty()) {
                label.assign(ent.name.begin(), ent.name.end());
                // CJK chars → pinyin first letter
                for (auto& c : label) {
                    if (c >= 0x4E00 && c <= 0x9FFF)
                        c = pinyin_head(c);
                }
            }
            if (label.empty() || label.find_first_not_of(L" \t\n\r\0") == std::wstring::npos)
                label = L"?";  // fallback: always show something
            float tw = get_text_width(label, ts);
            draw_text_shadow(x + w * 0.5f - tw * 0.5f, head.y - 18,
                             label, Color(1, 1, 1, 0.95f * cfg.global_alpha), ts);
        }

        if (cfg.show_distance) {
            wchar_t buf[32];
            swprintf(buf, 32, L"%.0fm", ent.distance / 39.37f);
            float tw = get_text_width(buf, ts);
            draw_text_shadow(x + w * 0.5f - tw * 0.5f, foot.y + 4,
                             buf, Color(1, 1, 1, 0.85f * cfg.global_alpha), ts);
        }

        if (cfg.show_weapon) {
            std::wstring wname;
            if (!ent.weapon_name.empty())
                wname.assign(ent.weapon_name.begin(), ent.weapon_name.end());
            if (wname.empty())
                wname = L"?";  // always show something
            float tw = get_text_width(wname, ts);
            draw_text_shadow(x + w * 0.5f - tw * 0.5f, foot.y + 16,
                             wname, Color(1, 1, 1, 0.85f * cfg.global_alpha), ts);
        }

        if (cfg.show_skeleton && ent.bones_valid) {
            for (int bc = 0; bc < NUM_BONE_CONNS; ++bc) {
                int b0 = BONE_CONNECTIONS[bc][0];
                int b1 = BONE_CONNECTIONS[bc][1];
                if (b0 >= BoneIndex::MAX_BONES || b1 >= BoneIndex::MAX_BONES) continue;
                // Skip if either bone position is invalid (sentinel -1)
                int x0 = ent.bone_positions[b0 * 2];
                int y0 = ent.bone_positions[b0 * 2 + 1];
                int x1 = ent.bone_positions[b1 * 2];
                int y1 = ent.bone_positions[b1 * 2 + 1];
                if (x0 < 0 || y0 < 0 || x1 < 0 || y1 < 0) continue;
                Color skelCol = cfg.skeleton_color;
                skelCol.a *= cfg.global_alpha;
                draw_line((float)x0, (float)y0, (float)x1, (float)y1,
                          skelCol, cfg.skeleton_thickness);
            }
        }
    }
}

} // namespace cs2::esp