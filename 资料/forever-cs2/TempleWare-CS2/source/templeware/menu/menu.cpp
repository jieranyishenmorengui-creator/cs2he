#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.h"
#include "../config/config.h"
#include <iostream>
#include <vector>
#include "../config/configmanager.h"
#include "../keybinds/keybinds.h"
#include "../utils/logging/log.h"
#include "../../../external/imgui/imgui_internal.h"
#include "../../../external/imgui/imgui_impl_dx9.h"
#include "../../../external/imgui/imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>
#include "../../../external/imgui/imgui.h"
#include "../interfaces/interfaces.h"
#include "../../../external/imgui/gui.hpp"
#include "../../../external/imgui/blur.hpp"
#include "../hooks/hooks.h"
#include "hashes.hpp"
#include "bytes.hpp"

#ifdef FONT_AWESOME_BINARY_DEFINED
extern unsigned char font_awesome_binary[];
extern unsigned int font_awesome_binary_size;
#endif

static float menu_anim = 0.0f;
static int menu_tab = 0;
static int menu_subtab = 0;
static ImVec4 accent_color = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
static ImVec4 text_color = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
static ImVec4 border_color = ImVec4(40, 40, 40, 255);
static ImVec4 line_color = ImVec4(0.43f, 0.43f, 0.50f, 0.7f);
static ImVec4 button_color = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
static ImVec4 button_hovered = ImVec4(0.37f, 0.61f, 0.98f, 1.0f);
static ImVec4 button_active = ImVec4(0.25f, 0.51f, 0.85f, 1.0f);
static ImTextureID avatar = nullptr;
static ImTextureID bg = nullptr;

Menu* Menu::instance = nullptr;

Menu::Menu() {
    instance = this;
    activeTab = 0;
    showMenu = true;
    stack = false;
}

void Menu::init(HWND& window, ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11RenderTargetView* mainRenderTargetView) {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(pDevice, pContext);

    io.Fonts->AddFontFromMemoryTTF(museo500_binary, sizeof(museo500_binary), 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());

    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromMemoryTTF(font_awesome_binary, sizeof(font_awesome_binary), 15.0f, &icons_config, icon_ranges);

    io.Fonts->AddFontFromMemoryTTF(museo900_binary, sizeof(museo900_binary), 28.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());

    std::cout << "Menu initialized\n";
}

void Menu::render() {
    keybind.pollInputs();
    if (!showMenu) return;

    ImGuiIO& io = ImGui::GetIO();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(690, 500), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_Once);

    ImGui::Begin("FOREVER", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();

    menu_anim = ImLerp(menu_anim, 1.0f, 0.045f);

    if (bg) {
        ImGui::GetBackgroundDrawList()->AddImage(bg, ImVec2(0, 0), io.DisplaySize);
    }

    // Title
    ImFont* boldFont = io.Fonts->Fonts.size() > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];
    float fontSize = boldFont->FontSize;
    ImVec2 textPos = pos + ImVec2(170 / 2 - boldFont->CalcTextSizeA(fontSize, FLT_MAX, 0, "FOREVER").x / 2, 20);

    draw->AddText(boldFont, fontSize, textPos + ImVec2(1, 0),
        ImGui::ColorConvertFloat4ToU32(accent_color), "FOREVER");
    draw->AddText(boldFont, fontSize, textPos,
        ImGui::ColorConvertFloat4ToU32(text_color), "FOREVER");

    ImU32 sep_color = ImGui::GetColorU32(ImGuiCol_Separator);

    draw->AddLine(
        pos + ImVec2(0, size.y - 50),
        pos + ImVec2(170, size.y - 50),
        sep_color,
        1.0f
    );

    // Avatar circle
    ImVec2 center = pos + ImVec2(30, size.y - 25);
    draw->AddCircleFilled(center, 15.0f, ImColor(20, 20, 20, 255), 64);

    draw->AddText(pos + ImVec2(50, size.y - 40),
        ImGui::ColorConvertFloat4ToU32(text_color), "Forever");
    draw->AddText(pos + ImVec2(50, size.y - 25),
        ImGui::ColorConvertFloat4ToU32(ImVec4(0.7f, 0.7f, 0.7f, 1.0f)), "Till:");
    draw->AddText(pos + ImVec2(50 + ImGui::CalcTextSize("Till: ").x, size.y - 25),
        ImGui::ColorConvertFloat4ToU32(accent_color), "never");

    // Left menu (tabs)
    ImGui::SetCursorPos(ImVec2(10, 70));
    ImGui::BeginChild("##tabs", ImVec2(150, size.y - 120));

    gui.group_title("Aimbot");
    const char* tabNames[] = { "Legitbot", "Players", "World", "Misc", "Configs" };
    const char* tabIcons[] = { (const char*)ICON_FA_MOUSE, (const char*)ICON_FA_USERS, (const char*)ICON_FA_GLOBE, (const char*)ICON_FA_LAYER_GROUP, (const char*)ICON_FA_COG };

    for (int i = 0; i < 1; ++i) {
        if (gui.tab(tabIcons[i], tabNames[i], menu_tab == i)) {
            menu_tab = i;
            menu_anim = 0.0f;
        }
    }
    gui.group_title("Common");
    for (int i = 1; i < 4; ++i) {
        if (gui.tab(tabIcons[i], tabNames[i], menu_tab == i)) {
            menu_tab = i;
            menu_anim = 0.0f;
        }
    }
    gui.group_title("Presets");
    for (int i = 4; i < 5; ++i) {
        if (gui.tab(tabIcons[i], tabNames[i], menu_tab == i)) {
            menu_tab = i;
            menu_anim = 0.0f;
        }
    }
    ImGui::EndChild();

    // Main window (tab content)
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menu_anim);
    ImGui::SetCursorPos(ImVec2(185, 81 - (5 * menu_anim)));
    ImGui::BeginChild("##childs", ImVec2(size.x - 200, size.y - 96));

    switch (menu_tab) {
        case 0: {
            // Legitbot tab content
            break;
        }
        case 1: {
            float featherWidth = ImGui::GetWindowWidth() * 0.45f;
            float crownWidth = ImGui::GetWindowWidth() * 0.55f - ImGui::GetStyle().ItemSpacing.x;

            ImGui::BeginChild("Feather", ImVec2(featherWidth, 210));
            {
                ImVec2 savedPos = ImGui::GetCursorPos();
                ImGui::Checkbox("Box", &Config::esp);
                ImGui::SetCursorPos(ImVec2(savedPos.x + 160.0f, savedPos.y));
                ImGui::ColorEdit4("##BoxColor", (float*)&Config::espColor,
                    ImGuiColorEditFlags_NoInputs |
                    ImGuiColorEditFlags_NoTooltip |
                    ImGuiColorEditFlags_AlphaPreview);
            }
            ImGui::Separator();
            ImGui::Checkbox("Health Bar", &Config::showHealth);
            ImGui::Separator();
            ImGui::Checkbox("Name", &Config::showNameTags);
            ImGui::Separator();
            ImGui::Checkbox("Team Check", &Config::teamCheck);
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("Crown", ImVec2(crownWidth, ImGui::GetWindowHeight()));
            ImGui::Checkbox("Chams visible", &Config::enemyChams);
            if (Config::enemyChams) {
                ImGui::SameLine(200);
                ImGui::ColorEdit4("##ChamsVisible", (float*)&Config::colVisualChams,
                    ImGuiColorEditFlags_NoInputs |
                    ImGuiColorEditFlags_NoTooltip |
                    ImGuiColorEditFlags_AlphaPreview);
            }
            ImGui::Separator();

            ImGui::Checkbox("Chams Invisible", &Config::enemyChamsInvisible);
            if (Config::enemyChamsInvisible) {
                ImGui::SameLine(200);
                ImGui::ColorEdit4("##ChamsInvisible", (float*)&Config::colVisualChamsIgnoreZ,
                    ImGuiColorEditFlags_NoInputs |
                    ImGuiColorEditFlags_NoTooltip |
                    ImGuiColorEditFlags_AlphaPreview);
            }
            ImGui::Separator();

            const char* chamsMaterials[] = { "Flat", "Illuminate", "Glow", "Latex", "Ghost", "Fatal" };
            ImGui::Combo("Material", &Config::chamsMaterial, chamsMaterials, IM_ARRAYSIZE(chamsMaterials));
            ImGui::EndChild();
            break;
        }
        case 2: {
            float featherWidth = ImGui::GetWindowWidth() * 0.45f;
            float crownWidth = ImGui::GetWindowWidth() * 0.55f - ImGui::GetStyle().ItemSpacing.x;
            ImGui::BeginChild("Feather", ImVec2(featherWidth, 210));
            {
                ImVec2 savedPos = ImGui::GetCursorPos();
                ImGui::Checkbox("SkyBox color", &Config::skybox);
                if (Config::skybox) {
                    ImGui::SetCursorPos(ImVec2(savedPos.x + 160.0f, savedPos.y));
                    ImGui::ColorEdit4("##SkyBox Color", (float*)&Config::skyboxcolor,
                        ImGuiColorEditFlags_NoInputs |
                        ImGuiColorEditFlags_NoTooltip |
                        ImGuiColorEditFlags_AlphaPreview);
                }
            }
            ImGui::Separator();
            {
                ImVec2 savedPos = ImGui::GetCursorPos();
                ImGui::Checkbox("Light color", &Config::light);
                if (Config::light) {
                    ImGui::SetCursorPos(ImVec2(savedPos.x + 160.0f, savedPos.y));
                    ImGui::ColorEdit4("##DrawLight", (float*)&Config::DrawLight,
                        ImGuiColorEditFlags_NoInputs |
                        ImGuiColorEditFlags_NoTooltip |
                        ImGuiColorEditFlags_AlphaPreview);
                }
            }
            ImGui::Separator();
            ImGui::Checkbox("Fog rendering", &Config::fog);
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("Crown", ImVec2(crownWidth, ImGui::GetWindowHeight()));
            ImGui::Checkbox("Custom FOV", &Config::fovEnabled);
            if (Config::fovEnabled) {
                ImGui::Separator();
                ImGui::SliderFloat("FOV", &Config::fov, 20.0f, 140.0f, "%1.0f");
            }
            ImGui::Separator();
            ImGui::Checkbox("View model", &Config::viewmodell);
            if (Config::viewmodell) {
                ImGui::Separator();
                ImGui::SliderFloat("X", &Config::viewx, -20.f, 20.f, "%1.0f");
                ImGui::Separator();
                ImGui::SliderFloat("Y", &Config::viewy, -20.f, 20.f, "%1.0f");
                ImGui::Separator();
                ImGui::SliderFloat("Z", &Config::viewz, -20.f, 20.f, "%1.0f");
                ImGui::Separator();
                ImGui::SliderFloat("Fov view", &Config::ifov, 60.f, 150.f, "%1.0f");
            }
            ImGui::EndChild();
            break;
        }
        case 3: {
            float featherWidth = ImGui::GetWindowWidth() * 0.45f;
            float crownWidth = ImGui::GetWindowWidth() * 0.55f - ImGui::GetStyle().ItemSpacing.x;

            ImGui::BeginChild("Feather", ImVec2(featherWidth, 210));
            ImGui::Checkbox("Anti Flash", &Config::antiflash);
            ImGui::Separator();
            ImGui::Checkbox("Remove Smoke", &Config::SmokeRemove);
            ImGui::Separator();
            ImGui::Checkbox("Remove Scope", &Config::ScopeRemove);
            ImGui::Separator();
            ImGui::Checkbox("Remove Legs", &Config::removelegs);
            ImGui::Separator();
            ImGui::Checkbox("Enable Spammer", &Config::spammer);
            ImGui::Separator();
            const char* spamOptions[] = { "Forever" };
            ImGui::Combo("Spammer", &Config::spammingcho, spamOptions, IM_ARRAYSIZE(spamOptions));
            ImGui::EndChild();
            break;
        }
        case 4: {
            ImGui::BeginChild("ConfigLeft", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f - 5, 0), false);
            ImGui::Text("General");
            /*ImGui::Separator();*/

            static char configName[128] = "";
            static std::vector<std::string> configList = internal_config::ConfigManager::ListConfigs();
            static int selectedConfigIndex = -1;

            ImGui::InputText("Config Name", configName, IM_ARRAYSIZE(configName));

            if (ImGui::Button("Refresh")) {
                configList = internal_config::ConfigManager::ListConfigs();
            }
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                internal_config::ConfigManager::Load(configName);
            }
            ImGui::SameLine();
            if (ImGui::Button("Save")) {
                internal_config::ConfigManager::Save(configName);
                configList = internal_config::ConfigManager::ListConfigs();
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                internal_config::ConfigManager::Remove(configName);
                configList = internal_config::ConfigManager::ListConfigs();
            }

            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginChild("ConfigRight", ImVec2(0, 0), false);
            ImGui::Text("Saved Configs");
            /*ImGui::Separator();*/

            for (int i = 0; i < static_cast<int>(configList.size()); i++) {
                if (ImGui::Selectable(configList[i].c_str(), selectedConfigIndex == i)) {
                    selectedConfigIndex = i;
                    strncpy_s(configName, sizeof(configName), configList[i].c_str(), _TRUNCATE);
                }
            }

            ImGui::EndChild();
            break;
        }
        default:
            break;
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::End();
    ImGui::PopStyleVar();
}

void* __fastcall H::hkIsRelativeMouseMode(void* pThisptr, bool bActive) {
    auto res = IsRelativeMouseMode.GetOriginal();
    Menu::sdk = bActive;
    return res(pThisptr, Menu::IsMenuOpen() ? false : bActive);
}

bool __fastcall H::hkMouseInputEnabled(void* rcx) {
    auto res = MouseInputEnabled.GetOriginal();
    return Menu::ShouldBlockMouseInput() ? false : res(rcx);
}

void Menu::toggleMenu() {
    showMenu = !showMenu;
    auto repet = H::IsRelativeMouseMode.GetOriginal();
    repet(I::InputSys, Menu::IsMenuOpen() ? false : Menu::sdk);
}
