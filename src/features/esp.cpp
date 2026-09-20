#define IMGUI_DEFINE_MATH_OPERATORS
#include "features/esp.hpp"
#include "app/settings.hpp"
#include "sdk/skel_log.hpp"
#include "sdk/skeleton.hpp"
#include "sdk/vis.hpp"

#include "imgui.h"
#include "imgui_settings.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

namespace {

struct SkeletonLink {
    Skel from;
    Skel to;
    SkeletonBodyMode minimum_mode;
    float min_len;
    float max_len;
};

struct SkeletonDot {
    Skel joint;
    SkeletonBodyMode minimum_mode;
};

constexpr SkeletonLink kSkeletonLinks[] = {
    { Skel::Head,       Skel::Neck,       SkeletonBodyMode::Head,  2.f, 28.f },
    { Skel::Neck,       Skel::SpineUpper, SkeletonBodyMode::Upper, 2.f, 40.f },
    { Skel::SpineUpper, Skel::SpineLower, SkeletonBodyMode::Upper, 2.f, 45.f },
    { Skel::SpineLower, Skel::Pelvis,     SkeletonBodyMode::Upper, 2.f, 50.f },
    { Skel::Neck,       Skel::LShoulder,  SkeletonBodyMode::Upper, 2.f, 50.f },
    { Skel::LShoulder,  Skel::LElbow,     SkeletonBodyMode::Upper, 4.f, 55.f },
    { Skel::LElbow,     Skel::LHand,      SkeletonBodyMode::Upper, 2.f, 50.f },
    { Skel::Neck,       Skel::RShoulder,  SkeletonBodyMode::Upper, 2.f, 50.f },
    { Skel::RShoulder,  Skel::RElbow,     SkeletonBodyMode::Upper, 4.f, 55.f },
    { Skel::RElbow,     Skel::RHand,      SkeletonBodyMode::Upper, 2.f, 50.f },
    { Skel::Pelvis,     Skel::LHip,       SkeletonBodyMode::Full,  2.f, 50.f },
    { Skel::LHip,       Skel::LKnee,      SkeletonBodyMode::Full,  4.f, 65.f },
    { Skel::LKnee,      Skel::LFoot,      SkeletonBodyMode::Full,  2.f, 60.f },
    { Skel::Pelvis,     Skel::RHip,       SkeletonBodyMode::Full,  2.f, 50.f },
    { Skel::RHip,       Skel::RKnee,      SkeletonBodyMode::Full,  4.f, 65.f },
    { Skel::RKnee,      Skel::RFoot,      SkeletonBodyMode::Full,  2.f, 60.f },
};

constexpr SkeletonDot kSkeletonDots[] = {
    { Skel::Head,       SkeletonBodyMode::Head },
    { Skel::Neck,       SkeletonBodyMode::Head },
    { Skel::SpineUpper, SkeletonBodyMode::Upper },
    { Skel::SpineLower, SkeletonBodyMode::Upper },
    { Skel::Pelvis,     SkeletonBodyMode::Upper },
    { Skel::LShoulder,  SkeletonBodyMode::Upper },
    { Skel::LElbow,     SkeletonBodyMode::Upper },
    { Skel::LHand,      SkeletonBodyMode::Upper },
    { Skel::RShoulder,  SkeletonBodyMode::Upper },
    { Skel::RElbow,     SkeletonBodyMode::Upper },
    { Skel::RHand,      SkeletonBodyMode::Upper },
    { Skel::LHip,       SkeletonBodyMode::Full },
    { Skel::LKnee,      SkeletonBodyMode::Full },
    { Skel::LFoot,      SkeletonBodyMode::Full },
    { Skel::RHip,       SkeletonBodyMode::Full },
    { Skel::RKnee,      SkeletonBodyMode::Full },
    { Skel::RFoot,      SkeletonBodyMode::Full },
};

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

    dl->AddLine(a, b, with_a(ImVec4(0.02f, 0.03f, 0.04f, 1.f), col.w * 0.42f), thickness + 0.8f);
    dl->AddLine(a, b, ImGui::ColorConvertFloat4ToU32(col), thickness);

    ImVec4 hi = col;
    hi.x = std::min(hi.x * 1.18f + 0.10f, 1.f);
    hi.y = std::min(hi.y * 1.18f + 0.10f, 1.f);
    hi.z = std::min(hi.z * 1.18f + 0.10f, 1.f);
    hi.w *= 0.50f;
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
    fill.w = 0.36f * alpha;
    dl->AddRectFilled(ha, hb, ImGui::ColorConvertFloat4ToU32(fill));
    dl->AddRectFilled(va, vb, ImGui::ColorConvertFloat4ToU32(fill));

    ImVec4 inner = col;
    inner.x *= 0.42f;
    inner.y *= 0.44f;
    inner.z *= 0.48f;
    inner.w = 0.46f * alpha;

    ImVec4 outer = col;
    outer.x = std::min(outer.x * 1.12f + 0.16f, 1.f);
    outer.y = std::min(outer.y * 1.12f + 0.16f, 1.f);
    outer.z = std::min(outer.z * 1.12f + 0.16f, 1.f);
    outer.w = 0.82f * alpha;

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
    cap.w *= 0.20f;
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
    const ImVec4 low = from_arr(g_menu.health_low);
    const ImVec4 high = from_arr(g_menu.health_high);
    if (!g_menu.vis_health_gradient)
        return high;
    return ImVec4(
        low.x * (1.f - t) + high.x * t,
        low.y * (1.f - t) + high.y * t,
        low.z * (1.f - t) + high.z * t,
        1.f);
}

void draw_health_bar(ImDrawList* dl, ImVec2 box_min, ImVec2 box_max, float hp, float alpha, float time_s)
{
    hp = std::clamp(hp, 0.f, 1.f);
    const float h = box_max.y - box_min.y;
    const float w = std::clamp(static_cast<float>(g_menu.vis_health_width), 2.f, 8.f);
    const float gap = 6.f;
    const bool left = g_menu.vis_health_position == static_cast<int>(HealthBarPosition::Left);
    const ImVec2 a = left ? ImVec2(box_min.x - gap - w, box_min.y)
                          : ImVec2(box_max.x + gap, box_min.y);
    const ImVec2 b = left ? ImVec2(box_min.x - gap, box_max.y)
                          : ImVec2(a.x + w, box_max.y);
    const float rnd = std::min(w * 0.45f, 2.f);

    ImVec4 hc = health_vec(hp);
    hc.w = 0.18f * alpha;
    dl->AddShadowRect(ImVec2(a.x - 1.f, a.y - 1.f), ImVec2(b.x + 1.f, b.y + 1.f),
                      ImGui::ColorConvertFloat4ToU32(hc), 6.f, ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, rnd);

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

    if (g_menu.vis_health_value) {
        char value[12]{};
        std::snprintf(value, sizeof(value), "%d", static_cast<int>(std::lround(hp * 100.f)));
        const ImVec2 text_size = ImGui::CalcTextSize(value);
        const float tx = left ? a.x - 4.f - text_size.x : b.x + 4.f;
        const float ty = a.y + 1.f;
        const ImU32 text_shadow = IM_COL32(3, 5, 8, static_cast<int>(180.f * alpha));
        const ImU32 text_col = with_a(health_vec(hp), 0.95f * alpha);
        dl->AddText(ImVec2(tx + 1.f, ty + 1.f), text_shadow, value);
        dl->AddText(ImVec2(tx, ty), text_col, value);
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
    const float w = mx.x - mn.x;
    const float h = mx.y - mn.y;
    if (w < 4.f || h < 8.f || alpha <= 0.01f)
        return;

    const float round = std::clamp(std::min(w, h) * 0.22f, 8.f, 22.f);
    const float a = std::clamp(alpha, 0.f, 1.f);
    ImVec4 wash = col;
    wash.w = 0.11f * a;
    ImVec4 bloom = col;
    bloom.w = 0.16f * a;

    dl->AddShadowRect(mn, mx, ImGui::ColorConvertFloat4ToU32(bloom), 18.f,
                      ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, round);
    dl->AddRectFilled(mn, mx, ImGui::ColorConvertFloat4ToU32(wash), round);

    ImVec4 heel = col;
    heel.x *= 0.85f;
    heel.y *= 0.85f;
    heel.z *= 0.88f;
    heel.w = 0.08f * a;
    const float split = mn.y + h * 0.42f;
    dl->AddRectFilled(ImVec2(mn.x, split), mx, ImGui::ColorConvertFloat4ToU32(heel), round,
                      ImDrawFlags_RoundCornersBottom);
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

void draw_head_marker(ImDrawList* dl, ImVec2 center, float radius, ImVec4 col, HeadMarkerStyle style, float glow, float alpha, float time_s)
{
    if (radius < 2.f || alpha <= 0.01f)
        return;

    const float pulse = 0.92f + 0.08f * std::sinf(time_s * 1.8f);
    ImVec4 halo = col;
    halo.w = 0.16f * alpha * pulse;
    const float shadow_size = std::clamp(glow + 2.f, 4.f, 12.f);
    const ImU32 outline = with_a(ImVec4(0.02f, 0.03f, 0.04f, 1.f), 0.82f * alpha);
    ImVec4 fill = col;
    fill.w = 0.20f * alpha;
    ImVec4 ring = col;
    ring.w = 0.94f * alpha;

    if (style == HeadMarkerStyle::Dot) {
        const float dot_radius = std::max(2.f, radius * 0.48f);
        dl->AddShadowCircle(center, dot_radius, ImGui::ColorConvertFloat4ToU32(halo), shadow_size,
                            ImVec2(0, 0), 0, 24);
        dl->AddCircleFilled(center, dot_radius + 1.1f, outline, 24);
        dl->AddCircleFilled(center, dot_radius, ImGui::ColorConvertFloat4ToU32(ring), 24);
        return;
    }

    if (style == HeadMarkerStyle::Box) {
        const float half = radius * 0.82f;
        const ImVec2 mn(center.x - half, center.y - half);
        const ImVec2 mx(center.x + half, center.y + half);
        dl->AddShadowRect(mn, mx, ImGui::ColorConvertFloat4ToU32(halo), shadow_size,
                          ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, 3.f);
        dl->AddRectFilled(mn, mx, ImGui::ColorConvertFloat4ToU32(fill), 3.f);
        dl->AddRect(mn, mx, outline, 3.f, 0, 2.2f);
        dl->AddRect(mn, mx, ImGui::ColorConvertFloat4ToU32(ring), 3.f, 0, 1.2f);
        return;
    }

    dl->AddShadowCircle(center, radius + 0.5f, ImGui::ColorConvertFloat4ToU32(halo), shadow_size,
                        ImVec2(0, 0), 0, 28);
    dl->AddCircle(center, radius + 1.1f, outline, 28, 2.4f);
    dl->AddCircleFilled(center, radius, ImGui::ColorConvertFloat4ToU32(fill), 28);
    dl->AddCircle(center, radius, ImGui::ColorConvertFloat4ToU32(ring), 28, 1.25f);
}

void draw_skeleton(ImDrawList* dl, const Player& player, const Mat4x4& view, float screen_w, float screen_h,
                   ImVec4 color, float alpha)
{
    if (!g_menu.vis_skeleton || player.joint_mask == 0 || alpha <= 0.01f)
        return;

    const SkeletonBodyMode mode = static_cast<SkeletonBodyMode>(g_menu.vis_skeleton_mode);
    const SkeletonStyle style = static_cast<SkeletonStyle>(g_menu.vis_skeleton_style);
    const bool draw_lines = style != SkeletonStyle::Points;
    const bool draw_points = style != SkeletonStyle::Lines;
    const float thickness = std::clamp(g_menu.vis_skeleton_thickness / 10.f, 0.8f, 3.f);

    std::array<ImVec2, kSkelCount> points{};
    std::array<bool, kSkelCount> attempted{};
    std::array<bool, kSkelCount> projected{};

    auto project = [&](Skel joint) -> bool {
        const int i = static_cast<int>(joint);
        if (!player.has_joint(joint))
            return false;
        if (attempted[i])
            return projected[i];
        attempted[i] = true;
        const Vec3& world = player.joints[i];
        if (!std::isfinite(world.x) || !std::isfinite(world.y) || !std::isfinite(world.z)) {
            skel_log::write("W2S NaN joint=%d pawn=%p", i, reinterpret_cast<void*>(player.pawn));
            return false;
        }
        Vec2 screen{};
        if (!world_to_screen(world, view, screen_w, screen_h, screen))
            return false;
        if (std::fabs(screen.x) > screen_w * 4.f || std::fabs(screen.y) > screen_h * 4.f)
            return false;
        points[i] = ImVec2(screen.x, screen.y);
        projected[i] = true;
        return true;
    };

    color.w *= alpha;
    if (color.w <= 0.01f)
        return;

    int lines = 0;
    int rejected = 0;
    if (draw_lines) {
        const ImU32 edge = with_a(ImVec4(0.02f, 0.03f, 0.04f, 1.f), color.w * 0.72f);
        const ImU32 line = ImGui::ColorConvertFloat4ToU32(color);
        for (const SkeletonLink& link : kSkeletonLinks) {
            if (static_cast<int>(mode) < static_cast<int>(link.minimum_mode))
                continue;
            if (!player.has_joint(link.from) || !player.has_joint(link.to))
                continue;
            const Vec3& a = player.joints[static_cast<int>(link.from)];
            const Vec3& b = player.joints[static_cast<int>(link.to)];
            if (!skel_seg_ok(a, b, link.min_len, link.max_len)) {
                ++rejected;
                continue;
            }
            if (!project(link.from) || !project(link.to))
                continue;
            const ImVec2 pa = points[static_cast<int>(link.from)];
            const ImVec2 pb = points[static_cast<int>(link.to)];
            const float dx = pa.x - pb.x;
            const float dy = pa.y - pb.y;
            if (dx * dx + dy * dy > 420.f * 420.f) {
                ++rejected;
                continue;
            }
            dl->AddLine(pa, pb, edge, thickness + 1.35f);
            dl->AddLine(pa, pb, line, thickness);
            ++lines;
        }
    }

    int dots = 0;
    if (draw_points) {
        const float radius = std::clamp(1.55f + thickness * 0.58f, 2.f, 3.4f);
        const ImU32 edge = with_a(ImVec4(0.02f, 0.03f, 0.04f, 1.f), color.w * 0.78f);
        const ImU32 fill = ImGui::ColorConvertFloat4ToU32(color);
        for (const SkeletonDot& joint : kSkeletonDots) {
            if (static_cast<int>(mode) < static_cast<int>(joint.minimum_mode))
                continue;
            if (g_menu.vis_head && joint.joint == Skel::Head)
                continue;
            if (!project(joint.joint))
                continue;
            const ImVec2 p = points[static_cast<int>(joint.joint)];
            if (p.x < -radius || p.x > screen_w + radius || p.y < -radius || p.y > screen_h + radius)
                continue;
            dl->AddCircleFilled(p, radius + 0.8f, edge, 14);
            dl->AddCircleFilled(p, radius, fill, 14);
            ++dots;
        }
    }

    static bool logged_ok = false;
    if (!logged_ok && (lines > 0 || dots > 0)) {
        logged_ok = true;
        skel_log::write("draw ok pawn=%p mask=0x%05x lines=%d dots=%d rejected=%d",
                        reinterpret_cast<void*>(player.pawn), player.joint_mask, lines, dots, rejected);
    }
}

float distance_alpha(float distance_m, float max_distance_m)
{
    if (max_distance_m <= 0.f)
        return 1.f;
    const float fade_start = max_distance_m * 0.72f;
    if (distance_m <= fade_start)
        return 1.f;
    if (distance_m >= max_distance_m)
        return 0.f;
    const float t = std::clamp((distance_m - fade_start) / (max_distance_m - fade_start), 0.f, 1.f);
    const float smooth = t * t * (3.f - 2.f * t);
    return 1.f - smooth;
}

void draw_distance_label(ImDrawList* dl, ImVec2 box_min, ImVec2 box_max, float distance_m, float alpha)
{
    if (!g_menu.vis_distance || alpha <= 0.01f)
        return;

    char text[24]{};
    std::snprintf(text, sizeof(text), "%dm", std::max(0, static_cast<int>(std::lround(distance_m))));
    const ImVec2 size = ImGui::CalcTextSize(text);
    const float x = (box_min.x + box_max.x - size.x) * 0.5f;
    const float y_below = box_max.y + 4.f;
    const float y = y_below + size.y <= ImGui::GetIO().DisplaySize.y ? y_below : box_min.y - size.y - 4.f;
    const ImU32 shadow = IM_COL32(3, 5, 8, static_cast<int>(175.f * alpha));
    const ImU32 label = IM_COL32(226, 232, 238, static_cast<int>(218.f * alpha));
    dl->AddText(ImVec2(x + 1.f, y + 1.f), shadow, text);
    dl->AddText(ImVec2(x, y), label, text);
}

void draw_player_name(ImDrawList* dl, ImVec2 box_min, ImVec2 box_max, const std::string& name, float alpha,
                        ImU32 label_col = IM_COL32(235, 240, 246, 230))
{
    if (name.empty() || alpha <= 0.01f)
        return;

    const ImVec2 size = ImGui::CalcTextSize(name.c_str());
    const float x = (box_min.x + box_max.x - size.x) * 0.5f;
    const float y = box_min.y - size.y - 3.f;
    if (y < -size.y)
        return;

    // Scale the caller-provided label color by alpha, keep its RGB.
    const float base_a = ((label_col >> 24) & 255) / 255.f;
    const ImU32 shadow = IM_COL32(3, 5, 8, static_cast<int>(180.f * alpha));
    const ImU32 label = (label_col & 0x00FFFFFF) |
        (static_cast<ImU32>(std::clamp(base_a * alpha, 0.f, 1.f) * 255.f) << 24);
    dl->AddText(ImVec2(x + 1.f, y + 1.f), shadow, name.c_str());
    dl->AddText(ImVec2(x, y), label, name.c_str());
}

void draw_weapon_icon(ImDrawList* dl, ImVec2 box_min, ImVec2 box_max, const std::string& icon_utf8, float alpha)
{
    if (!g_menu.vis_weapon_icon || icon_utf8.empty() || !font::weapon_icons || alpha <= 0.01f)
        return;

    const float icon_size = static_cast<float>(std::clamp(g_menu.vis_weapon_icon_size, 10, 36));
    const ImVec2 size = font::weapon_icons->CalcTextSizeA(icon_size, FLT_MAX, 0.0f, icon_utf8.c_str());

    const float x = (box_min.x + box_max.x - size.x) * 0.5f;
    float y_below = box_max.y + 3.f;
    if (g_menu.vis_distance)
        y_below += 14.f;

    const float y = (y_below + size.y <= ImGui::GetIO().DisplaySize.y) ? y_below : (box_min.y - size.y - 4.f);

    const ImVec4 col = from_arr(g_menu.weapon_icon_color);
    const ImU32 label = with_a(col, col.w * alpha);
    const ImU32 shadow = IM_COL32(2, 4, 6, static_cast<int>(180.f * alpha * col.w));

    dl->AddText(font::weapon_icons, icon_size, ImVec2(x + 1.f, y + 1.f), shadow, icon_utf8.c_str());
    dl->AddText(font::weapon_icons, icon_size, ImVec2(x, y), label, icon_utf8.c_str());
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
    const float max_dist_m = static_cast<float>(g_menu.vis_max_distance);

    const ImU32 team_name_col = ImGui::ColorConvertFloat4ToU32(from_arr(g_menu.box_team));

    for (const Player& p : game.players()) {
        const bool teammate = p.team == game.local_team();
        // Teammates draw with team colors when team names are on,
        // even with "Enemies only" active. Otherwise the old rule holds.
        if (teammate && !g_menu.vis_team_names && g_menu.vis_team_check)
            continue;
        const float distance_m = p.distance * 0.0254f;
        if (max_dist_m > 0.f && distance_m >= max_dist_m)
            continue;
        bool player_visible = true;
        if (vis.ready()) {
            const Vec3 from = game.local_head();
            const Vec3 head = p.has_joint(Skel::Head) ? p.joints[static_cast<int>(Skel::Head)] : p.eye;
            const Vec3 chest = p.has_joint(Skel::SpineUpper) ? p.joints[static_cast<int>(Skel::SpineUpper)]
                                                             : p.origin + Vec3{ 0.f, 0.f, p.ducked ? 32.f : 48.f };
            player_visible = vis.visible(from, head) || vis.visible(from, chest);
        }
        if (g_menu.vis_visible_only && !player_visible)
            continue;

        ImVec2 corners[8]{};
        ImVec2 mn, mx;
        int n = 0;
        if (!project_bounds(p, game.view_matrix(), screen_w, screen_h, mn, mx, corners, n))
            continue;

        mn.x -= 3.5f; mn.y -= 2.5f;
        mx.x += 3.5f; mx.y += 2.5f;

        const float dist_a = distance_alpha(distance_m, max_dist_m);
        const float move_a = 0.90f + 0.10f * std::clamp(p.speed / 180.f, 0.f, 1.f);
        const float pulse = 0.975f + 0.025f * std::sinf(time_s * 1.55f + p.origin.x * 0.02f);
        const float alpha = dist_a * move_a * pulse;
        const float hp = (p.max_health > 0) ? (p.health / static_cast<float>(p.max_health)) : 0.f;

        const ImVec4 col = from_arr(teammate ? g_menu.box_team : g_menu.box_enemy);
        const ImVec4 head_col = from_arr(teammate ? g_menu.head_team : g_menu.head_enemy);
        const ImVec4 skeleton_col = from_arr(player_visible ? g_menu.skeleton_visible : g_menu.skeleton_hidden);
        const BoxStyle style = static_cast<BoxStyle>(g_menu.vis_box_style);

        if (style == BoxStyle::Filled)
            draw_filled_box(dl, mn, mx, col, alpha);
        else if (style == BoxStyle::Box3D && n == 8)
            draw_3d_box(dl, corners, col, line_t, glow, alpha);
        else if (style == BoxStyle::Corner)
            draw_corner_box(dl, mn, mx, col, beam, glow, corner_pct, alpha);

        // Teammates: name in team color over the box, optional dimmed
        // distance below it. Same placement/size/gap as enemy layout.
        // Health, head marker, and weapon icon stay enemy-only to keep
        // team ESP quiet.
        if (!p.name.empty())
            draw_player_name(dl, mn, mx, p.name, alpha, teammate ? team_name_col : IM_COL32(235, 240, 246, 230));

        draw_skeleton(dl, p, game.view_matrix(), screen_w, screen_h, skeleton_col, alpha);

        if (!teammate && g_menu.vis_health)
            draw_health_bar(dl, mn, mx, hp, alpha, time_s);

        if ((!teammate && g_menu.vis_distance) || (teammate && g_menu.vis_team_distance))
            draw_distance_label(dl, mn, mx, distance_m, teammate ? alpha * 0.75f : alpha);

        if (!teammate && g_menu.vis_weapon_icon && !p.weapon_icon_utf8.empty())
            draw_weapon_icon(dl, mn, mx, p.weapon_icon_utf8, alpha);

        if (!teammate && g_menu.vis_head) {
            const Vec3 head_anchor = p.eye + (p.head - p.eye) * 0.45f;
            Vec2 head_screen{};
            if (world_to_screen(head_anchor, game.view_matrix(), screen_w, screen_h, head_screen)) {
                const float size_scale = std::clamp(g_menu.vis_head_size / 10.f, 0.5f, 2.f);
                const float radius = std::clamp((mx.x - mn.x) * 0.105f * size_scale, 2.5f, 24.f);
                if (head_screen.x >= -radius && head_screen.x <= screen_w + radius &&
                    head_screen.y >= -radius && head_screen.y <= screen_h + radius) {
                    draw_head_marker(dl, ImVec2(head_screen.x, head_screen.y), radius, head_col,
                                     static_cast<HeadMarkerStyle>(g_menu.vis_head_style), glow, alpha, time_s);
                }
            }
        }
    }
}

namespace {

void icon_fps_bars(ImDrawList* dl, ImVec2 p, ImU32 col)
{
    const float h[3] = { 5.f, 8.f, 11.f };
    for (int i = 0; i < 3; ++i) {
        const float x = p.x + i * 4.f;
        dl->AddRectFilled(ImVec2(x, p.y + 11.f - h[i]), ImVec2(x + 2.4f, p.y + 11.f), col, 0.8f);
    }
}

void icon_signal(ImDrawList* dl, ImVec2 p, ImU32 col)
{
    const float h[4] = { 4.f, 6.5f, 9.f, 11.5f };
    for (int i = 0; i < 4; ++i) {
        const float x = p.x + i * 3.4f;
        dl->AddRectFilled(ImVec2(x, p.y + 12.f - h[i]), ImVec2(x + 2.2f, p.y + 12.f), col, 0.7f);
    }
}

void icon_clock(ImDrawList* dl, ImVec2 c, ImU32 col)
{
    dl->AddCircle(c, 6.2f, col, 16, 1.15f);
    dl->AddLine(c, ImVec2(c.x, c.y - 3.4f), col, 1.15f);
    dl->AddLine(c, ImVec2(c.x + 2.6f, c.y + 1.4f), col, 1.15f);
}

void icon_pin(ImDrawList* dl, ImVec2 c, ImU32 col)
{
    dl->AddCircle(c, 4.4f, col, 12, 1.15f);
    dl->AddCircleFilled(c, 1.5f, col, 8);
    dl->AddLine(ImVec2(c.x, c.y + 4.2f), ImVec2(c.x, c.y + 8.6f), col, 1.2f);
}

const char* status_region()
{
    static char buf[64] = "Frankfurt am Main, DE";
    static bool init = false;
    if (init)
        return buf;
    init = true;

    wchar_t iso[8]{};
    const GEOID id = GetUserGeoID(GEOCLASS_NATION);
    GetGeoInfoW(id, GEO_ISO2, iso, 8, 0);

    DYNAMIC_TIME_ZONE_INFORMATION tz{};
    GetDynamicTimeZoneInformation(&tz);
    const wchar_t* key = tz.TimeZoneKeyName;

    struct Map { const wchar_t* tz; const char* label; };
    const Map maps[] = {
        { L"W. Europe Standard Time", "Frankfurt am Main, DE" },
        { L"Central Europe Standard Time", "Berlin, DE" },
        { L"GMT Standard Time", "London, GB" },
        { L"Romance Standard Time", "Paris, FR" },
        { L"Eastern Standard Time", "New York, US" },
        { L"Pacific Standard Time", "Los Angeles, US" },
        { L"Russian Standard Time", "Moscow, RU" },
        { L"Tokyo Standard Time", "Tokyo, JP" },
        { L"China Standard Time", "Shanghai, CN" },
    };
    for (const auto& m : maps) {
        if (key && !_wcsicmp(key, m.tz)) {
            std::snprintf(buf, sizeof(buf), "%s", m.label);
            return buf;
        }
    }
    if (iso[0] && iso[1]) {
        std::snprintf(buf, sizeof(buf), "%c%c", (char)iso[0], (char)iso[1]);
    }
    return buf;
}

const char* fallback_user()
{
    static char buf[64] = "player";
    static bool init = false;
    if (init)
        return buf;
    init = true;
    DWORD n = sizeof(buf);
    if (!GetUserNameA(buf, &n) || !buf[0])
        std::snprintf(buf, sizeof(buf), "player");
    return buf;
}

ImU32 ping_col(int ms)
{
    if (ms <= 0) return IM_COL32(168, 176, 186, 170);
    if (ms < 45) return IM_COL32(156, 204, 176, 210);
    if (ms < 80) return IM_COL32(210, 196, 150, 210);
    return IM_COL32(210, 160, 158, 210);
}

} // namespace

void draw_watermark(ImDrawList* dl, const Game& game, const VisCheck& /*vis*/, float /*screen_w*/)
{
    if (!g_menu.misc_watermark || !dl)
        return;

    const int fps = game.fps();
    const int ping = game.local_ping();
    const double t = ImGui::GetTime();
    static double t0 = -1.0;
    if (t0 < 0.0)
        t0 = t;
    const int elapsed = (int)(t - t0);
    const int hh = elapsed / 3600;
    const int mm = (elapsed / 60) % 60;
    const int ss = elapsed % 60;

    char fps_s[16], ping_s[16], time_s[16];
    if (fps > 0)
        std::snprintf(fps_s, sizeof(fps_s), "%d FPS", fps);
    else
        std::snprintf(fps_s, sizeof(fps_s), "-- FPS");
    if (ping > 0)
        std::snprintf(ping_s, sizeof(ping_s), "%d ms", ping);
    else
        std::snprintf(ping_s, sizeof(ping_s), "-- ms");
    if (hh > 0)
        std::snprintf(time_s, sizeof(time_s), "%d:%02d:%02d", hh, mm, ss);
    else
        std::snprintf(time_s, sizeof(time_s), "%02d:%02d", mm, ss);

    const char* region = status_region();
    const char* user = !game.local_name().empty() ? game.local_name().c_str() : fallback_user();

    const ImU32 text = IM_COL32(220, 228, 236, 205);
    const ImU32 mute = IM_COL32(168, 176, 186, 165);
    const ImU32 fps_c = fps >= 120 ? IM_COL32(156, 204, 176, 210)
                      : fps >= 60  ? text
                                   : IM_COL32(210, 196, 150, 210);
    const ImU32 ping_c = ping_col(ping);
    const ImU32 live_c = game.attached() ? IM_COL32(132, 196, 164, 230) : IM_COL32(196, 168, 120, 210);

    const ImVec2 fps_sz = ImGui::CalcTextSize(fps_s);
    const ImVec2 ping_sz = ImGui::CalcTextSize(ping_s);
    const ImVec2 time_sz = ImGui::CalcTextSize(time_s);
    const ImVec2 region_sz = ImGui::CalcTextSize(region);
    const ImVec2 user_sz = ImGui::CalcTextSize(user);

    const float icon_w = 16.f;
    const float gap = 8.f;
    const float sep = 14.f;
    const float pad_x = 14.f;
    const float h = 38.f;

    const float seg_fps = icon_w + gap + fps_sz.x;
    const float seg_ping = icon_w + gap + ping_sz.x;
    const float seg_time = icon_w + gap + time_sz.x;
    const float seg_reg = icon_w + gap + region_sz.x;
    const float seg_user = user_sz.x + 18.f;

    const float w = pad_x + seg_fps + sep + seg_ping + sep + seg_time + sep + seg_reg + sep + seg_user + pad_x;
    const ImVec2 a(16.f, 14.f);
    const ImVec2 b(a.x + w, a.y + h);

    dl->AddShadowRect(a, b, IM_COL32(0, 0, 0, 70), 16.f, ImVec2(0, 4), ImDrawFlags_ShadowCutOutShapeBackground, 11.f);
    dl->AddRectFilled(a, b, IM_COL32(10, 12, 16, 148), 11.f);
    dl->AddRect(a, b, IM_COL32(220, 230, 240, 26), 11.f, 0, 1.f);

    auto sep_at = [&](float x) {
        dl->AddLine(ImVec2(x, a.y + 11.f), ImVec2(x, b.y - 11.f), IM_COL32(220, 230, 240, 28), 1.f);
    };

    float x = a.x + pad_x;
    const float cy = a.y + h * 0.5f;
    auto text_y = [&](const ImVec2& sz) { return cy - sz.y * 0.5f; };

    icon_fps_bars(dl, ImVec2(x, cy - 6.f), fps_c);
    dl->AddText(ImVec2(x + icon_w + gap, text_y(fps_sz)), fps_c, fps_s);
    x += seg_fps + sep * 0.5f;
    sep_at(x);
    x += sep * 0.5f;

    icon_signal(dl, ImVec2(x, cy - 6.5f), ping_c);
    dl->AddText(ImVec2(x + icon_w + gap, text_y(ping_sz)), ping_c, ping_s);
    x += seg_ping + sep * 0.5f;
    sep_at(x);
    x += sep * 0.5f;

    icon_clock(dl, ImVec2(x + 7.f, cy), mute);
    dl->AddText(ImVec2(x + icon_w + gap, text_y(time_sz)), text, time_s);
    x += seg_time + sep * 0.5f;
    sep_at(x);
    x += sep * 0.5f;

    icon_pin(dl, ImVec2(x + 7.f, cy - 2.f), mute);
    dl->AddText(ImVec2(x + icon_w + gap, text_y(region_sz)), text, region);
    x += seg_reg + sep * 0.5f;
    sep_at(x);
    x += sep * 0.5f;

    dl->AddText(ImVec2(x, text_y(user_sz)), text, user);
    const ImVec2 dot(b.x - pad_x - 5.f, cy);
    dl->AddCircleFilled(dot, 6.6f, IM_COL32(255, 255, 255, 18), 16);
    dl->AddCircleFilled(dot, 4.2f, live_c, 16);
    dl->AddCircle(dot, 4.2f, IM_COL32(255, 255, 255, 35), 16, 1.f);
}
