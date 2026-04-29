#include "skins.h"

//static bool is_paint_kit_for_weapon(c_paint_kit* paint_kit, const char* weapon_id)
//{
//    auto path = "panorama/images/econ/default_generated/" + std::string(weapon_id) + "_" + paint_kit->m_name + "_light_png.vtex_c";
//    return interfaces::file_system->exists(path.c_str(), "GAME");
//}
void c_skins::dump() {
    auto item_schema = interfaces::client->get_econ_item_system()->get_econ_item_schema();
    if (!item_schema) {
        debug_text(WHITE_COLOR, "Failed to get item schema.\n");
        return;
    }

    if (!m_dumped_items.empty()) {
        debug_text(WHITE_COLOR, "Items already dumped. Skipping.\n");
        return;
    }

    const auto& vec_items = item_schema->m_sorted_item_definition_map;
    const auto& paint_kits = item_schema->m_paint_kits;
    const auto& alternate_icons = item_schema->m_alternate_icons_map;

    debug_text(WHITE_COLOR, "Starting item dump. Total items: %d\n", vec_items.m_size);

    for (const auto& it : vec_items) {
        c_econ_item_definition* item = it.m_value;
        if (!item) {
            debug_text(WHITE_COLOR, "Skipped null item definition.\n");
            continue;
        }

        const bool is_weapon = item->is_weapon();
        const bool is_agent = item->is_agent();
        const bool is_knife = item->is_knife(true);
        const bool is_glove = item->is_glove(true);

        const char* item_base_name = item->m_item_base_name;
        if (!item_base_name || item_base_name[0] == '\0') {
            debug_text(WHITE_COLOR, "Skipped item with empty name.\n");
            continue;
        }

        const char* localized_name = interfaces::localize->find_key(item_base_name);
        if (!localized_name) {
            debug_text(WHITE_COLOR, "Failed to localize item name: %s\n", item_base_name);
            continue;
        }

        if (is_agent) {
            m_agents.m_dumped_agent_models.emplace_back(item->get_model_name());
            m_agents.m_dumped_agent_name.emplace_back(localized_name);
            debug_text(WHITE_COLOR, "Agent: %s\n", localized_name);
        }

        if (is_knife) {
            m_knives.m_dumped_knife_models.emplace_back(item->get_model_name());
            m_knives.m_dumped_knife_name.emplace_back(localized_name);
            debug_text(WHITE_COLOR, "Knife: %s\n", localized_name);
        }

        if (is_glove) {
            m_gloves.m_dumped_glove_models.emplace_back(item->get_model_name());
            m_gloves.m_dumped_glove_name.emplace_back(localized_name);
            m_gloves.m_glove_idxs.emplace_back(item->m_definition_index);
            debug_text(WHITE_COLOR, "Glove: %s\n", localized_name);
        }

        if (!is_weapon && !is_knife && !is_glove)
            continue;

        c_dumped_item dumped_item{};
        dumped_item.m_name = localized_name;
        dumped_item.m_def_index = item->m_definition_index;
        dumped_item.m_rarity = item->m_item_rarity;

        if (is_knife || is_glove)
            dumped_item.m_unusual_item = true;

        int valid_skins = 0;

        for (const auto& kit_it : paint_kits) {
            c_paint_kit* paint_kit = kit_it.m_value;
            if (!paint_kit || paint_kit->m_id == 0 || paint_kit->m_id == 9001)
                continue;

            const char* skin_name = interfaces::localize->find_key(paint_kit->m_description_tag);
            if (!skin_name || skin_name[0] == '\0')
                continue;

            c_dumped_skins dumped_skin;
            dumped_skin.m_name = skin_name;
            dumped_skin.m_id = paint_kit->m_id;
            dumped_skin.m_rarity = paint_kit->m_rarity;

            dumped_item.m_dumped_skins.emplace_back(dumped_skin);
            ++valid_skins;
        }

        if (!dumped_item.m_dumped_skins.empty() && is_weapon) {
            std::sort(
                dumped_item.m_dumped_skins.begin(),
                dumped_item.m_dumped_skins.end(),
                [](const c_dumped_skins& a, const c_dumped_skins& b) {
                    return a.m_rarity > b.m_rarity;
                }
            );
        }

        m_dumped_items.emplace_back(dumped_item);
        debug_text(WHITE_COLOR, "Weapon: %s | ID: %d | Skins: %d\n", localized_name, dumped_item.m_def_index, valid_skins);
    }

    debug_text(WHITE_COLOR, "Item dump finished. Total dumped: %d\n", m_dumped_items.size());
}


void c_skins::gloves(c_base_view_model* vm)
{
    c_cs_player_pawn* local_player = g_ctx.local;
    if (m_gloves.m_dumped_glove_models.empty())
        return;

    auto item_schema = interfaces::client->get_econ_item_system()->get_econ_item_schema();
    if (!item_schema)
        return;

    c_econ_item_view* econ_glove = &local_player->econ_gloves();
	//int currnet_model = 0;
    int currnet_model = m_gloves.m_glove_idxs[config.extra.glove_selected_paintkit];
    int model_skin = config.extra.m_selected_skin_glove;

    static int last_skin = -1;
    static float last_spawn_time = -1.f;

    if (econ_glove->item_definition_index() != currnet_model || local_player->m_spawn_time_index() != last_spawn_time)
        econ_glove->item_definition_index() = currnet_model;

    if (model_skin != last_skin && model_skin > 0 || local_player->m_spawn_time_index() != last_spawn_time) {
        static auto create_new_paint_kit = reinterpret_cast<c_paint_kit * (__fastcall*)(c_econ_item_view*)>(utils::find_pattern(g_modules.client, XOR_STR("48 89 5C 24 10 56 48 83 EC 20 48 8B 01 FF 50 10 48 8B 1D ? ? ? ?")));

        auto paint_kit = create_new_paint_kit(econ_glove);
        if (paint_kit && paint_kit->m_name)
        {
            last_skin = model_skin;

            auto current_paint_kit = item_schema->m_paint_kits.find_by_key(model_skin);
            if (current_paint_kit.has_value())
                paint_kit->m_name = current_paint_kit.value()->m_name;
        }

        econ_glove->initialized() = true;
        local_player->set_body_group();
        local_player->need_to_reapply_gloves() = true;

        last_spawn_time = local_player->m_spawn_time_index();
    }
}
void c_skins::agents(int stage) {
    auto local_player = g_ctx.local;
    if (!local_player || stage != 6)
        return;

    int team_num = local_player->team_num();
    const bool is_ct = team_num == 3, is_t = team_num == 2;
    if (config.extra.custom_models_t && is_t)
    {
        if (m_model_path_t.find(("game/csgo/")) == std::string::npos)
            return;
        interfaces::m_res_manager->PreCache((m_model_path_t.substr(m_model_path_t.find(("game/csgo/")) + 10)).c_str());
        g_ctx.local->set_model((m_model_path_t.substr(m_model_path_t.find(("game/csgo/")) + 10)).c_str());
    }
    if (config.extra.custom_models_ct && is_ct)
    {
        if (m_model_path_ct.find(("game/csgo/")) == std::string::npos)
            return;
        interfaces::m_res_manager->PreCache((m_model_path_ct.substr(m_model_path_ct.find(("game/csgo/")) + 10)).c_str());
        g_ctx.local->set_model((m_model_path_ct.substr(m_model_path_ct.find(("game/csgo/")) + 10)).c_str());
    }

        //if (is_ct && config.extra.model_ct && !config.extra.custom_models_ct)
        //    local_player->set_model(m_agents.m_dumped_agent_models.at(config.extra.m_selected_ct));
        //else if (is_t && config.extra.model_t && !config.extra.custom_models_t)
        //    local_player->set_model(m_agents.m_dumped_agent_models.at(config.extra.m_selected));
}
void c_skins::knifes(int stage)
{
    if (!g_ctx.local || m_knives.m_dumped_knife_models.empty())
        return;

    c_player_weapon_services* weapon_service = g_ctx.local->weapon_services();
    if (!weapon_service)
        return;

    c_cs_player_view_model_services* view_model_service = g_ctx.local->view_model_services();
    if (!view_model_service)
        return;

    c_base_view_model* view_model = reinterpret_cast<c_base_view_model*>(interfaces::entity_system->get_base_entity(view_model_service->view_model().get_entry_index()));
    if (!view_model)
        return;

    if (stage == 6 && config.extra.glove_changer)
        gloves(view_model);

    c_network_utl_vector<c_base_handle>& my_weapons = weapon_service->my_weapons();

    for (auto i = 0; i < my_weapons.m_size; i++)
    {
        c_econ_entity* weapon = reinterpret_cast<c_econ_entity*>(interfaces::entity_system->get_base_entity(my_weapons.m_elements[i].get_entry_index()));
        if (!weapon)
            continue;

        c_attribute_container* attribute_manager = weapon->attribute_manager();
        if (!attribute_manager)
            continue;

        c_econ_item_view* item = attribute_manager->item();
        if (!item)
            continue;

        c_econ_item_definition* item_defenition = item->get_static_data();
        if (!item_defenition)
            continue;

        auto weapon_scene = weapon->game_scene_node();
        if (!weapon_scene)
            continue;

        auto view_model_scene = view_model->game_scene_node();
        if (!view_model_scene)
            continue;

        auto item_schema = interfaces::client->get_econ_item_system()->get_econ_item_schema();
        if (!item_schema)
            return;

        bool is_knife = item_defenition->is_knife(false);

        int current_weapon = get_skin_config(item->item_definition_index());
        if (!is_knife && current_weapon == CONFIG_UNKNOWN)
            continue;

        static bool knife_set = false;
        switch (current_weapon)
        {
        case CONFIG_BAYONET:
        case CONFIG_KNIFECSS:
        case CONFIG_KNIFEFLIP:
        case CONFIG_KNIFEGUT:
        case CONFIG_KNIFEKARAMBIT:
        case CONFIG_KNIFEM9BAYONET:
        case CONFIG_KNIFETACTICAL:
        case CONFIG_KNIFEFALCHION:
        case CONFIG_KNIFESURVIVALBOWIE:
        case CONFIG_KNIFEBUTTERFLY:
        case CONFIG_KNIFEPUSH:
        case CONFIG_KNIFECORD:
        case CONFIG_KNIFECANIS:
        case CONFIG_KNIFEURSUS:
        case CONFIG_KNIFEGYPSYJACKKNIFE:
        case CONFIG_KNIFEOUTDOOR:
        case CONFIG_KNIFESTILETTO:
        case CONFIG_KNIFEWIDOWMAKER:
        case CONFIG_KNIFESKELETON:
        case CONFIG_KNIFEKUKRI:
            knife_set = true;
            break;
        default:
            knife_set = false;
            break;
        }
        auto current_skin = &config.extra.m_skin_settings[current_weapon];


        /*
		если название ножа в кнайф ченджере совпадает с выбранным селектом для выстановления скина то даем is_knife = true
        */
        static std::vector<const char*> weapon_names{};
        if (weapon_names.size() < g_skins->m_dumped_items.size())
            for (auto& item : g_skins->m_dumped_items)
                weapon_names.emplace_back(item.m_name);

        if (knife_set || current_skin->m_paint_kit > 0) {
            int paint_kit_to_apply = current_skin->m_paint_kit;

            // Если нож и выбран ручной скин — применяем его
            if (knife_set)
                paint_kit_to_apply = config.extra.m_selected_skin_knife;

            auto paint_kit = item_schema->m_paint_kits.find_by_key(paint_kit_to_apply);
            bool use_old_model = paint_kit.has_value() && *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(paint_kit.value()) + 0xB2);

            uint64_t mesh_mask = 1 /*+ static_cast<uint64_t>(use_old_model)*/;
			debug_text(WHITE_COLOR, "Applying skin: %s | Paint Kit: %d | Knife: %d\n", item_defenition->m_item_base_name, paint_kit_to_apply, knife_set);
            weapon->fallback_stat_trak() = 1337;
            weapon->fallback_paint_kit() = paint_kit_to_apply;
            item->item_id_high() = -1;
            weapon_scene->set_mesh_group_mask(mesh_mask);
            view_model_scene->set_mesh_group_mask(mesh_mask);
        }
        if (is_knife && stage == 6) {
            int team_num = g_ctx.local->team_num();
            const bool is_ct = team_num == 3, is_t = team_num == 2;
            static int selectedKnife = 0;
            if (is_ct && config.extra.knife_changer_ct)
                selectedKnife = config.extra.m_selected_knife_t;
            else if (is_t && config.extra.knife_changer_t)
				selectedKnife = config.extra.m_selected_knife_ct;

            const auto& model_path = m_knives.m_dumped_knife_models.at(selectedKnife);
            int model_index = m_knives.m_knife_idxs.at(selectedKnife);
            debug_text(WHITE_COLOR, "IS KNIFE 1\n");

            weapon->set_model(model_path);
            item->item_definition_index() = model_index;

            if (view_model->weapon() == weapon->get_handle()) {
                view_model->set_model(model_path);
				debug_text(WHITE_COLOR, "IS KNIFE 2\n");
                weapon->fallback_paint_kit() = config.extra.m_selected_skin_knife;
                weapon->fallback_stat_trak() = config.extra.m_selected_skin_knife;
                item->item_id_high() = -1;
                weapon_scene->set_mesh_group_mask(1);
                view_model_scene->set_mesh_group_mask(1);
                C_CSGOViewModel2* fixed_view_model = reinterpret_cast<C_CSGOViewModel2*>(view_model);
                if (fixed_view_model && fixed_view_model->pAnimationGraphInstance)
                    fixed_view_model->pAnimationGraphInstance->pAnimGraphNetworkedVariables = nullptr;
            }
        }
    }
}

c_models_data c_skins::custom_models(const std::string& folder)
{
    c_models_data data;

    if (!std::filesystem::is_directory(folder))
        return data;

    for (const auto& entry : std::filesystem::directory_iterator(folder))
    {
        if (!entry.is_directory())
            continue;

        std::string name = entry.path().filename().string();
        std::unordered_map< std::string, std::vector< std::string > > packs;

        for (const auto& pack : std::filesystem::directory_iterator(entry.path()))
        {
            if (!pack.is_directory())
                continue;

            std::vector< std::string > model_list;

            for (const auto& model : std::filesystem::directory_iterator(pack.path()))
            {
                if (model.is_regular_file() && model.path().extension() == (".vmdl_c"))
                {
                    std::string model_name = model.path().filename().string();


                    if (model_name.find(("_arms")) == std::string::npos && model_name.find(("_arm")) == std::string::npos)
                    {
                        if (model_name.ends_with((".vmdl_c")))
                            model_name.erase(model_name.size() - 2);

                        model_list.push_back(model_name);
                    }
                }
            }

            if (!model_list.empty())
                packs[pack.path().filename().string()] = model_list;
        }

        if (!packs.empty())
            data[name] = packs;
    }

    return data;
}