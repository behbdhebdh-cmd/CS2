#include "main.h"
#include "directx_blur.h"
#include "app/settings.hpp"
#include "features/esp.hpp"
#include "sdk/game.hpp"
#include "sdk/offsets.hpp"
#include "sdk/vis.hpp"
#include "imgui_internal.h"

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
static float g_menu_alpha = 1.f;
static HWND g_found_game = nullptr;

static bool key_edge(int vk)
{
    static bool down[256]{};
    const int i = vk & 255;
    const bool now = (GetAsyncKeyState(vk) & 0x8000) != 0;
    const bool edge = now && !down[i];
    down[i] = now;
    return edge;
}

static const char* kBones[] = { "Head", "Neck", "Chest", "Pelvis" };
static const char* kBoxStyle[] = { "Corner Box", "3D Box", "Filled Box" };

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
        ex |= WS_EX_TRANSPARENT;
    else
        ex &= ~WS_EX_TRANSPARENT;
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
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
            SetWindowPos(overlay, HWND_TOPMOST, tl.x, tl.y, w, h, SWP_NOACTIVATE);
            return;
        }
    }
    SetWindowPos(overlay, HWND_TOPMOST, 0, 0, fallback_w, fallback_h, SWP_NOACTIVATE);
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
    tabs_info.push_back({ "Settings", { "Menu" } });

    c_tabs p_tabs(tabs_info);
    CNotifications p_notif;
    g_vis.set_search_dir("D:\\CS2\\maps");

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

        g_game.tick();
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
        if (g_menu.misc_watermark)
            draw_watermark(GetBackgroundDrawList(), g_game, g_vis, ow);

        c::anim::speed = ImGui::GetIO().DeltaTime * 12.f;
        c::second_color = utils::GetDarkColor(c::main_color);
        g_menu_alpha = g_menu_open ? 1.f : 0.f;
        s.Alpha = g_menu_alpha;

        if (g_menu_open) {
            ImGui::SetNextWindowPos(ImVec2((ow - c::bg::size.x) * 0.5f, (oh - c::bg::size.y) * 0.5f), ImGuiCond_Once);
            ImGui::SetNextWindowSize(c::bg::size);
            Begin("CS2", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);
            {
                const ImVec2& pos = ImGui::GetWindowPos();

                draw_background_blur(GetBackgroundDrawList(), g_pSwapChain, g_pd3dDevice, g_pd3dDeviceContext, pos, pos + c::bg::size, c::bg::rounding);
                GetBackgroundDrawList()->AddRectFilled(pos, pos + c::bg::size, utils::GetColorWithAlpha(c::window_bg_color, c::window_bg_color.Value.w * s.Alpha), c::bg::rounding);
                GetBackgroundDrawList()->AddRect(pos, pos + c::bg::size, IM_COL32(230, 236, 242, (int)(42 * s.Alpha)), c::bg::rounding, 0, 1.f);

                GetBackgroundDrawList()->AddText(pos + ImVec2(18, c::bg::size.y - 32), c::label::default, "INSERT / F7  hide   ·   ESC  close   ·   F8  unload");

                GetBackgroundDrawList()->AddRectFilled(pos, pos + ImVec2(c::bg::size.x, 68), GetColorU32(c::child::background), c::bg::rounding, ImDrawFlags_RoundCornersTop);

                PushFont(font::bold_font);
                GetBackgroundDrawList()->AddText(utils::center_text(pos, pos + ImVec2(70, 68), ICON_FIRE_FILL) + ImVec2(0, 4.5f), main_color, ICON_FIRE_FILL);
                GetBackgroundDrawList()->AddText(ImVec2(pos.x + 60, utils::center_text(pos, pos + ImVec2(70, 68), "CS2").y), c::label::active, "CS2");
                PopFont();

                ImGui::SetCursorScreenPos(pos + ImVec2(c::bg::size.x - 50.f, 18.f));
                if (ImGui::InvisibleButton("##close_menu", ImVec2(34.f, 34.f)))
                    g_menu_open = false;
                {
                    const bool hov = ImGui::IsItemHovered();
                    const ImVec2 x0 = pos + ImVec2(c::bg::size.x - 42.f, 22.f);
                    GetBackgroundDrawList()->AddCircleFilled(x0 + ImVec2(9, 9), 13.f, IM_COL32(255, 255, 255, hov ? 28 : 12), 24);
                    GetBackgroundDrawList()->AddText(x0 + ImVec2(3, -1), IM_COL32(230, 235, 240, hov ? 230 : 170), "x");
                }

                g_menu_x = pos.x;
                g_menu_y = pos.y;
                g_menu_w = c::bg::size.x;
                g_menu_h = c::bg::size.y;

                p_tabs.DrawTabs();

                const float half_w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
                const float full_h = 400.f;

                if (p_tabs.IsTabActive(0))
                {
                    ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                    custom::Child("Box##L", ImVec2(half_w, full_h), true);
                    custom::Checkbox("Enable", &g_menu.vis_enable);
                    custom::Combo("Style", &g_menu.vis_box_style, kBoxStyle, IM_ARRAYSIZE(kBoxStyle));
                    custom::Checkbox("Health bar", &g_menu.vis_health);
                    custom::Checkbox("Enemies only", &g_menu.vis_team_check);
                    custom::Checkbox("Visible only", &g_menu.vis_visible_only);
                    custom::SliderInt("Thickness", &g_menu.vis_thickness, 8, 28);
                    custom::SliderInt("Glow", &g_menu.vis_glow, 10, 90);
                    custom::SliderInt("Corner length", &g_menu.vis_corner, 16, 42);
                    custom::SliderInt("Max distance", &g_menu.vis_max_distance, 20, 400);
                    custom::EndChild();

                    ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);
                    custom::Child("Color##R", ImVec2(half_w, full_h), true);
                    custom::ColorEdit4("Enemy", g_menu.box_enemy, picker_flags);
                    custom::ColorEdit4("Team", g_menu.box_team, picker_flags);
                    custom::EndChild();
                }

                if (p_tabs.IsTabActive(1))
                {
                    ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                    custom::Child("Aim", ImVec2(ImGui::GetContentRegionAvail().x - 21, full_h), true);
                    custom::Checkbox("Enable", &g_menu.aim_enable);
                    custom::Checkbox("Visible only", &g_menu.aim_visible);
                    custom::Checkbox("Recoil compensation", &g_menu.aim_recoil);
                    custom::SliderInt("FOV", &g_menu.aim_fov, 1, 30);
                    custom::SliderInt("Smooth", &g_menu.aim_smooth, 1, 40);
                    custom::Combo("Bone", &g_menu.aim_bone, kBones, IM_ARRAYSIZE(kBones));
                    custom::Keybind("Aim key", &g_menu.aim_key, &g_menu.aim_key_mode);
                    ImGui::Dummy(ImVec2(0, 8));
                    ImGui::TextWrapped("Not wired yet — visuals first.");
                    custom::EndChild();
                }

                if (p_tabs.IsTabActive(2))
                {
                    ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                    custom::Child("Trigger", ImVec2(ImGui::GetContentRegionAvail().x - 21, full_h), true);
                    custom::Checkbox("Enable", &g_menu.trigger_enable);
                    custom::SliderInt("Delay (ms)", &g_menu.trigger_delay_ms, 0, 200);
                    custom::Keybind("Trigger key", &g_menu.trigger_key, &g_menu.trigger_key_mode);
                    ImGui::Dummy(ImVec2(0, 8));
                    ImGui::TextWrapped("Not wired yet — visuals first.");
                    custom::EndChild();
                }

                if (p_tabs.IsTabActive(3))
                {
                    ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                    custom::Child("Menu", ImVec2(ImGui::GetContentRegionAvail().x - 21, full_h), true);
                    custom::ColorEdit4("Accent", (float*)&c::main_color, picker_flags);
                    custom::Checkbox("Watermark", &g_menu.misc_watermark);
                    ImGui::Dummy(ImVec2(0, 8));
                    ImGui::TextWrapped("INSERT or F7 toggles this panel. ESC hides it. F8 unloads the overlay.");
                    ImGui::Dummy(ImVec2(0, 8));
                    ImGui::TextWrapped("Offsets: %s  (%s)", offsets::kDumpUtc, offsets::kDumpUpdate);
                    ImGui::Dummy(ImVec2(0, 12));
                    if (custom::Button("Unload overlay", ImVec2(ImGui::GetContentRegionAvail().x, 44)))
                        g_want_quit = true;
                    custom::EndChild();
                }

                p_notif.Render();
            }
            End();

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
        return MA_NOACTIVATE;

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
