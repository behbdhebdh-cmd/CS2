#include "main.h"
#include "directx_blur.h"
#include "sdk/offsets.hpp"

#include <d3d11.h>
#include <tchar.h>
#include <D3DX11tex.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "freetype.lib")

// Menu state is UI-only. None of these flags are connected to the game.
struct MenuState {
    bool aim_enable = false;
    bool aim_visible = true;
    bool aim_recoil = false;
    bool trigger_enable = false;
    bool vis_box = false;
    bool vis_health = false;
    bool vis_skeleton = false;
    bool vis_name = false;
    bool vis_weapon = false;
    bool vis_team_check = true;
    bool radar_enable = false;
    bool misc_watermark = true;
    bool misc_crosshair = false;

    int aim_fov = 8;
    int aim_smooth = 12;
    int aim_bone = 0;
    int team_filter = 0;
    int trigger_delay_ms = 20;
    int radar_size = 180;
    int radar_range = 2500;
    int box_thickness = 1;

    int aim_key = 0;
    int aim_key_mode = 1;
    int trigger_key = 0;
    int trigger_key_mode = 1;
    int menu_key = 0;
    int menu_key_mode = 0;

    float fov_color[4] = { 173 / 255.f, 143 / 255.f, 233 / 255.f, 0.55f };
    float box_visible[4] = { 118 / 255.f, 187 / 255.f, 117 / 255.f, 0.9f };
    float box_hidden[4] = { 220 / 255.f, 80 / 255.f, 80 / 255.f, 0.9f };
};

static MenuState g_menu;
static float g_menu_alpha = 1.f;
static bool g_menu_open = true;

static const char* kBones[] = { "Head", "Neck", "Chest", "Pelvis" };
static const char* kTeamFilter[] = { "Enemies", "All", "Team" };

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    const int screen_w = GetSystemMetrics(SM_CXSCREEN);
    const int screen_h = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"CS2Setup", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"CS2 Overlay Setup", WS_POPUP, 0, 0, screen_w, screen_h, nullptr, nullptr, wc.hInstance, nullptr);

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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImFontConfig cfg;
    cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_NoHinting | ImGuiFreeTypeBuilderFlags_LightHinting | ImGuiFreeTypeBuilderFlags_LoadColor;

    static ImWchar icomoon_ranges[] = { 0x1, 0x10FFFD, 0 };
    static ImFontConfig icomoon_config;
    icomoon_config.OversampleH = icomoon_config.OversampleV = 1;
    icomoon_config.MergeMode = true;
    icomoon_config.GlyphOffset.y = 2;

    io.Fonts->AddFontFromMemoryTTF(PoppinsRegular, sizeof(PoppinsRegular), 20.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(icomoon_compressed_data_base85, 18.f, &icomoon_config, icomoon_ranges);

    font::esp_font = io.Fonts->AddFontFromMemoryTTF(PoppinsRegular, sizeof(PoppinsRegular), 17.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    font::regular_m = io.Fonts->AddFontFromMemoryTTF(PoppinsMedium, sizeof(PoppinsMedium), 21.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    font::regular_l = io.Fonts->AddFontFromMemoryTTF(PoppinsMedium, sizeof(PoppinsMedium), 41.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    font::s_inter_semibold = io.Fonts->AddFontFromMemoryTTF(PoppinsSemiBold, sizeof(PoppinsSemiBold), 17.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    font::bold_font = io.Fonts->AddFontFromMemoryTTF(PoppinsBold, sizeof(PoppinsBold), 23.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(icomoon_compressed_data_base85, 32.f, &icomoon_config, icomoon_ranges);
    font::inter_medium = io.Fonts->AddFontFromMemoryTTF(PoppinsMedium, sizeof(PoppinsMedium), 17.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ImVec4 clear_color = ImColor(8, 8, 10, 255);

    D3DX11_IMAGE_LOAD_INFO info;
    ID3DX11ThreadPump* pump{ nullptr };
    if (texture::logotype_image == nullptr)
        D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, logotype, sizeof(logotype), &info, pump, &texture::logotype_image, 0);

    ImGuiStyle& s = ImGui::GetStyle();
    s.FramePadding = ImVec2(18, 10);
    s.ItemSpacing = ImVec2(4, 10);
    s.FrameRounding = 2.f;
    s.WindowRounding = 20.f;
    s.WindowBorderSize = 0.f;
    s.PopupBorderSize = 0.f;
    s.WindowPadding = ImVec2(20, 20);
    s.ChildBorderSize = 1.f;
    s.Colors[ImGuiCol_Border] = ImVec4(0.f, 0.f, 0.f, 0.f);
    s.Colors[ImGuiCol_Separator] = ImVec4(1.f, 1.f, 1.f, 0.2f);
    s.Colors[ImGuiCol_BorderShadow] = ImVec4(0.f, 0.f, 0.f, 0.f);
    s.WindowShadowSize = 0;
    s.PopupRounding = 5.f;
    s.ScrollbarSize = 1;
    s.SeparatorTextPadding = ImVec2(10, 10);

    // Trimmed LianFlow tab set — Fortnite leftovers / duplicate demo pages removed.
    std::vector<s_tab> tabs_info;
    tabs_info.push_back({ "Combat",  { "Aim", "Trigger" } });
    tabs_info.push_back({ "Visuals", { "Players", "Radar" } });
    tabs_info.push_back({ "Misc",    { "General" } });
    tabs_info.push_back({ "Settings",{ "Menu", "Config" } });

    c_tabs p_tabs(tabs_info);
    CNotifications p_notif;
    (void)offsets::client::dwEntityList;

    bool done = false;
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

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        NewFrame();

        LoadImages();

        if (texture::window_bg)
            ImGui::GetBackgroundDrawList()->AddImage(texture::window_bg, ImVec2(0, 0), ImVec2((float)screen_w, (float)screen_h));

        ImGui::SetNextWindowSize(c::bg::size);
        Begin("CS2", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Insert))
                g_menu_open = !g_menu_open;

            c::anim::speed = ImGui::GetIO().DeltaTime * 14.f;
            c::second_color = utils::GetDarkColor(c::main_color);

            const ImVec2& pos = ImGui::GetWindowPos();

            g_menu_alpha = ImLerp(g_menu_alpha, g_menu_open ? 1.f : 0.12f, c::anim::speed);
            s.Alpha = g_menu_alpha;

            draw_background_blur(GetBackgroundDrawList(), g_pSwapChain, g_pd3dDevice, g_pd3dDeviceContext, pos, pos + c::bg::size, c::bg::rounding);
            GetBackgroundDrawList()->AddRectFilled(pos, pos + c::bg::size, utils::GetColorWithAlpha(c::window_bg_color, c::window_bg_color.Value.w * s.Alpha), c::bg::rounding);

            GetBackgroundDrawList()->AddText(pos + ImVec2(15, c::bg::size.y - 35), c::label::default, "setup only  |  INSERT toggles menu");

            GetBackgroundDrawList()->AddRectFilled(pos, pos + ImVec2(c::bg::size.x, 70), GetColorU32(c::child::background), c::bg::rounding, ImDrawFlags_RoundCornersTopRight);

            PushFont(font::bold_font);
            GetBackgroundDrawList()->AddText(utils::center_text(pos, pos + ImVec2(70, 70), ICON_FIRE_FILL) + ImVec2(0, 4.5f), main_color, ICON_FIRE_FILL);
            GetBackgroundDrawList()->AddText(ImVec2(pos.x + 60, utils::center_text(pos, pos + ImVec2(70, 70), "CS2").y), c::label::active, "CS2");
            PopFont();

            GetBackgroundDrawList()->AddText(pos + ImVec2(c::bg::size.x - 24 - CalcTextSize("private").x, 18), c::label::active, "private");
            PushFont(font::esp_font);
            GetBackgroundDrawList()->AddText(pos + ImVec2(c::bg::size.x - 24 - CalcTextSize("HVH setup").x, 37), c::label::default, "HVH setup");
            PopFont();

            p_tabs.DrawTabs();

            const float half_w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
            const float full_h = 450.f;

            if (p_tabs.IsTabActive(0))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                custom::Child("Aim##L", ImVec2(half_w, full_h), true);
                custom::Checkbox("Enable", &g_menu.aim_enable);
                custom::Checkbox("Visible only", &g_menu.aim_visible);
                custom::Checkbox("Recoil compensation", &g_menu.aim_recoil);
                custom::SliderInt("FOV", &g_menu.aim_fov, 1, 30);
                custom::SliderInt("Smooth", &g_menu.aim_smooth, 1, 40);
                custom::Combo("Bone", &g_menu.aim_bone, kBones, IM_ARRAYSIZE(kBones));
                custom::ColorEdit4("FOV color", g_menu.fov_color, picker_flags);
                custom::Keybind("Aim key", &g_menu.aim_key, &g_menu.aim_key_mode);
                custom::EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);
                custom::Child("Notes##R", ImVec2(half_w, full_h), true);
                ImGui::TextWrapped("UI placeholder. These controls do not touch CS2.");
                ImGui::Dummy(ImVec2(0, 8));
                ImGui::TextWrapped("Paste fresh RVAs into src/sdk/offsets.hpp from cheatoffsets.com before any runtime work.");
                custom::EndChild();
            }

            if (p_tabs.IsTabActive(1))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                custom::Child("Trigger##L", ImVec2(ImGui::GetContentRegionAvail().x - 21, full_h), true);
                custom::Checkbox("Enable", &g_menu.trigger_enable);
                custom::SliderInt("Delay (ms)", &g_menu.trigger_delay_ms, 0, 200);
                custom::Keybind("Trigger key", &g_menu.trigger_key, &g_menu.trigger_key_mode);
                custom::EndChild();
            }

            if (p_tabs.IsTabActive(2))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                custom::Child("Players##L", ImVec2(half_w, full_h), true);
                custom::Checkbox("Box", &g_menu.vis_box);
                custom::Checkbox("Health", &g_menu.vis_health);
                custom::Checkbox("Skeleton", &g_menu.vis_skeleton);
                custom::Checkbox("Name", &g_menu.vis_name);
                custom::Checkbox("Weapon", &g_menu.vis_weapon);
                custom::Checkbox("Team check", &g_menu.vis_team_check);
                custom::SliderInt("Box thickness", &g_menu.box_thickness, 1, 6);
                custom::Combo("Filter", &g_menu.team_filter, kTeamFilter, IM_ARRAYSIZE(kTeamFilter));
                custom::ColorEdit4("Visible", g_menu.box_visible, picker_flags);
                custom::ColorEdit4("Hidden", g_menu.box_hidden, picker_flags);
                custom::EndChild();

                ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x * 3);
                custom::Child("Preview##R", ImVec2(half_w, full_h), true);
                ImGui::SetCursorPos(ImVec2(0, 40));
                ImGui::BeginChild("EspPreview");
                PushFont(font::esp_font);
                m_esp_draw.set_positions();
                m_esp_draw.on_draw();
                PopFont();
                ImGui::EndChild();
                custom::EndChild();
            }

            if (p_tabs.IsTabActive(3))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                custom::Child("Radar", ImVec2(ImGui::GetContentRegionAvail().x - 21, full_h), true);
                custom::Checkbox("Enable", &g_menu.radar_enable);
                custom::SliderInt("Size", &g_menu.radar_size, 80, 400);
                custom::SliderInt("Range", &g_menu.radar_range, 500, 8000);
                custom::EndChild();
            }

            if (p_tabs.IsTabActive(4))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                custom::Child("General", ImVec2(ImGui::GetContentRegionAvail().x - 21, full_h), true);
                custom::Checkbox("Watermark", &g_menu.misc_watermark);
                custom::Checkbox("Crosshair", &g_menu.misc_crosshair);
                custom::EndChild();
            }

            if (p_tabs.IsTabActive(5))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                custom::Child("Menu", ImVec2(ImGui::GetContentRegionAvail().x - 21, full_h), true);
                custom::ColorEdit4("Accent", (float*)&c::main_color, picker_flags);
                custom::Keybind("Menu key (Insert still works)", &g_menu.menu_key, &g_menu.menu_key_mode);
                ImGui::TextWrapped("Strip more widgets in src/app/main.cpp if you do not need them.");
                custom::EndChild();
            }

            if (p_tabs.IsTabActive(6))
            {
                ImGui::SetCursorPos(ImVec2(200.f, 85 + page_offset));
                custom::Child("Config", ImVec2(ImGui::GetContentRegionAvail().x - 21, full_h), true);
                ImGui::TextWrapped("Save / Load are not wired. Target file: config/settings.json");
                ImGui::Dummy(ImVec2(0, 8));
                custom::Button("Save (todo)", ImVec2(ImGui::GetContentRegionAvail().x, 44));
                custom::Button("Load (todo)", ImVec2(ImGui::GetContentRegionAvail().x, 44));
                custom::EndChild();
            }

            p_notif.Render();
        }
        End();

        Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
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
    sd.BufferDesc.RefreshRate.Numerator = 60;
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
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
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
