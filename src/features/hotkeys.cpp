#include "features/hotkeys.hpp"
#include "app/settings.hpp"

#include "imgui.h"

#include <algorithm>
#include <cstdio>

void hotkey_name(int vk, char* out, int out_size)
{
    if (!out || out_size <= 0)
        return;
    const char* s = nullptr;
    switch (vk) {
    case 0x01: s = "Mouse1"; break;
    case 0x02: s = "Mouse2"; break;
    case 0x04: s = "Mouse3"; break;
    case 0x05: s = "Mouse4"; break;
    case 0x06: s = "Mouse5"; break;
    case 0x08: s = "Back"; break;
    case 0x09: s = "Tab"; break;
    case 0x0D: s = "Enter"; break;
    case 0x10: s = "Shift"; break;
    case 0x11: s = "Ctrl"; break;
    case 0x12: s = "ALT"; break;
    case 0x14: s = "Caps"; break;
    case 0x1B: s = "ESC"; break;
    case 0x20: s = "Space"; break;
    default: break;
    }
    if (s) {
        std::snprintf(out, static_cast<size_t>(out_size), "%s", s);
        return;
    }
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) {
        out[0] = static_cast<char>(vk);
        out[1] = 0;
        return;
    }
    if (vk >= 0x70 && vk <= 0x87) {
        std::snprintf(out, static_cast<size_t>(out_size), "F%d", vk - 0x6F);
        return;
    }
    if (vk <= 0 || vk > 255)
        std::snprintf(out, static_cast<size_t>(out_size), "--");
    else
        std::snprintf(out, static_cast<size_t>(out_size), "0x%02X", vk);
}

namespace {

void draw_switch(ImDrawList* dl, ImVec2 c, bool on)
{
    // 26x14 pill + knob, soft green when on, grey when off.
    const float w = 26.f, h = 14.f, r = 7.f;
    const ImVec2 mn(c.x - w * 0.5f, c.y - h * 0.5f);
    const ImVec2 mx(c.x + w * 0.5f, c.y + h * 0.5f);
    dl->AddRectFilled(mn, mx,
        on ? IM_COL32(130, 190, 160, 230) : IM_COL32(70, 76, 86, 170), r);
    const float kx = on ? (mx.x - 2.f - 5.f) : (mn.x + 2.f + 5.f);
    dl->AddCircleFilled(ImVec2(kx, c.y), 5.f, IM_COL32(240, 244, 248, 255), 16);
}

} // namespace

void draw_hotkeys(ImDrawList* dl)
{
    if (!dl || !g_menu.misc_hotkeys)
        return;

    struct Row { const char* label; bool on; int key; bool keyed; };
    Row rows[] = {
        { "ESP",        g_menu.vis_enable,      0,                  false },
        { "Aimbot",     g_menu.aim_enable,      g_menu.aim_key,     true },
        { "Triggerbot", g_menu.trigger_enable,  g_menu.trigger_key, true },
    };

    char kn[3][16]{};
    for (int i = 0; i < 3; ++i)
        if (rows[i].keyed)
            hotkey_name(rows[i].key, kn[i], static_cast<int>(sizeof(kn[i])));

    float name_w = 0.f, key_w = 0.f;
    for (int i = 0; i < 3; ++i) {
        name_w = (std::max)(name_w, ImGui::CalcTextSize(rows[i].label).x);
        if (rows[i].keyed)
            key_w = (std::max)(key_w, ImGui::CalcTextSize(kn[i]).x + 14.f);
    }

    const float pad = 12.f;
    const float sw_w = 26.f, gap1 = 9.f, gap2 = 10.f;
    const float row_h = 24.f, header_h = 26.f;
    const float panel_w = pad * 2.f + sw_w + gap1 + name_w + gap2 + key_w;
    const float panel_h = pad + header_h + 4.f + 3.f * row_h + 10.f;

    // Fixed spot under the watermark (watermark: y 14..52 at x 16).
    const ImVec2 a(16.f, 60.f);
    const ImVec2 b(a.x + panel_w, a.y + panel_h);

    dl->AddShadowRect(a, b, IM_COL32(0, 0, 0, 70), 16.f, ImVec2(0, 4),
                      ImDrawFlags_ShadowCutOutShapeBackground, 11.f);
    dl->AddRectFilled(a, b, IM_COL32(10, 12, 16, 150), 11.f);
    dl->AddRect(a, b, IM_COL32(220, 230, 240, 26), 11.f, 0, 1.f);

    // Header: list icon + "Hotkeys".
    const float hcy = a.y + pad + header_h * 0.5f;
    const float ix = a.x + pad;
    const ImU32 icon_c = IM_COL32(240, 175, 90, 230);
    dl->AddLine(ImVec2(ix, hcy - 5.f), ImVec2(ix + 12.f, hcy - 5.f), icon_c, 2.f);
    dl->AddLine(ImVec2(ix, hcy), ImVec2(ix + 12.f, hcy), icon_c, 2.f);
    dl->AddLine(ImVec2(ix, hcy + 5.f), ImVec2(ix + 12.f, hcy + 5.f), icon_c, 2.f);
    {
        const char* title = "Hotkeys";
        const ImVec2 tsz = ImGui::CalcTextSize(title);
        dl->AddText(ImVec2(ix + 18.f, hcy - tsz.y * 0.5f),
                    IM_COL32(220, 228, 236, 205), title);
    }

    const float div_y = a.y + pad + header_h;
    dl->AddLine(ImVec2(a.x + pad, div_y), ImVec2(b.x - pad, div_y),
                IM_COL32(220, 230, 240, 24), 1.f);

    const float pill_h = 18.f;
    for (int i = 0; i < 3; ++i) {
        const float cy = div_y + 4.f + i * row_h + row_h * 0.5f;
        draw_switch(dl, ImVec2(a.x + pad + sw_w * 0.5f, cy), rows[i].on);

        const ImVec2 tsz = ImGui::CalcTextSize(rows[i].label);
        dl->AddText(ImVec2(a.x + pad + sw_w + gap1, cy - tsz.y * 0.5f),
            rows[i].on ? IM_COL32(220, 228, 236, 210) : IM_COL32(150, 158, 168, 150),
            rows[i].label);

        if (rows[i].keyed) {
            const ImVec2 p1(b.x - pad - key_w, cy - pill_h * 0.5f);
            const ImVec2 p2(b.x - pad, cy + pill_h * 0.5f);
            dl->AddRectFilled(p1, p2, IM_COL32(255, 255, 255, 16), 5.f);
            dl->AddRect(p1, p2, IM_COL32(220, 230, 240, 22), 5.f, 0, 1.f);
            const ImVec2 ksz = ImGui::CalcTextSize(kn[i]);
            dl->AddText(ImVec2(p1.x + (key_w - ksz.x) * 0.5f, cy - ksz.y * 0.5f),
                        IM_COL32(205, 212, 220, 210), kn[i]);
        }
    }
}
