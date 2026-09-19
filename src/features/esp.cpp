#include "features/esp.hpp"
#include "app/settings.hpp"
#include "sdk/vis.hpp"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

ImU32 with_a(ImVec4 c, float a)
{
    c.w = std::clamp(a, 0.f, 1.f);
    return ImGui::ColorConvertFloat4ToU32(c);
}

ImVec4 from_arr(const float c[4])
{
    return ImVec4(c[0], c[1], c[2], c[3]);
}

void clean_line(ImDrawList* dl, ImVec2 a, ImVec2 b, ImVec4 col, float thickness, float glow, float alpha)
{
    col.w *= alpha;
    if (col.w <= 0.01f)
        return;

    if (glow > 0.1f) {
        ImVec4 g = col;
        g.w *= 0.16f;
        dl->AddLine(a, b, ImGui::ColorConvertFloat4ToU32(g), thickness + glow);
    }

    dl->AddLine(a, b, with_a(ImVec4(0.02f, 0.03f, 0.04f, 1.f), col.w * 0.55f), thickness + 0.9f);
    dl->AddLine(a, b, ImGui::ColorConvertFloat4ToU32(col), thickness);

    ImVec4 hi = col;
    hi.x = std::min(hi.x * 1.18f + 0.10f, 1.f);
    hi.y = std::min(hi.y * 1.18f + 0.10f, 1.f);
    hi.z = std::min(hi.z * 1.18f + 0.10f, 1.f);
    hi.w *= 0.70f;
    dl->AddLine(a, b, ImGui::ColorConvertFloat4ToU32(hi), std::max(1.f, thickness * 0.38f));
}

void frame_corner(ImDrawList* dl, ImVec2 c, float sx, float sy, float len, float beam, ImVec4 col, float glow, float alpha)
{
    const float x1 = c.x + sx * len;
    const float y1 = c.y + sy * beam;
    const float x2 = c.x + sx * beam;
    const float y2 = c.y + sy * len;

    ImVec2 ha(std::min(c.x, x1), std::min(c.y, y1));
    ImVec2 hb(std::max(c.x, x1), std::max(c.y, y1));
    ImVec2 va(std::min(c.x, x2), std::min(c.y, y2));
    ImVec2 vb(std::max(c.x, x2), std::max(c.y, y2));

    ImVec4 glow_c = col;
    glow_c.w *= alpha * 0.22f;
    if (glow > 0.2f) {
        dl->AddShadowRect(ha, hb, ImGui::ColorConvertFloat4ToU32(glow_c), glow + 4.f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, 0.f);
        dl->AddShadowRect(va, vb, ImGui::ColorConvertFloat4ToU32(glow_c), glow + 4.f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, 0.f);
    }

    ImVec4 fill = col;
    fill.x *= 0.22f;
    fill.y *= 0.24f;
    fill.z *= 0.28f;
    fill.w = 0.55f * alpha;
    dl->AddRectFilled(ha, hb, ImGui::ColorConvertFloat4ToU32(fill));
    dl->AddRectFilled(va, vb, ImGui::ColorConvertFloat4ToU32(fill));

    ImVec4 inner = col;
    inner.x *= 0.42f;
    inner.y *= 0.44f;
    inner.z *= 0.48f;
    inner.w = 0.70f * alpha;

    ImVec4 outer = col;
    outer.x = std::min(outer.x * 1.12f + 0.16f, 1.f);
    outer.y = std::min(outer.y * 1.12f + 0.16f, 1.f);
    outer.z = std::min(outer.z * 1.12f + 0.16f, 1.f);
    outer.w = 0.92f * alpha;

    const ImVec2 outer_h(c.x + sx * len, c.y);
    const ImVec2 outer_v(c.x, c.y + sy * len);
    dl->AddLine(c, outer_h, ImGui::ColorConvertFloat4ToU32(outer), 1.15f);
    dl->AddLine(c, outer_v, ImGui::ColorConvertFloat4ToU32(outer), 1.15f);

    const ImVec2 in_c(c.x + sx * beam, c.y + sy * beam);
    const ImVec2 in_h(c.x + sx * len, c.y + sy * beam);
    const ImVec2 in_v(c.x + sx * beam, c.y + sy * len);
    dl->AddLine(in_c, in_h, ImGui::ColorConvertFloat4ToU32(inner), 1.f);
    dl->AddLine(in_c, in_v, ImGui::ColorConvertFloat4ToU32(inner), 1.f);

    ImVec4 cap = outer;
    cap.w *= 0.35f;
    dl->AddLine(outer_h, in_h, ImGui::ColorConvertFloat4ToU32(cap), 1.f);
    dl->AddLine(outer_v, in_v, ImGui::ColorConvertFloat4ToU32(cap), 1.f);
}

bool project_bounds(const Player& p, const Mat4x4& vm, float sw, float sh, ImVec2& mn, ImVec2& mx, ImVec2 out_corners[8], int& valid_n)
{
    mn = ImVec2(sw, sh);
    mx = ImVec2(0.f, 0.f);
    valid_n = 0;

    for (int i = 0; i < 8; ++i)
        out_corners[i] = ImVec2(-1.f, -1.f);

    for (int i = 0; i < 8; ++i) {
        Vec2 s{};
        if (!world_to_screen(p.corners[i], vm, sw, sh, s))
            continue;
        out_corners[i] = ImVec2(s.x, s.y);
        ++valid_n;
        mn.x = std::min(mn.x, s.x);
        mn.y = std::min(mn.y, s.y);
        mx.x = std::max(mx.x, s.x);
        mx.y = std::max(mx.y, s.y);
    }

    if (valid_n < 4)
        return false;
    if (mx.x - mn.x < 4.f || mx.y - mn.y < 8.f)
        return false;
    if (mx.x < 0.f || mx.y < 0.f || mn.x > sw || mn.y > sh)
        return false;
    return true;
}

ImVec4 health_vec(float t)
{
    t = std::clamp(t, 0.f, 1.f);
    if (t > 0.5f) {
        const float k = (t - 0.5f) * 2.f;
        return ImVec4(
            0.92f * (1.f - k) + 0.28f * k,
            0.78f * (1.f - k) + 0.84f * k,
            0.22f * (1.f - k) + 0.42f * k,
            1.f);
    }
    const float k = t * 2.f;
    return ImVec4(
        0.84f * (1.f - k) + 0.92f * k,
        0.16f * (1.f - k) + 0.78f * k,
        0.18f * (1.f - k) + 0.22f * k,
        1.f);
}

void draw_health_bar(ImDrawList* dl, ImVec2 box_min, ImVec2 box_max, float hp, float alpha, float time_s)
{
    hp = std::clamp(hp, 0.f, 1.f);
    const float h = box_max.y - box_min.y;
    const float w = std::clamp(h * 0.028f, 2.6f, 3.6f);
    const float gap = 6.f;
    const ImVec2 a(box_max.x + gap, box_min.y);
    const ImVec2 b(a.x + w, box_max.y);
    const float rnd = w * 0.5f;

    ImVec4 hc = health_vec(hp);
    hc.w = 0.18f * alpha;
    dl->AddShadowRect(ImVec2(a.x - 1.f, a.y - 1.f), ImVec2(b.x + 1.f, b.y + 1.f),
                      ImGui::ColorConvertFloat4ToU32(hc), 8.f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, rnd);

    dl->AddRectFilled(a, b, IM_COL32(6, 8, 10, (int)(155 * alpha)), rnd);
    dl->AddRect(a, b, IM_COL32(8, 10, 12, (int)(210 * alpha)), rnd, 0, 1.f);

    const float fill_h = h * hp;
    if (fill_h > 1.f) {
        const ImVec2 fa(a.x + 0.7f, b.y - fill_h);
        const ImVec2 fb(b.x - 0.7f, b.y - 0.6f);
        ImVec4 top = health_vec(hp);
        ImVec4 bot = health_vec(std::max(0.f, hp - 0.35f));
        top.w = bot.w = 0.92f * alpha;
        dl->AddRectFilledMultiColor(fa, fb,
            ImGui::ColorConvertFloat4ToU32(top), ImGui::ColorConvertFloat4ToU32(top),
            ImGui::ColorConvertFloat4ToU32(bot), ImGui::ColorConvertFloat4ToU32(bot));

        const float shimmer = 0.07f + 0.04f * (0.5f + 0.5f * std::sinf(time_s * 1.35f));
        dl->AddRectFilled(ImVec2(fa.x, fa.y), ImVec2(fa.x + w * 0.34f, fb.y),
                          IM_COL32(255, 255, 255, (int)(shimmer * 255.f * alpha)), rnd * 0.35f);
    }
}

void draw_corner_box(ImDrawList* dl, ImVec2 mn, ImVec2 mx, ImVec4 col, float beam, float glow, float corner_pct, float alpha)
{
    const float w = mx.x - mn.x;
    const float h = mx.y - mn.y;
    const float len = std::clamp(std::min(w, h) * corner_pct, 8.f, std::min(w, h) * 0.38f);

    frame_corner(dl, ImVec2(mn.x, mn.y),  1.f,  1.f, len, beam, col, glow, alpha);
    frame_corner(dl, ImVec2(mx.x, mn.y), -1.f,  1.f, len, beam, col, glow, alpha);
    frame_corner(dl, ImVec2(mn.x, mx.y),  1.f, -1.f, len, beam, col, glow, alpha);
    frame_corner(dl, ImVec2(mx.x, mx.y), -1.f, -1.f, len, beam, col, glow, alpha);
}

void draw_filled_box(ImDrawList* dl, ImVec2 mn, ImVec2 mx, ImVec4 col, float alpha)
{
    ImVec4 top = col;
    ImVec4 bot = col;
    top.w = 0.035f * alpha;
    bot.w = 0.11f * alpha;
    bot.x *= 0.75f;
    bot.y *= 0.75f;
    bot.z *= 0.80f;
    dl->AddRectFilledMultiColor(mn, mx,
        ImGui::ColorConvertFloat4ToU32(top), ImGui::ColorConvertFloat4ToU32(top),
        ImGui::ColorConvertFloat4ToU32(bot), ImGui::ColorConvertFloat4ToU32(bot));
}

void draw_3d_box(ImDrawList* dl, ImVec2 c[8], ImVec4 col, float thickness, float glow, float alpha)
{
    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7},
    };
    for (const auto& e : edges) {
        if (c[e[0]].x < 0.f || c[e[1]].x < 0.f)
            continue;
        clean_line(dl, c[e[0]], c[e[1]], col, thickness, glow, alpha);
    }
}

} // namespace

void draw_players(ImDrawList* dl, const Game& game, const VisCheck& vis, float screen_w, float screen_h, float time_s)
{
    if (!g_menu.vis_enable || !dl)
        return;

    const float beam = std::clamp(g_menu.vis_thickness / 10.f, 1.6f, 3.2f);
    const float line_t = std::clamp(g_menu.vis_thickness / 12.f, 0.9f, 1.6f);
    const float glow = g_menu.vis_glow / 12.f;
    const float corner_pct = std::clamp(g_menu.vis_corner / 100.f, 0.14f, 0.36f);
    const float max_dist = static_cast<float>(g_menu.vis_max_distance) * 10.f;
    const bool vis_on = g_menu.vis_visible_only && vis.ready();

    for (const Player& p : game.players()) {
        if (g_menu.vis_team_check && p.team == game.local_team())
            continue;
        if (max_dist > 1.f && p.distance > max_dist)
            continue;
        if (vis_on && !vis.visible(game.local_head(), p.head))
            continue;

        ImVec2 corners[8]{};
        ImVec2 mn, mx;
        int n = 0;
        if (!project_bounds(p, game.view_matrix(), screen_w, screen_h, mn, mx, corners, n))
            continue;

        mn.x -= 3.5f; mn.y -= 2.5f;
        mx.x += 3.5f; mx.y += 2.5f;

        const float dist_a = std::clamp(1.f - (p.distance - 350.f) / 3600.f, 0.42f, 1.f);
        const float move_a = 0.90f + 0.10f * std::clamp(p.speed / 180.f, 0.f, 1.f);
        const float pulse = 0.975f + 0.025f * std::sinf(time_s * 1.55f + p.origin.x * 0.02f);
        const float alpha = dist_a * move_a * pulse;
        const float hp = (p.max_health > 0) ? (p.health / static_cast<float>(p.max_health)) : 0.f;

        const bool teammate = p.team == game.local_team();
        const ImVec4 col = from_arr(teammate ? g_menu.box_team : g_menu.box_enemy);
        const BoxStyle style = static_cast<BoxStyle>(g_menu.vis_box_style);

        if (style == BoxStyle::Filled)
            draw_filled_box(dl, mn, mx, col, alpha);

        if (style == BoxStyle::Box3D && n == 8)
            draw_3d_box(dl, corners, col, line_t, glow, alpha);
        else
            draw_corner_box(dl, mn, mx, col, beam, glow, corner_pct, alpha);

        if (g_menu.vis_health)
            draw_health_bar(dl, mn, mx, hp, alpha, time_s);
    }
}

void draw_watermark(ImDrawList* dl, const Game& game, const VisCheck& vis, float /*screen_w*/)
{
    if (!g_menu.misc_watermark || !dl)
        return;

    char line[192];
    std::snprintf(line, sizeof(line), "CS2  ·  %s  ·  %s  ·  vis %s (%zu tris)  ·  INSERT  ·  F8",
                  game.attached() ? "live" : "waiting",
                  game.map_name().empty() ? "no map" : game.map_name().c_str(),
                  vis.ready() ? "on" : "off",
                  vis.triangles());

    const ImVec2 p(18.f, 14.f);
    const ImVec2 sz = ImGui::CalcTextSize(line);
    dl->AddRectFilled(ImVec2(p.x - 10.f, p.y - 6.f), ImVec2(p.x + sz.x + 10.f, p.y + sz.y + 6.f),
                      IM_COL32(10, 12, 16, 110), 8.f);
    dl->AddRect(ImVec2(p.x - 10.f, p.y - 6.f), ImVec2(p.x + sz.x + 10.f, p.y + sz.y + 6.f),
                IM_COL32(220, 230, 240, 22), 8.f, 0, 1.f);
    dl->AddText(p, IM_COL32(220, 228, 236, 190), line);
}
