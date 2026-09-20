#include "main.h"
#include "directx_blur.h"
#include "app/settings.hpp"
#include "app/config.hpp"
#include "features/esp.hpp"
#include "features/hotkeys.hpp"
#include "features/spec.hpp"
#include "features/hitlog.hpp"
#include "features/radar.hpp"
#include "features/weapon_icons_data.hpp"
#include "features/combat.hpp"
#include "sdk/game.hpp"
#include "sdk/offsets.hpp"
#include "sdk/vis.hpp"
#include "sdk/offset_update.hpp"
#include "imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <d3d11.h>
#include <dwmapi.h>
#include <tchar.h>
#include <windowsx.h>
#include <D3DX11tex.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "freetype.lib")
#pragma comment(lib, "dwmapi.lib")

static Game g_game;
static VisCheck g_vis;
static float g_menu_alpha = 0.f;
static float g_menu_anim = 0.f;
static HWND g_found_game = nullptr;

static float smootherstep(float t)
{
    t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
    return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
}

static void scale_draw_list(ImDrawList* dl, ImVec2 pivot, float scale)
{
    if (!dl || std::fabs(scale - 1.f) < 0.0008f)
        return;
    for (int i = 0; i < dl->VtxBuffer.Size; ++i) {
        ImDrawVert& v = dl->VtxBuffer[i];
        v.pos.x = pivot.x + (v.pos.x - pivot.x) * scale;
        v.pos.y = pivot.y + (v.pos.y - pivot.y) * scale;
    }
    for (int i = 0; i < dl->CmdBuffer.Size; ++i) {
        ImVec4& cr = dl->CmdBuffer[i].ClipRect;
        cr.x = pivot.x + (cr.x - pivot.x) * scale;
        cr.y = pivot.y + (cr.y - pivot.y) * scale;
        cr.z = pivot.x + (cr.z - pivot.x) * scale;
        cr.w = pivot.y + (cr.w - pivot.y) * scale;
    }
}

static void scale_menu_windows(const char* root, ImVec2 pivot, float scale)
{
    if (!root || std::fabs(scale - 1.f) < 0.0008f)
        return;
    ImGuiContext& g = *GImGui;
    for (ImGuiWindow* w : g.Windows) {
        if (!w || w->Hidden)
            continue;
        ImGuiWindow* r = w->RootWindow ? w->RootWindow : w;
        if (!r || !r->Name || std::strcmp(r->Name, root) != 0)
            continue;
        scale_draw_list(w->DrawList, pivot, scale);
    }
}

static bool key_edge(int vk)
{
    static bool down[256]{};
    const int i = vk & 255;
    const bool now = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool edge = now && !down[i];
    down[i] = now;
    return edge;
}

static const char* kBones[] = { "Head", "Neck", "Chest", "Upper chest", "Head > chest", "Head > neck > chest" };
static const char* kProfiles[] = { "Rifle", "Pistol", "Sniper" };
static const char* kTriggerHitbox[] = { "Head", "Chest", "Body" };
static const char* kBoxStyle[] = { "Corner Box", "3D Box", "Filled Box" };
static const char* kHealthPosition[] = { "Left", "Right" };
static const char* kHeadStyle[] = { "Circle", "Dot", "Box" };
static const char* kSkeletonMode[] = { "Head only", "Upper body", "Full skeleton" };
static const char* kSkeletonStyle[] = { "Lines", "Points", "Lines + points" };
static const char* kAnchors[] = { "Top right", "Top left", "Bottom left", "Bottom right" };
static const char* kRadarFilter[] = { "Everyone", "Enemies only", "Teammates only" };

static BOOL CALLBACK find_cs2_cb(HWND hwnd, LPARAM)
{
    if (!IsWindowVisible(hwnd))
        return TRUE;
    wchar_t title[256]{};
    GetWindowTextW(hwnd, title, 256);
    if (wcsstr(title, L"Counter-Strike 2") == nullptr)
        return TRUE;
    g_found_game = hwnd;
    return FALSE;
}

static HWND find_cs2_window()
{
    g_found_game = nullptr;
    EnumWindows(find_cs2_cb, 0);
    return g_found_game;
}

static void set_passthrough(HWND hwnd, bool pass)
{
    static int last = -1;
    const int want = pass ? 1 : 0;
    if (want == last)
        return;
    last = want;

    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (pass)
        ex |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
    else
        ex &= ~(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | (pass ? SWP_NOACTIVATE : 0));
}

static void sync_overlay(HWND overlay, HWND game, int fallback_w, int fallback_h)
{
    if (game && IsWindow(game) && !IsIconic(game)) {
        RECT cr{};
        GetClientRect(game, &cr);
        POINT tl{ 0, 0 };
        ClientToScreen(game, &tl);
        const int w = cr.right - cr.left;
        const int h = cr.bottom - cr.top;
        if (w >= 64 && h >= 64) {
            SetWindowPos(overlay, HWND_TOPMOST, tl.x, tl.y, w, h,
                         g_menu_open ? 0 : SWP_NOACTIVATE);
            return;
        }
    }
    SetWindowPos(overlay, HWND_TOPMOST, 0, 0, fallback_w, fallback_h,
                 g_menu_open ? 0 : SWP_NOACTIVATE);
}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    SetProcessDPIAware();

    const int screen_w = GetSystemMetrics(SM_CXSCREEN);
    const int screen_h = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"CS2Overlay", nullptr };
    ::RegisterClassExW(&wc);

    const DWORD ex = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT;
    HWND hwnd = ::CreateWindowExW(ex, wc.lpszClassName, L"CS2", WS_POPUP, 0, 0, screen_w, screen_h, nullptr, nullptr, wc.hInstance, nullptr);

    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

    ImFontConfig cfg;
    cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_NoHinting | ImGuiFreeTypeBuilderFlags_LightHinting | ImGuiFreeTypeBuilderFlags_LoadColor;

    static ImWchar icomoon_ranges[] = { 0x1, 0x10FFFD, 0 };
    static ImFontConfig icomoon_config;
    icomoon_config.OversampleH = icomoon_config.OversampleV = 1;
    icomoon_config.MergeMode = true;
    icomoon_config.GlyphOffset.y = 2;

    io.Fonts->AddFontFromMemoryTTF(PoppinsRegular, sizeof(PoppinsRegular), 20.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(icomoon_compressed_data_base85, 18.f, &icomoon_config, icomoon_ranges);

    font::esp_font = io.Fonts->AddFontFromMemoryTTF(PoppinsRegular, sizeof(PoppinsRegular), 16.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    font::regular_m = io.Fonts->AddFontFromMemoryTTF(PoppinsMedium, sizeof(PoppinsMedium), 21.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    font::regular_l = io.Fonts->AddFontFromMemoryTTF(PoppinsMedium, sizeof(PoppinsMedium), 41.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    font::s_inter_semibold = io.Fonts->AddFontFromMemoryTTF(PoppinsSemiBold, sizeof(PoppinsSemiBold), 17.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    font::bold_font = io.Fonts->AddFontFromMemoryTTF(PoppinsBold, sizeof(PoppinsBold), 23.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(icomoon_compressed_data_base85, 32.f, &icomoon_config, icomoon_ranges);
    font::inter_medium = io.Fonts->AddFontFromMemoryTTF(PoppinsMedium, sizeof(PoppinsMedium), 17.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());

    static const ImWchar weapon_icon_ranges[] = {
        0xE000, 0xE0FF,
        0xE100, 0xE1FF,
        0xE200, 0xE2FF,
        0
    };
    ImFontConfig weapon_icon_config;
    weapon_icon_config.FontDataOwnedByAtlas = false;
    weapon_icon_config.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
    weapon_icon_config.OversampleH = 1;
    weapon_icon_config.OversampleV = 1;
    font::weapon_icons = io.Fonts->AddFontFromMemoryTTF(
        (void*)obs_icons_bytes,
        sizeof(obs_icons_bytes),
        24.f,
        &weapon_icon_config,
        weapon_icon_ranges
    );

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImGuiStyle& s = ImGui::GetStyle();
    s.FramePadding = ImVec2(16, 9);
    s.ItemSpacing = ImVec2(6, 12);
    s.FrameRounding = 6.f;
    s.WindowRounding = 18.f;
    s.WindowBorderSize = 0.f;
    s.PopupBorderSize = 0.f;
    s.WindowPadding = ImVec2(22, 22);
    s.ChildBorderSize = 0.f;
    s.Colors[ImGuiCol_Border] = ImVec4(1.f, 1.f, 1.f, 0.06f);
    s.Colors[ImGuiCol_Separator] = ImVec4(1.f, 1.f, 1.f, 0.08f);
    s.Colors[ImGuiCol_BorderShadow] = ImVec4(0.f, 0.f, 0.f, 0.f);
    s.WindowShadowSize = 28.f;
    s.PopupRounding = 8.f;
    s.ScrollbarSize = 2;
    s.SeparatorTextPadding = ImVec2(10, 10);

    std::vector<s_tab> tabs_info;
    tabs_info.push_back({ "Visuals",  { "Players" } });
    tabs_info.push_back({ "Combat",   { "Aim", "Trigger" } });
    tabs_info.push_back({ "Settings", { "Menu", "Configs" } });
    tabs_info.push_back({ "Misc",     { "Team", "Hitlog" } });
    tabs_info.push_back({ "Radar",    { "Radar" } });

    c_tabs p_tabs(tabs_info);
    CNotifications p_notif;
    g_vis.set_search_dir("D:\\CS2\\maps");
    OffsetUpdate::instance().start();
    ConfigStore::instance().startup();
    radar_startup();
    static char g_cfg_name[64] = "config";
    static std::string g_cfg_selected;

    bool done = false;
    DWORD last_window_sync = 0;
    HWND game_hwnd = nullptr;

    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        const DWORD now = GetTickCount();
        if (now - last_window_sync > 250) {
            last_window_sync = now;
            game_hwnd = find_cs2_window();
            sync_overlay(hwnd, game_hwnd, screen_w, screen_h);
        }

        g_game.tick((g_menu.vis_enable && g_menu.vis_skeleton) || g_menu.aim_enable || g_menu.trigger_enable);
        OffsetUpdate::instance().tick();
        if (g_game.attached())
            g_vis.tick(g_game.map_name());

        if (key_edge(VK_INSERT) || key_edge(VK_F7))
            g_menu_open = !g_menu_open;
        if (key_edge(VK_ESCAPE) && g_menu_open)
            g_menu_open = false;
        if (key_edge(VK_F8) || g_want_quit)
            done = true;

        set_passthrough(hwnd, !g_menu_open);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        NewFrame();

        LoadImages();

        RECT overlay_rc{};
        GetClientRect(hwnd, &overlay_rc);
        const float ow = static_cast<float>(overlay_rc.right - overlay_rc.left);
        const float oh = static_cast<float>(overlay_rc.bottom - overlay_rc.top);

        draw_players(GetBackgroundDrawList(), g_game, g_vis, ow, oh, ImGui::GetTime());
        combat_draw(GetBackgroundDrawList(), g_game, ow, oh);
        if (g_menu.misc_watermark)
            draw_watermark(GetBackgroundDrawList(), g_game, g_vis, ow);
        draw_hotkeys(GetBackgroundDrawList());
        draw_spectators(GetBackgroundDrawList(), g_game, ow, oh);
        hitlog_draw(GetBackgroundDrawList(), g_game, ow, oh);
        radar_draw(GetBackgroundDrawList(), g_game, ow, oh);

        combat_tick(g_game, g_vis, ImGui::GetIO().DeltaTime, g_menu_open);
        hitlog_tick(g_game);
        radar_housekeep(g_menu_open);

        c::anim::speed = ImGui::GetIO().DeltaTime * 12.f;
        c::second_color = utils::GetDarkColor(c::main_color);

        const float dt = (std::min)(ImGui::GetIO().DeltaTime, 0.05f);
        const float open_spd = 1.f / 0.36f;
        const float close_spd = 1.f / 0.24f;
        if (g_menu_open)
            g_menu_anim = (std::min)(1.f, g_menu_anim + dt * open_spd);
        else
            g_menu_anim = (std::max)(0.f, g_menu_anim - dt * close_spd);

        const float ease = smootherstep(g_menu_anim);
        g_menu_alpha = ease;
        s.Alpha = ease;
        s.WindowShadowSize = 8.f + 20.f * ease;

        if (g_menu_anim > 0.001f) {
            static bool placed = false;
            if (!placed) {
                g_menu_w = std::clamp(g_menu_w, 720.f, ow);
                g_menu_h = std::clamp(g_menu_h, 520.f, oh);
                g_menu_x = (ow - g_menu_w) * 0.5f;
                g_menu_y = (oh - g_menu_h) * 0.5f;
                placed = true;
            }
            g_menu_w = std::clamp(g_menu_w, 720.f, ow);
            g_menu_h = std::clamp(g_menu_h, 520.f, oh);
            g_menu_x = std::clamp(g_menu_x, 0.f, (std::max)(0.f, ow - 160.f));
            g_menu_y = std::clamp(g_menu_y, 0.f, (std::max)(0.f, oh - 90.f));
            c::bg::size = ImVec2(g_menu_w, g_menu_h);

            const float scale = 0.965f + 0.035f * ease;
            const float y_off = (1.f - ease) * 16.f;
            const ImVec2 pos(g_menu_x, g_menu_y + y_off);
            const ImVec2 sz(g_menu_w, g_menu_h);
            const ImVec2 pivot(pos.x + sz.x * 0.5f, pos.y + sz.y * 0.5f);

            ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
            ImGui::SetNextWindowSize(sz, ImGuiCond_Always);
            Begin("CS2", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
            {
                ImDrawList* chrome = ImGui::GetWindowDrawList();

                draw_background_blur(chrome, g_pSwapChain, g_pd3dDevice, g_pd3dDeviceContext, pos, pos + sz, c::bg::rounding);
                chrome->AddRectFilled(pos, pos + sz, utils::GetColorWithAlpha(c::window_bg_color, c::window_bg_color.Value.w * s.Alpha), c::bg::rounding);
                chrome->AddRect(pos, pos + sz, IM_COL32(230, 236, 242, (int)(42 * s.Alpha)), c::bg::rounding, 0, 1.f);

                chrome->AddText(pos + ImVec2(18, sz.y - 32), c::label::default, "INSERT / F7  hide   ·   drag header   ·   resize corner   ·   F8  unload");

                chrome->AddRectFilled(pos, pos + ImVec2(sz.x, 68), GetColorU32(c::child::background), c::bg::rounding, ImDrawFlags_RoundCornersTop);

                PushFont(font::bold_font);
                chrome->AddText(utils::center_text(pos, pos + ImVec2(70, 68), ICON_FIRE_FILL) + ImVec2(0, 4.5f), main_color, ICON_FIRE_FILL);
                chrome->AddText(ImVec2(pos.x + 60, utils::center_text(pos, pos + ImVec2(70, 68), "CS2").y), c::label::active, "CS2");
                PopFont();

                ImGui::SetCursorScreenPos(pos);
                ImGui::InvisibleButton("##drag_menu", ImVec2(sz.x - 70.f, 68.f));
                if (ImGui::IsItemHovered())
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    g_menu_x += ImGui::GetIO().MouseDelta.x;
                    g_menu_y += ImGui::GetIO().MouseDelta.y;
                }

                ImGui::SetCursorScreenPos(pos + ImVec2(sz.x - 50.f, 18.f));
                if (ImGui::InvisibleButton("##close_menu", ImVec2(34.f, 34.f)))
                    g_menu_open = false;
                {
                    const bool hov = ImGui::IsItemHovered();
                    const ImVec2 x0 = pos + ImVec2(sz.x - 42.f, 22.f);
                    chrome->AddCircleFilled(x0 + ImVec2(9, 9), 13.f, IM_COL32(255, 255, 255, hov ? 28 : 12), 24);
                    chrome->AddText(x0 + ImVec2(3, -1), IM_COL32(230, 235, 240, hov ? 230 : 170), "x");
                }

                {
                    const ImVec2 grip = pos + sz - ImVec2(22.f, 22.f);
                    ImGui::SetCursorScreenPos(grip);
                    ImGui::InvisibleButton("##resize_menu", ImVec2(18.f, 18.f));
                    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
                    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                        g_menu_w += ImGui::GetIO().MouseDelta.x;
                        g_menu_h += ImGui::GetIO().MouseDelta.y;
                    }
                    const ImU32 gc = IM_COL32(220, 230, 240, ImGui::IsItemHovered() || ImGui::IsItemActive() ? 180 : 90);
                    chrome->AddLine(grip + ImVec2(4, 16), grip + ImVec2(16, 4), gc, 1.4f);
                    chrome->AddLine(grip + ImVec2(8, 16), grip + ImVec2(16, 8), gc, 1.4f);
                    chrome->AddLine(grip + ImVec2(12, 16), grip + ImVec2(16, 12), gc, 1.4f);
                }

                p_tabs.DrawTabs();

                const float half_w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
                const float full_h = sz.y - 128.f;

                if (p_tabs.IsTabActive(0))
                {
                    ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                    custom::Child("Box##L", ImVec2(half_w, full_h), true);
                    custom::Checkbox("Enable", &g_menu.vis_enable);
                    custom::Combo("Style", &g_menu.vis_box_style, kBoxStyle, IM_ARRAYSIZE(kBoxStyle));
                    custom::Checkbox("Health bar", &g_menu.vis_health);
                    custom::Combo("Health position", &g_menu.vis_health_position, kHealthPosition, IM_ARRAYSIZE(kHealthPosition));
                    custom::SliderInt("Health width", &g_menu.vis_health_width, 2, 8);
                    custom::Checkbox("Health gradient", &g_menu.vis_health_gradient);
                    custom::Checkbox("Health value", &g_menu.vis_health_value);
                    custom::Checkbox("Head marker", &g_menu.vis_head);
                    custom::Combo("Head shape", &g_menu.vis_head_style, kHeadStyle, IM_ARRAYSIZE(kHeadStyle));
                    custom::SliderInt("Head size", &g_menu.vis_head_size, 5, 20);
                    custom::Checkbox("Skeleton", &g_menu.vis_skeleton);
                    custom::Combo("Skeleton body", &g_menu.vis_skeleton_mode, kSkeletonMode, IM_ARRAYSIZE(kSkeletonMode));
                    custom::Combo("Skeleton style", &g_menu.vis_skeleton_style, kSkeletonStyle, IM_ARRAYSIZE(kSkeletonStyle));
                    custom::SliderInt("Skeleton thickness", &g_menu.vis_skeleton_thickness, 8, 30);
                    custom::Checkbox("Distance", &g_menu.vis_distance);
                    custom::Checkbox("Weapon icon", &g_menu.vis_weapon_icon);
                    custom::SliderInt("Icon size", &g_menu.vis_weapon_icon_size, 10, 32);
                    custom::Checkbox("Enemies only", &g_menu.vis_team_check);
                    custom::Checkbox("Visible only", &g_menu.vis_visible_only);
                    custom::SliderInt("Thickness", &g_menu.vis_thickness, 8, 28);
                    custom::SliderInt("Glow", &g_menu.vis_glow, 10, 90);
                    custom::SliderInt("Corner length", &g_menu.vis_corner, 16, 42);
                    custom::SliderInt("Max distance (m)", &g_menu.vis_max_distance, 20, 400);
                    custom::EndChild();

                    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);
                    custom::Child("Color##R", ImVec2(half_w, full_h), true);
                    custom::ColorEdit4("Enemy", g_menu.box_enemy, picker_flags);
                    custom::ColorEdit4("Team", g_menu.box_team, picker_flags);
                    custom::ColorEdit4("Health low", g_menu.health_low, picker_flags);
                    custom::ColorEdit4("Health high", g_menu.health_high, picker_flags);
                    custom::ColorEdit4("Head enemy", g_menu.head_enemy, picker_flags);
                    custom::ColorEdit4("Head team", g_menu.head_team, picker_flags);
                    custom::ColorEdit4("Skeleton visible", g_menu.skeleton_visible, picker_flags);
                    custom::ColorEdit4("Skeleton hidden", g_menu.skeleton_hidden, picker_flags);
                    custom::ColorEdit4("Weapon icon", g_menu.weapon_icon_color, picker_flags);
                    custom::EndChild();
                }

                if (p_tabs.IsTabActive(1))
                {
                    CombatProfile& prof = g_menu.aim_edit_profile == 1 ? g_menu.aim_pistol
                        : g_menu.aim_edit_profile == 2 ? g_menu.aim_sniper
                        : g_menu.aim_rifle;
                    ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                    custom::Child("Aim##L", ImVec2(half_w, full_h), true);
                    custom::Checkbox("Enable", &g_menu.aim_enable);
                    custom::Keybind("Aim key", &g_menu.aim_key, &g_menu.aim_key_mode);
                    custom::Checkbox("Visible only", &g_menu.aim_visible);
                    custom::Checkbox("Enemies only", &g_menu.aim_team_check);
                    custom::Checkbox("Draw FOV", &g_menu.aim_fov_draw);
                    custom::Combo("Weapon profile", &g_menu.aim_edit_profile, kProfiles, IM_ARRAYSIZE(kProfiles));
                    custom::SliderFloat("FOV (deg)", &prof.fov, 0.5f, 15.f, "%.1f");
                    custom::SliderFloat("Smooth", &prof.smooth, 0.f, 1.f, "%.2f");
                    custom::Combo("Bone", &prof.bone, kBones, IM_ARRAYSIZE(kBones));
                    ImGui::Dummy(ImVec2(0, 6));
                    ImGui::TextWrapped("Hold the key. Active in-game: %s",
                                       weapon_class_name(classify_weapon(g_game.weapon_def())));
                    custom::EndChild();

                    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);
                    custom::Child("Humanize##R", ImVec2(half_w, full_h), true);
                    custom::Checkbox("Humanize", &g_menu.aim_humanize);
                    custom::SliderFloat("Reaction min (ms)", &g_menu.aim_reaction_min, 0.f, 300.f, "%.0f");
                    custom::SliderFloat("Reaction max (ms)", &g_menu.aim_reaction_max, 0.f, 300.f, "%.0f");
                    custom::SliderFloat("Noise", &g_menu.aim_noise, 0.f, 1.f, "%.2f");
                    custom::SliderFloat("Overshoot", &g_menu.aim_overshoot, 0.f, 1.f, "%.2f");
                    custom::SliderFloat("Miss chance", &g_menu.aim_miss, 0.f, 0.20f, "%.2f");
                    custom::Checkbox("Recoil compensation", &g_menu.aim_recoil);
                    custom::SliderFloat("RCS yaw", &prof.rcs_yaw, 0.f, 2.5f, "%.2f");
                    custom::SliderFloat("RCS pitch", &prof.rcs_pitch, 0.f, 2.5f, "%.2f");
                    custom::Checkbox("Debug log", &g_menu.aim_debug);
                    ImGui::Dummy(ImVec2(0, 6));
                    ImGui::TextWrapped("Legit: FOV 1.5-5, smooth 0.5-0.8, humanize on. Hold ALT or a mouse button.");
                    custom::EndChild();
                }

                if (p_tabs.IsTabActive(2))
                {
                    ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                    custom::Child("Trigger##L", ImVec2(half_w, full_h), true);
                    custom::Checkbox("Enable", &g_menu.trigger_enable);
                    custom::Keybind("Trigger key", &g_menu.trigger_key, &g_menu.trigger_key_mode);
                    custom::Combo("Hitbox", &g_menu.trigger_hitbox, kTriggerHitbox, IM_ARRAYSIZE(kTriggerHitbox));
                    custom::SliderInt("First delay (ms)", &g_menu.trigger_first_ms, 0, 250);
                    custom::SliderInt("Next delay (ms)", &g_menu.trigger_next_ms, 0, 400);
                    custom::SliderInt("Jitter (ms)", &g_menu.trigger_jitter_ms, 0, 80);
                    custom::EndChild();

                    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);
                    custom::Child("Filters##R", ImVec2(half_w, full_h), true);
                    custom::Checkbox("Visible only", &g_menu.trigger_visible);
                    custom::Checkbox("Enemies only", &g_menu.trigger_team_check);
                    custom::Checkbox("Scope check (snipers)", &g_menu.trigger_scope);
                    custom::Checkbox("Anti-flash", &g_menu.trigger_flash);
                    custom::Checkbox("Ignore knife / nades", &g_menu.trigger_weapon_filter);
                    ImGui::Dummy(ImVec2(0, 8));
                    ImGui::TextWrapped("Fires only while the key is held and a hitbox is under the crosshair.");
                    custom::EndChild();
                }

                if (p_tabs.IsTabActive(3))
                {
                    ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                    custom::Child("Menu", ImVec2(ImGui::GetContentRegionAvail().x - 21, full_h), true);
                    custom::ColorEdit4("Accent", (float*)&c::main_color, picker_flags);
                    custom::Checkbox("Watermark", &g_menu.misc_watermark);
                    custom::Checkbox("Hotkeys panel", &g_menu.misc_hotkeys);
                    ImGui::Dummy(ImVec2(0, 8));
                    ImGui::TextWrapped("INSERT or F7 toggles this panel. ESC hides it. F8 unloads the overlay.");
                    ImGui::Dummy(ImVec2(0, 8));
                    ImGui::TextWrapped("Offsets: %s  (%s)  [%s]", offsets::kDumpUtc, offsets::kDumpUpdate, offsets::kDumpSource);
                    ImGui::Dummy(ImVec2(0, 4));
                    ImGui::TextWrapped("%s", OffsetUpdate::instance().status().c_str());
                    ImGui::Dummy(ImVec2(0, 8));
                    if (custom::Button("Refresh offsets", ImVec2(ImGui::GetContentRegionAvail().x, 44)))
                        OffsetUpdate::instance().request_poll();
                    ImGui::Dummy(ImVec2(0, 8));
                    if (custom::Button("Unload overlay", ImVec2(ImGui::GetContentRegionAvail().x, 44)))
                        g_want_quit = true;
                    custom::EndChild();
                }

                if (p_tabs.IsTabActive(4))
                {
                    // Zwei Spalten wie in Visuals/Combat: links Aktionen (fix, kompakt),
                    // rechts die gespeicherten Configs als direkt scrollende Liste.
                    // Das ersetzt den alten Single-Panel-Aufbau mit verschachteltem
                    // BeginChild: der innere ##cfg_list hat Wheel-Events geschluckt,
                    // sodass der aeussere Bereich nie scrollte und Save/Refresh/
                    // Default unten abgeschnitten und unklickbar waren.
                    auto& cfgs = ConfigStore::instance();
                    ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                    custom::Child("Actions##CfgL", ImVec2(half_w, full_h), true);
                    {
                        const float inner_w = ImGui::GetContentRegionAvail().x;
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.f, 1.f, 1.f, 0.045f));
                        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.f, 1.f, 1.f, 0.08f));
                        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(1.f, 1.f, 1.f, 0.10f));
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.94f, 0.96f, 1.f));
                        ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.62f, 0.66f, 0.70f, 0.85f));
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(16.f, 14.f));
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.f, 0.f));
                        ImGui::PushItemWidth(inner_w);
                        ImGui::InputTextEx("##n", "Type a name", g_cfg_name, IM_ARRAYSIZE(g_cfg_name),
                                           ImVec2(inner_w, 48.f), ImGuiInputTextFlags_None);
                        ImGui::PopItemWidth();
                        ImGui::PopStyleVar(3);
                        ImGui::PopStyleColor(5);

                        ImGui::Dummy(ImVec2(0, 2));
                        ImGui::TextWrapped("Presets");
                        ImGui::Dummy(ImVec2(0, 2));
                        // Vertikal gestapelt statt 3-spaltig: kein Abschneiden am
                        // rechten Rand mehr ("Semi Rage" war halb ausserhalb).
                        if (custom::Button("Legit", ImVec2(inner_w, 34))) {
                            if (cfgs.apply_preset(0))
                                p_notif.AddMessage(cfgs.status().c_str(), ICON_CHECK_FILL, c::main_color);
                            else
                                p_notif.AddMessage(cfgs.status().c_str(), ICON_ALERT_FILL, ImColor(210, 120, 120));
                        }
                        if (custom::Button("Legit with Aim", ImVec2(inner_w, 34))) {
                            if (cfgs.apply_preset(1))
                                p_notif.AddMessage(cfgs.status().c_str(), ICON_CHECK_FILL, c::main_color);
                            else
                                p_notif.AddMessage(cfgs.status().c_str(), ICON_ALERT_FILL, ImColor(210, 120, 120));
                        }
                        if (custom::Button("Semi Rage", ImVec2(inner_w, 34))) {
                            if (cfgs.apply_preset(2))
                                p_notif.AddMessage(cfgs.status().c_str(), ICON_CHECK_FILL, c::main_color);
                            else
                                p_notif.AddMessage(cfgs.status().c_str(), ICON_ALERT_FILL, ImColor(210, 120, 120));
                        }
                        ImGui::Dummy(ImVec2(0, 2));
                        if (custom::Button("Save", ImVec2(inner_w, 38))) {
                            const std::string want = sanitize_config_name(g_cfg_name);
                            if (want.empty()) {
                                p_notif.AddMessage("Enter a name (A-Z, 0-9)", ICON_ALERT_FILL, ImColor(210, 120, 120));
                            } else if (cfgs.save(want)) {
                                g_cfg_selected = want;
                                std::snprintf(g_cfg_name, sizeof(g_cfg_name), "%s", want.c_str());
                                p_notif.AddMessage(cfgs.status().c_str(), ICON_CHECK_FILL, c::main_color);
                            } else {
                                p_notif.AddMessage(cfgs.status().c_str(), ICON_ALERT_FILL, ImColor(210, 120, 120));
                            }
                        }
                        ImGui::Dummy(ImVec2(0, 2));
                        {
                            const float btn_w = (inner_w - 12.f) * 0.5f;
                            if (custom::Button("Save As...", ImVec2(btn_w, 34))) {
                                if (cfgs.save_as_dialog(hwnd, g_cfg_name)) {
                                    const std::string sel = cfgs.last_loaded();
                                    if (!sel.empty()) {
                                        g_cfg_selected = sel;
                                        std::snprintf(g_cfg_name, sizeof(g_cfg_name), "%s", sel.c_str());
                                    }
                                    p_notif.AddMessage(cfgs.status().c_str(), ICON_CHECK_FILL, c::main_color);
                                } else if (strstr(cfgs.status().c_str(), "failed")) {
                                    p_notif.AddMessage(cfgs.status().c_str(), ICON_ALERT_FILL, ImColor(210, 120, 120));
                                }
                            }
                            ImGui::SameLine(0, 12.f);
                            if (custom::Button("Open...", ImVec2(btn_w, 34))) {
                                if (cfgs.open_dialog(hwnd)) {
                                    const std::string sel = cfgs.last_loaded();
                                    if (!sel.empty()) {
                                        g_cfg_selected = sel;
                                        std::snprintf(g_cfg_name, sizeof(g_cfg_name), "%s", sel.c_str());
                                    }
                                    p_notif.AddMessage(cfgs.status().c_str(), ICON_CHECK_FILL, c::main_color);
                                } else if (strstr(cfgs.status().c_str(), "failed")) {
                                    p_notif.AddMessage(cfgs.status().c_str(), ICON_ALERT_FILL, ImColor(210, 120, 120));
                                }
                            }
                            ImGui::NewLine();
                        }
                        {
                            const float btn_w = (inner_w - 12.f) * 0.5f;
                            if (custom::Button("Refresh", ImVec2(btn_w, 34))) {
                                cfgs.refresh();
                                p_notif.AddMessage(cfgs.status().c_str(), ICON_REFRESH_1_FILL, c::main_color);
                            }
                            ImGui::SameLine(0, 12.f);
                            if (custom::Button("Default", ImVec2(btn_w, 34))) {
                                cfgs.reset_defaults();
                                p_notif.AddMessage(cfgs.status().c_str(), ICON_CHECK_FILL, c::main_color);
                            }
                            ImGui::NewLine();
                        }
                        const std::string preview = sanitize_config_name(g_cfg_name);
                        ImGui::Dummy(ImVec2(0, 2));
                        if (preview.empty())
                            ImGui::TextWrapped("Name: letters, numbers, dash or underscore.");
                        else
                            ImGui::TextWrapped("Will save as  %s.json", preview.c_str());
                        ImGui::TextWrapped("%s", cfgs.status().c_str());
                    }
                    custom::EndChild();

                    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);
                    custom::Child("Saved##CfgR", ImVec2(half_w, full_h), true);
                    {
                        char saved_title[64]{};
                        std::snprintf(saved_title, sizeof(saved_title), "Saved (%d)",
                                      static_cast<int>(cfgs.names().size()));
                        ImGui::TextWrapped("%s", saved_title);
                        ImGui::Dummy(ImVec2(0, 2));
                        if (cfgs.names().empty()) {
                            ImGui::TextWrapped("No saved configs yet.");
                        } else {
                            for (const std::string& name : cfgs.names()) {
                                ImGui::PushID(name.c_str());
                                const float row_w = ImGui::GetContentRegionAvail().x;
                                if (custom::Button("Load", ImVec2(76.f, 32.f))) {
                                    if (cfgs.load(name)) {
                                        g_cfg_selected = name;
                                        std::snprintf(g_cfg_name, sizeof(g_cfg_name), "%s", name.c_str());
                                        p_notif.AddMessage(cfgs.status().c_str(), ICON_CHECK_FILL, c::main_color);
                                    } else {
                                        p_notif.AddMessage(cfgs.status().c_str(), ICON_ALERT_FILL, ImColor(210, 120, 120));
                                    }
                                }
                                ImGui::SameLine(0, 8.f);
                                if (custom::Button("Delete", ImVec2(76.f, 32.f))) {
                                    if (cfgs.remove(name)) {
                                        if (g_cfg_selected == name)
                                            g_cfg_selected.clear();
                                        p_notif.AddMessage(cfgs.status().c_str(), ICON_DELETE_FILL, c::main_color);
                                    } else {
                                        p_notif.AddMessage(cfgs.status().c_str(), ICON_ALERT_FILL, ImColor(210, 120, 120));
                                    }
                                }
                                ImGui::SameLine(0, 12.f);
                                ImGui::AlignTextToFramePadding();
                                ImGui::TextUnformatted(name.c_str());
                                ImGui::Dummy(ImVec2(row_w, 2.f));
                                ImGui::PopID();
                            }
                        }
                    }
                    custom::EndChild();
                }

                if (p_tabs.IsTabActive(5))
                {
                    ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                    custom::Child("Team ESP##L", ImVec2(half_w, full_h), true);
                    custom::Checkbox("Team names", &g_menu.vis_team_names);
                    custom::Checkbox("Team distance", &g_menu.vis_team_distance);
                    ImGui::Dummy(ImVec2(0, 6));
                    ImGui::TextWrapped("Names draw over teammate boxes in team color, same layout as enemy names. Distance below the box is dimmed.");
                    custom::EndChild();

                    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);
                    custom::Child("Spectators##R", ImVec2(half_w, full_h), true);
                    custom::Checkbox("Enable", &g_menu.spec_enable);
                    custom::Combo("Position", &g_menu.spec_anchor, kAnchors, IM_ARRAYSIZE(kAnchors));
                    custom::SliderInt("Offset X", &g_menu.spec_off_x, -400, 400);
                    custom::SliderInt("Offset Y", &g_menu.spec_off_y, -400, 400);
                    ImGui::Dummy(ImVec2(0, 6));
                    ImGui::TextWrapped("Lists who spectates you (observer target). Empty shows a dimmed hint. Needs fresh observer offsets; otherwise the list stays empty.");
                    custom::EndChild();
                }

                if (p_tabs.IsTabActive(6))
                {
                    ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                    custom::Child("Damage log", ImVec2(ImGui::GetContentRegionAvail().x - 21, full_h), true);
                    custom::Checkbox("Enable", &g_menu.hitlog_enable);
                    custom::SliderInt("Max entries", &g_menu.hitlog_max, 1, 8);
                    custom::SliderFloat("Lifetime (s)", &g_menu.hitlog_time, 2.f, 8.f, "%.1f");
                    custom::Checkbox("Kill weapon icon", &g_menu.hitlog_kill_icon);
                    custom::Combo("Position", &g_menu.hitlog_anchor, kAnchors, IM_ARRAYSIZE(kAnchors));
                    custom::SliderInt("Offset X", &g_menu.hitlog_off_x, -400, 400);
                    custom::SliderInt("Offset Y", &g_menu.hitlog_off_y, -400, 400);
                    custom::ColorEdit4("Head", g_menu.hitlog_head, picker_flags);
                    custom::ColorEdit4("Chest", g_menu.hitlog_chest, picker_flags);
                    custom::ColorEdit4("Body", g_menu.hitlog_body, picker_flags);
                    ImGui::Dummy(ImVec2(0, 6));
                    ImGui::TextWrapped("Health-diff feed. Zones are a heuristic from the aim lock; kills show the current weapon glyph.");
                    custom::EndChild();
                }

                if (p_tabs.IsTabActive(7))
                {
                    ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                    custom::Child("Radar", ImVec2(ImGui::GetContentRegionAvail().x - 21, full_h), true);
                    if (custom::Checkbox("Enable", &g_radar.enable))
                        radar_mark_dirty();
                    if (custom::Combo("Filter", &g_radar.filter, kRadarFilter, IM_ARRAYSIZE(kRadarFilter)))
                        radar_mark_dirty();
                    if (custom::Checkbox("Heading-up (rotate)", &g_radar.rotate))
                        radar_mark_dirty();
                    if (custom::SliderInt("Size", &g_radar.size, 140, 320))
                        radar_mark_dirty();
                    if (custom::SliderFloat("Opacity", &g_radar.opacity, 0.30f, 1.f, "%.2f"))
                        radar_mark_dirty();
                    if (custom::SliderFloat("Range (m)", &g_radar.range, 10.f, 60.f, "%.0f"))
                        radar_mark_dirty();
                    if (custom::Combo("Position", &g_radar.anchor, kAnchors, IM_ARRAYSIZE(kAnchors)))
                        radar_mark_dirty();
                    if (custom::SliderInt("Offset X", &g_radar.off_x, -600, 600))
                        radar_mark_dirty();
                    if (custom::SliderInt("Offset Y", &g_radar.off_y, -600, 600))
                        radar_mark_dirty();
                    if (custom::ColorEdit4("Enemy", g_radar.enemy, picker_flags))
                        radar_mark_dirty();
                    if (custom::ColorEdit4("Team", g_radar.team, picker_flags))
                        radar_mark_dirty();
                    if (custom::ColorEdit4("Local arrow", g_radar.local, picker_flags))
                        radar_mark_dirty();
                    if (custom::ColorEdit4("Rings", g_radar.ring, picker_flags))
                        radar_mark_dirty();
                    ImGui::Dummy(ImVec2(0, 6));
                    ImGui::TextWrapped("Dots outside the range clamp to the rim and dim. Saved to radar.json when the menu closes.");
                    custom::EndChild();
                }

                p_notif.Render();
            }
            End();

            scale_menu_windows("CS2", pivot, scale);

            g_menu_w = c::bg::size.x;
            g_menu_h = c::bg::size.y;
            g_hitn = 0;
            g_hits[g_hitn++] = { g_menu_x, g_menu_y, g_menu_w, g_menu_h };
            ImGuiContext& ig = *GImGui;
            for (ImGuiWindow* w : ig.Windows) {
                if (!w || w->Hidden || (!w->Active && !w->WasActive))
                    continue;
                if ((w->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) == 0)
                    continue;
                if (w->Size.x < 2.f || w->Size.y < 2.f)
                    continue;
                if (g_hitn >= 24)
                    break;
                g_hits[g_hitn++] = { w->Pos.x, w->Pos.y, w->Size.x, w->Size.y };
            }
        } else {
            g_hitn = 0;
        }

        Render();
        const float clear[4] = { 0.f, 0.f, 0.f, 0.f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(0, 0);
    }

    OffsetUpdate::instance().stop();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 0;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_MOUSEACTIVATE)
        return g_menu_open ? MA_ACTIVATE : MA_NOACTIVATE;

    if (msg == WM_NCHITTEST) {
        if (!g_menu_open)
            return HTTRANSPARENT;
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &pt);
        const float mx = (float)pt.x, my = (float)pt.y;
        for (int i = 0; i < g_hitn; ++i) {
            const HitRect& r = g_hits[i];
            if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h)
                return HTCLIENT;
        }
        return HTTRANSPARENT;
    }

    if (g_menu_open && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
