#include "visuals.h"
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include "../../hooks/hooks.h"
// #include "../../players/players.h"
#include "../../utils/memory/patternscan/patternscan.h"
#include "../../utils/memory/gaa/gaa.h"
#include "../../../../external/imgui/imgui.h"
#include "../../interfaces/interfaces.h"
#include "../../config/config.h"
#include "../../menu/menu.h"
using namespace Esp;

LocalPlayerCached cached_local;
std::vector<PlayerCache> cached_players;
static std::unordered_map<int, float> animated_health; // handle index -> animated health

void Visuals::init() {
    viewMatrix.viewMatrix = (viewmatrix_t*)M::getAbsoluteAddress(M::patternScan("client", "48 8D 0D ? ? ? ? 48 C1 E0 06"), 3, 0);
}

void Esp::cache()
{
    if (!I::EngineClient->valid())
        return;

    /*  Old method READ ME
    * @ // @here: manually cache once all existing entitys
        // to avoid issues when injecting mid game and hkAddEnt not called by game on existing Entity's

       int highest_index = I::GameEntity->Instance->GetHighestEntityIndex();
        for (int i = 1; i <= highest_index; i++) {
            auto entity = I::GameEntity->Instance->Get(i);
            if (!entity)
                continue;

            uintptr_t entityPointer = reinterpret_cast<uintptr_t>(entity);

            SchemaClassInfoData_t* entityInfo = nullptr;
            GetSchemaClassInfo(entityPointer, &entityInfo);
            if (!entityInfo) continue;

            if (strcmp(entityInfo->szName, "C_CSPlayerPawn") == 0) {
                bool exists = std::any_of(Players::pawns.begin(), Players::pawns.end(),
                    [&](const C_CSPlayerPawn& pawn) { return pawn.getAddress() == entityPointer; });
                if (!exists) {
                    Players::pawns.emplace_back(entityPointer);
                    std::cout << "Added pawn " << Players::pawns.size() << "\n";
                }
                continue;
            }

            if (strcmp(entityInfo->szName, "CCSPlayerController") == 0) {
                bool exists = std::any_of(Players::controllers.begin(), Players::controllers.end(),
                    [&](const CCSPlayerController& controller) { return controller.getAddress() == entityPointer; });
                if (!exists) {
                    Players::controllers.emplace_back(entityPointer);
                }
                continue;
            }
    }*/

    cached_players.clear();

    int nMaxHighestEntity = I::GameEntity->Instance->GetHighestEntityIndex();

    for (int i = 1; i <= nMaxHighestEntity; i++)
    {
        auto Entity = I::GameEntity->Instance->Get(i);
        if (!Entity)
            continue;

        if (!Entity->handle().valid())
            continue;

        SchemaClassInfoData_t* _class = nullptr;
        Entity->dump_class_info(&_class);
        if (!_class)
            continue;

        const uint32_t hash = HASH(_class->szName);

        PlayerType_t type = none;

        if (hash == HASH("CCSPlayerController"))
        {

            type = none; int health = 0;
            Vector_t position; Vector_t viewOffset;
            const char* name = "none"; const char* weapon_name = "none";

            CCSPlayerController* Controller = reinterpret_cast<CCSPlayerController*>(Entity);
            if (!Controller)
                continue;

            if (!Controller->m_hPawn().valid())
                continue;

            //@handle caching local player 
            if (Controller->IsLocalPlayer()) {
                auto LocalPlayer = I::GameEntity->Instance->Get<C_CSPlayerPawn>(Controller->m_hPawn().index());
                if (!LocalPlayer) {
                    cached_local.reset();
                    continue;
                }

                cached_local.alive = LocalPlayer->m_iHealth() > 0;
                if (LocalPlayer->m_iHealth() > 0) {
                    cached_local.poisition = LocalPlayer->m_vOldOrigin();
                    cached_local.health = LocalPlayer->m_iHealth();
                    cached_local.handle = LocalPlayer->handle().index();
                    cached_local.team = LocalPlayer->m_iTeamNum();
                }
                else {
                    cached_local.reset();
                }
            }
            else // @handle only players
            {
                auto Player = I::GameEntity->Instance->Get<C_CSPlayerPawn>(Controller->m_hPawn().index());
                if (!Player)
                    continue;

                if (Player->m_iHealth() <= 0)
                    continue;

                health = Player->m_iHealth();
                name = Controller->getName();
                position = Player->m_vOldOrigin(); viewOffset = Player->m_vecViewOffset();
                /*
                C_CSWeaponBase* active_weapon = Player->GetActiveWeapon();
                if (active_weapon) {
                    auto* weapon_data = active_weapon->Data();
                    if (weapon_data) {
                        const char* m_name = weapon_data->m_szName();
                        if (const char* weapon_prefix = strstr(m_name, "weapon_")) {
                            weapon_name.assign(weapon_prefix + strlen("weapon_"));
                        }
                        else {
                            weapon_name.assign(m_name);
                        }
                        std::transform(weapon_name.begin(), weapon_name.end(), weapon_name.begin(), ::toupper);
                    }
                }
                */
                

                cached_players.emplace_back(Entity, Player, Player->handle(),
                    type, health, name,
                    weapon_name, position, viewOffset, Player->m_iTeamNum());
            }
        }
    }
}

void Visuals::esp() {
    if (!Config::esp && !Config::showHealth && !Config::espFill && !Config::showNameTags)
        return;

    C_CSPlayerPawn* localPawn = H::oGetLocalPlayer(0);
    if (!localPawn || cached_players.empty())
        return;

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (const auto& Player : cached_players) {
        if (!Player.handle.valid() || Player.health <= 0 || Player.handle.index() == INVALID_EHANDLE_INDEX)
            continue;

        if (Config::teamCheck && (Player.team_num == cached_local.team))
            continue;

        Vector_t feetPos = Player.position;
        Vector_t headPos = Player.position + Player.viewOffset;

        Vector_t feetScreen, headScreen;
        if (!viewMatrix.WorldToScreen(feetPos, feetScreen) ||
            !viewMatrix.WorldToScreen(headPos, headScreen))
            continue;

        // --- calculate box dimensions ---
        float boxHeight = (feetScreen.y - headScreen.y) * 1.3f;
        float boxWidth = boxHeight / 2.0f;
        float boxX = ((feetScreen.x + headScreen.x) / 2.0f) - (boxWidth / 2.0f);
        float boxY = headScreen.y - (boxHeight - (feetScreen.y - headScreen.y)) / 2.0f;

        // --- draw box (outline + main) ---
        if (Config::esp) {
            if (Config::espOutline) {
                drawList->AddRect(ImVec2(boxX - 1, boxY - 1), ImVec2(boxX + boxWidth + 1, boxY + boxHeight + 1), IM_COL32(0, 0, 0, 255));
                drawList->AddRect(ImVec2(boxX + 1, boxY + 1), ImVec2(boxX + boxWidth - 1, boxY + boxHeight - 1), IM_COL32(0, 0, 0, 255));
            }
            drawList->AddRect(ImVec2(boxX, boxY), ImVec2(boxX + boxWidth, boxY + boxHeight), ImGui::ColorConvertFloat4ToU32(Config::espColor));
        }

        // --- HP bar ---
        if (Config::showHealth) {
            int hp = Player.health;
            float& anim_health = animated_health[Player.handle.index()];
            anim_health = anim_health + (hp - anim_health) * 0.1f;

            // Узкая вертикальная полоска слева от бокса
            float barWidth = 3.0f;
            float barX = boxX - barWidth - 2.0f; // вплотную слева
            float barY = boxY;
            float barHeight = boxHeight;

            int iClampedHp = hp < 100 ? hp : 100;
            float filledHeight = barHeight * (anim_health / 100.f);

            // Цвет: от красного к зелёному
            float green = iClampedHp * 2.55f;
            float red = 255.f - green;
            ImU32 hpColor = IM_COL32((int)red, (int)green, 0, 255);

            // Чёрный фон
            drawList->AddRectFilled(ImVec2(barX, barY), ImVec2(barX + barWidth, barY + barHeight), IM_COL32(0, 0, 0, 255));
            // Цветная заливка
            if (iClampedHp > 0) {
                drawList->AddRectFilled(
                    ImVec2(barX + 1, barY + barHeight - filledHeight),
                    ImVec2(barX + barWidth - 1, barY + barHeight),
                    hpColor
                );
            }
            // Чёрная рамка
            drawList->AddRect(ImVec2(barX, barY), ImVec2(barX + barWidth, barY + barHeight), IM_COL32(0, 0, 0, 255));
        }

        // --- name tag ---
        if (Config::showNameTags) {
            std::string playerName = Player.name ? Player.name : "";
            if (!playerName.empty()) {
                if (playerName.size() > 15)
                    playerName = playerName.substr(0, 15) + "...";
                ImVec2 nameSize = ImGui::CalcTextSize(playerName.c_str());
                float nameX = boxX + (boxWidth - nameSize.x) / 2;
                float nameY = boxY - nameSize.y - 4;
                drawList->AddText(ImVec2(nameX + 1, nameY + 1), IM_COL32(0, 0, 0, 255), playerName.c_str()); // shadow
                drawList->AddText(ImVec2(nameX, nameY), IM_COL32(255, 255, 255, 255), playerName.c_str());
            }
        }
    }
}
