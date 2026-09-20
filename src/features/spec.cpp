#include "features/spec.hpp"
#include "features/panels.hpp"
#include "app/settings.hpp"
#include "sdk/game.hpp"

#include "imgui.h"

#include <algorithm>
#include <vector>

void draw_spectators(ImDrawList* dl, const Game& game, float screen_w, float screen_h)
{
    if (!dl || !g_menu.spec_enable)
        return;

    const std::vector<std::string>& names = game.spectators();

    // Measure first so the panel hugs its content.
    float text_w = ImGui::CalcTextSize("No spectators").x;
    for (const std::string& n : names)
        text_w = (std::max)(text_w, ImGui::CalcTextSize(n.c_str()).x);

    const float pad = 12.f;
    const float header_h = 26.f;
    const float row_h = 22.f;
    const size_t rows = names.empty() ? 1 : names.size();
    const float panel_w = pad * 2.f + 10.f + 8.f + text_w;
    const float panel_h = pad + header_h + 4.f + static_cast<float>(rows) * row_h + 8.f;

    const ImVec2 a = panel_anchor(g_menu.spec_anchor, panel_w, panel_h, screen_w, screen_h,
                                  static_cast<float>(g_menu.spec_off_x),
                                  static_cast<float>(g_menu.spec_off_y));
    const ImVec2 b(a.x + panel_w, a.y + panel_h);

    dl->AddShadowRect(a, b, IM_COL32(0, 0, 0, 70), 16.f, ImVec2(0, 4),
                      ImDrawFlags_ShadowCutOutShapeBackground, 11.f);
    dl->AddRectFilled(a, b, IM_COL32(10, 12, 16, 150), 11.f);
    dl->AddRect(a, b, IM_COL32(220, 230, 240, 26), 11.f, 0, 1.f);

    // Header: eye glyph + "Spectators".
    const float hcy = a.y + pad + header_h * 0.5f;
    const ImVec2 ec(a.x + pad + 6.f, hcy);
    const ImU32 eye_c = IM_COL32(150, 200, 235, 230);
    dl->AddCircle(ec, 6.f, eye_c, 16, 1.6f);
    dl->AddCircleFilled(ec, 2.4f, eye_c, 12);
    {
        const char* title = "Spectators";
        const ImVec2 tsz = ImGui::CalcTextSize(title);
        dl->AddText(ImVec2(ec.x + 11.f, hcy - tsz.y * 0.5f),
                    IM_COL32(220, 228, 236, 205), title);
    }

    const float div_y = a.y + pad + header_h;
    dl->AddLine(ImVec2(a.x + pad, div_y), ImVec2(b.x - pad, div_y),
                IM_COL32(220, 230, 240, 24), 1.f);

    if (names.empty()) {
        const char* hint = "No spectators";
        const ImVec2 tsz = ImGui::CalcTextSize(hint);
        const float cy = div_y + 4.f + row_h * 0.5f;
        dl->AddText(ImVec2(a.x + pad, cy - tsz.y * 0.5f),
                    IM_COL32(140, 148, 158, 150), hint);
        return;
    }

    for (size_t i = 0; i < names.size(); ++i) {
        const float cy = div_y + 4.f + i * row_h + row_h * 0.5f;
        dl->AddCircleFilled(ImVec2(a.x + pad + 5.f, cy), 3.f,
                            IM_COL32(132, 196, 164, 230), 12);
        const ImVec2 tsz = ImGui::CalcTextSize(names[i].c_str());
        dl->AddText(ImVec2(a.x + pad + 10.f + 8.f, cy - tsz.y * 0.5f),
                    IM_COL32(220, 228, 236, 210), names[i].c_str());
    }
}
