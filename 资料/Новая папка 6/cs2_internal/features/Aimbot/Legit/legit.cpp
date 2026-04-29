#include "legit.h"
#include "../../../sdk/utils/render/render.h"
#include "../../../Bind System/key.h"
#include "../../engine prediction/eprediction.hpp"
#include "../Rage/Wall Penetrate/auto_wall.h"
#include "../../../sdk/entity/entity.h"
bool triggercock;
void c_legitbot::triggerbot(c_user_cmd* user_cmd, c_cs_player_pawn* best_target, legit_settings sets, int hit_chance, vec3_t local_eye_pos, vec3_t best_head, c_base_player_weapon* weapon)
{
    if (config.legit.triggerbot || g_key_handler->is_pressed(config.legit.triggerbot_keybind, config.legit.triggerbot_keybind_style)) {
        if (triggercock)
            return;
        static auto last_trigger_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto ms_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_trigger_time).count();

        if (!best_target->is_alive() || !best_target->is_enemy())
            return;
        auto weapon_data = weapon->get_weapon_data();
        if (!weapon_data)
            return;
        if (config.legit.auto_scope && (weapon_data->weapon_type() == WEAPONTYPE_SNIPER_RIFLE) && !(user_cmd->buttons.button_state & IN_ZOOM) && !g_ctx.local->scoped()/* && !interfaces::cvar->get_by_name("weapon_accuracy_nospread")->get_bool()*/)
        {
            if (legit_debug) debug_text(WHITE_COLOR, "[legitbot] Forcing zoom for sniper\n");
            user_cmd->buttons.button_state |= IN_ATTACK2;
        }
        auto_stop_legit(user_cmd, g_ctx.local, weapon,/* interfaces::cvar->get_by_name("weapon_accuracy_nospread")->get_bool()*/ false);

        if (hit_chance >= sets.triggerbot_hitchance) {
            bool trigger_should_fire = false;

            if (config.legit.autowall) {
                penetration_data_t trigger_pen_data{};
                const bool override_active = g_key_handler->is_pressed(sets.min_damage_keybind, sets.min_damage_keybind_style);
                const int required_dmg = override_active ? sets.min_damage_override : sets.min_damage;

                if (g_auto_wall->fire_bullet(local_eye_pos, best_head, best_target, weapon_data, trigger_pen_data)) {
                    if (trigger_pen_data.m_damage >= required_dmg)
                        trigger_should_fire = true;
                }
            }
            else {
                if (is_player_visible(g_ctx.local, best_target))
                    trigger_should_fire = true;
            }

            if (trigger_should_fire && ms_since_last >= sets.triggerbot_delay_betwenn_shots) {
                interfaces::engine_client->client_cmd_unrestricted("+attack;-attack");
                last_trigger_time = now;
            }
        }
    }
}

void c_legitbot::legitbot(c_user_cmd* user_cmd)
{
    if (!interfaces::engine_client->is_connected() || !interfaces::engine_client->is_in_game())
        return;

    if (!g_ctx.local || !g_ctx.local_controller || !g_ctx.local->is_alive())
        return;

    if (!config.legit.enable)
        return;

    int def_idx = get_active_weapon_def_index();
    if (def_idx == 0)
        return;

    weapon_category_t category = get_weapon_category_by_defindex(def_idx);
    legit_settings sets = get_legit_settings(category);

    vec3_t original_angles = {
        user_cmd->pb.base().viewangles().x(),
        user_cmd->pb.base().viewangles().y(),
        user_cmd->pb.base().viewangles().z()
    };

    vec3_t local_eye_pos = g_ctx.local->get_eye_pos();

    auto player_entities = entity::get("CCSPlayerController");
    if (player_entities.empty())
        return;

    float best_fov = FLT_MAX;
    vec3_t best_angle{}, best_head{};
    c_cs_player_pawn* best_target = nullptr;

    for (auto player_entity : player_entities) {
        auto controller = reinterpret_cast<c_cs_player_controller*>(player_entity);
        if (!controller || controller == g_ctx.local_controller)
            continue;

        auto handle = controller->pawn();
        if (!handle.is_valid())
            continue;

        auto pawn = reinterpret_cast<c_cs_player_pawn*>(interfaces::entity_system->get_base_entity(handle.get_entry_index()));
        if (!pawn || pawn == g_ctx.local || !pawn->is_alive() || !pawn->is_enemy() || pawn->gungame_immunity() > 0)
            continue;

        if (config.legit.aim_checks[4]) {
            if (!(pawn->flags() & FL_ONGROUND))
                continue;
        }

        vec3_t head_pos = pawn->get_bone_position(6);
        if (head_pos.is_zero())
            continue;

        vec3_t angle_to_target = math::calculate_angles(local_eye_pos, head_pos);
        float fov = math::get_fov2vec(original_angles, angle_to_target);
        if (fov > sets.fov || fov >= best_fov)
            continue;

        //if (!config.legit.autowall && !is_player_visible(g_ctx.local, pawn))
        //    continue;
        if (!is_visible(pawn, head_pos))
            continue;
        best_fov = fov;
        best_head = head_pos;
        best_angle = math::calculate_angles(local_eye_pos, head_pos);
        best_target = pawn;
    }

    if (!best_target)
        return;

    math::normalize_angles(best_angle);

    c_base_player_weapon* weapon = g_ctx.local->get_active_weapon();
    if (!weapon)
        return;

    auto weapon_data = weapon->get_weapon_data();
    if (!weapon_data)
        return;

    int hc = calc_hc(best_target, best_angle, weapon, weapon_data, /*interfaces::cvar->get_by_name("weapon_accuracy_nospread")->get_bool()*/ false);

    penetration_data_t pen_data{};
    if (config.legit.autowall) {
        const bool override_active = g_key_handler->is_pressed(sets.min_damage_keybind, sets.min_damage_keybind_style);
        const int required_dmg = override_active ? sets.min_damage_override : sets.min_damage;

        if (!g_auto_wall->fire_bullet(local_eye_pos, best_head, best_target, weapon_data, pen_data))
            return;

        if (pen_data.m_damage < required_dmg)
            return;
    }

    vec3_t delta = best_angle - original_angles;
    math::normalize_angles(delta);

    if (sets.enable_rcs && g_ctx.local->shots_fired() >= sets.rcs_after_shots) {
        vec3_t punch = get_removed_aim_punch_angle(g_ctx.local);
        delta -= punch;
    }

    float fov_to_target = math::get_fov2vec(original_angles, best_angle);
    float silent_percent = 50.f - ((sets.fov - 1.f) / (30.f - 1.f)) * (50.f - 20.f);
    silent_percent = std::clamp(silent_percent, 20.f, 50.f);

    float silent_fov_threshold = sets.fov * (silent_percent / 100.f);
    bool silent_phase = fov_to_target <= silent_fov_threshold;


    if (silent_phase) {
        for (int i = 0; i < user_cmd->pb.input_history_size(); i++) {
            auto tick = user_cmd->pb.mutable_input_history(i);
            if (tick) {
                tick->mutable_view_angles()->set_x(best_angle.x);
                tick->mutable_view_angles()->set_y(best_angle.y);
                tick->mutable_view_angles()->set_z(best_angle.z);
            }
        }
        if(config.legit.triggerbot || g_key_handler->is_pressed(config.legit.triggerbot_keybind, config.legit.triggerbot_keybind_style))
            if (hc >= sets.triggerbot_hitchance)
            {
                triggercock = true;
                interfaces::engine_client->client_cmd_unrestricted("+attack;-attack");
                triggercock = false;
            }
                
    }
    else {
        bool should_aim =
            config.legit.aimlock ||
            g_key_handler->is_pressed(config.legit.aimlock_keybind, config.legit.aimlock_keybind_style) ||
            (user_cmd->buttons.button_state & IN_ATTACK);

        if (should_aim) {
            vec3_t final_angle = (config.legit.aimlock || g_key_handler->is_pressed(config.legit.aimlock_keybind, config.legit.aimlock_keybind_style))
                ? original_angles + delta
                : original_angles + delta / (sets.smooth * 7.5f);

            auto now = std::chrono::steady_clock::now();
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            float time = static_cast<float>(now_ms) / 1000.0f;

            float jitter = std::sin(time * 10.0f) * 0.25f;
            float jitter_x = std::cos(time * 9.3f) * 0.15f;

            final_angle.x += jitter_x;
            final_angle.y += jitter;

            if (config.legit.aim_checks[0]) {
                if (category == weapon_category_t::scout || category == weapon_category_t::auto_sniper) {
                    if (g_ctx.local->scoped() != 1)
                        return;
                }
            }

            if (config.legit.aim_checks[1] || config.legit.aim_checks[2]) {
                if (g_ctx.local->flash_duration() > 30.f)
                    return;
            }

            if (config.legit.aim_checks[3]) {
                if (!(g_ctx.local->flags() & FL_ONGROUND))
                    return;
            }

            math::clamp_angles(final_angle);
            interfaces::csgo_input->set_view_angles(final_angle);
        }
    }

    g_legitbot->triggerbot(user_cmd, best_target, sets, hc, local_eye_pos, best_head, weapon);
}


void c_legitbot::draw_fov_circle()
{
    if (!config.legit.enable)
		return;
    if (!config.legit.draw_fov)
        return;
    if (!interfaces::engine_client->is_in_game() && !interfaces::engine_client->is_connected())
        return;
    if (!g_ctx.local)
        return;
    if (!g_ctx.local->is_alive())
        return;
    int local_weapon_def_index = get_active_weapon_def_index();
    if (!local_weapon_def_index)
        return;

    weapon_category_t category = get_weapon_category_by_defindex(local_weapon_def_index);


    struct legit_settings {
        float fov;
        float smooth;
        float delay;
        float humanize_float;
        float rcs_smooth;
        float rcs_fov;
        int rcs_after_shots;
    };

    legit_settings settings;

    if (category == weapon_category_t::pistols)
        settings = {
            static_cast<float>(config.legit.fov_pistols),
            static_cast<float>(config.legit.smooth_pistols),
            static_cast<float>(config.legit.delay_pistols),
            static_cast<float>(config.legit.humanize_float_pistols),
            static_cast<float>(config.legit.rcs_smooth_pistols),
            static_cast<float>(config.legit.rcs_fov_pistols),
            config.legit.rcs_after_shots_pistols
    };
    else if (category == weapon_category_t::deagle)
        settings = {
            static_cast<float>(config.legit.fov_deagle),
            static_cast<float>(config.legit.smooth_deagle),
            static_cast<float>(config.legit.delay_deagle),
            static_cast<float>(config.legit.humanize_float_deagle),
            static_cast<float>(config.legit.rcs_smooth_deagle),
            static_cast<float>(config.legit.rcs_fov_deagle),
            config.legit.rcs_after_shots_deagle
    };
    else if (category == weapon_category_t::heavy)
        settings = {
            static_cast<float>(config.legit.fov_heavy),
            static_cast<float>(config.legit.smooth_heavy),
            static_cast<float>(config.legit.delay_heavy),
            static_cast<float>(config.legit.humanize_float_heavy),
            static_cast<float>(config.legit.rcs_smooth_heavy),
            static_cast<float>(config.legit.rcs_fov_heavy),
            config.legit.rcs_after_shots_heavy
    };
    else if (category == weapon_category_t::rifles)
        settings = {
            static_cast<float>(config.legit.fov_rifles),
            static_cast<float>(config.legit.smooth_rifles),
            static_cast<float>(config.legit.delay_rifles),
            static_cast<float>(config.legit.humanize_float_rifles),
            static_cast<float>(config.legit.rcs_smooth_rifles),
            static_cast<float>(config.legit.rcs_fov_rifles),
            config.legit.rcs_after_shots_rifles
    };
    else if (category == weapon_category_t::auto_sniper)
        settings = {
            static_cast<float>(config.legit.fov_auto),
            static_cast<float>(config.legit.smooth_auto),
            static_cast<float>(config.legit.delay_auto),
            static_cast<float>(config.legit.humanize_float_auto),
            static_cast<float>(config.legit.rcs_smooth_auto),
            static_cast<float>(config.legit.rcs_fov_auto),
            config.legit.rcs_after_shots_auto
    };
    else if (category == weapon_category_t::scout)
        settings = {
            static_cast<float>(config.legit.fov_scout),
            static_cast<float>(config.legit.smooth_scout),
            static_cast<float>(config.legit.delay_scout),
            static_cast<float>(config.legit.humanize_float_scout),
            static_cast<float>(config.legit.rcs_smooth_scout),
            static_cast<float>(config.legit.rcs_fov_scout),
            config.legit.rcs_after_shots_scout
    };
    else
        return;



    if (!config.legit.enable)
        return;



    float fov_deg;
    int shot_fired = g_ctx.local->shots_fired();
    if (shot_fired >= settings.rcs_after_shots)
        fov_deg = settings.rcs_fov;
    else
        fov_deg = settings.fov;
    if (fov_deg <= 0.0f)
        return;

    float screen_w = (float)g_render->m_screen_size.x;
    float screen_h = (float)g_render->m_screen_size.y;
    vec3_t screen_center = { screen_w / 2.0f, screen_h / 2.0f };
    float radius;
    if (!g_ctx.local)
        return;

    if (!g_ctx.local->get_active_weapon())
        return;

    if (g_ctx.local->get_active_weapon()->zoom_level() == 0)
        radius = tanf(deg2rad(fov_deg / 2.0f)) / tanf(deg2rad(config.extra.desiredfov / 2.0f)) * (screen_w / 2.0f);
    else if(g_ctx.local->get_active_weapon()->zoom_level() == 1)
        radius = tanf(deg2rad(fov_deg / 2.0f)) / tanf(deg2rad(config.extra.desired_fov_zoom_level_1 / 2.0f)) * (screen_w / 2.0f);
    else
        radius = tanf(deg2rad(fov_deg / 2.0f)) / tanf(deg2rad(config.extra.desired_fov_zoom_level_2 / 2.0f)) * (screen_w / 2.0f);

    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    vec3_t screen_center1337 = { screen_w / 2.0f, screen_h / 2.0f, 0.0f };
    draw->AddCircle(to_imvec2(screen_center1337), radius, IM_COL32(255, 255, 255, 150), 64, 1.5f);

}
