#include "features/hitlog.hpp"
#include "features/panels.hpp"
#include "features/combat.hpp"
#include "features/weapon_icons.hpp"
#include "app/settings.hpp"
#include "sdk/game.hpp"
#include "sdk/skeleton.hpp"
#include "sdk/math.hpp"

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
#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

// Zones: 0 head, 1 chest, 2 body.
int bone_to_zone(int bone)
{
    if (bone == static_cast<int>(Skel::Head) || bone == static_cast<int>(Skel::Neck))
        return 0;
    if (bone == static_cast<int>(Skel::SpineUpper))
        return 1;
    return 2;
}

ImVec4 zone_color(int zone)
{
    if (zone == 0)
        return ImVec4(g_menu.hitlog_head[0], g_menu.hitlog_head[1], g_menu.hitlog_head[2], g_menu.hitlog_head[3]);
    if (zone == 1)
        return ImVec4(g_menu.hitlog_chest[0], g_menu.hitlog_chest[1], g_menu.hitlog_chest[2], g_menu.hitlog_chest[3]);
    return ImVec4(g_menu.hitlog_body[0], g_menu.hitlog_body[1], g_menu.hitlog_body[2], g_menu.hitlog_body[3]);
}

struct Track {
    int hp = 0;
    std::string name;
    ULONGLONG last_dmg = 0;
    int last_zone = 2;
};

struct Entry {
    std::string text;  // "Name  27" or just "Name" for kills
    int zone = 2;
    bool kill = false;
    std::string icon;  // weapon glyph for kill rows
    ULONGLONG t = 0;
};

std::unordered_map<uintptr_t, Track> g_hp;
std::vector<Entry> g_log;
ULONGLONG g_marker_until = 0;
bool g_marker_head = false;

// Attribution: health polling cannot see the shooter, so damage only counts
// as ours when the local player recently fired (shots window) or recently
// held the aim lock on the victim (lock window). Everything else — incoming,
// teammate crossfire, environment — stays out of the marker and the feed.
int g_last_shots = -1;
ULONGLONG g_last_shot_ms = 0;
uintptr_t g_lock_pawn = 0;
int g_lock_bone = -1;
ULONGLONG g_lock_ms = 0;

constexpr ULONGLONG kShotWindowMs = 600;
constexpr ULONGLONG kLockWindowMs = 800;
constexpr ULONGLONG kKillShotWindowMs = 900;
constexpr ULONGLONG kKillLockWindowMs = 1000;

void push_entry(Entry e)
{
    g_log.push_back(std::move(e));
    while (g_log.size() > 32)
        g_log.erase(g_log.begin());
}

} // namespace

void hitlog_tick(const Game& game)
{
    const ULONGLONG now = GetTickCount64();
    if (!game.attached()) {
        g_hp.clear();
        g_log.clear();
        g_marker_until = 0;
        g_last_shots = -1;
        g_last_shot_ms = 0;
        g_lock_pawn = 0;
        g_lock_bone = -1;
        g_lock_ms = 0;
        return;
    }

    const ULONGLONG life_ms = static_cast<ULONGLONG>((std::max)(0.5f, g_menu.hitlog_time) * 1000.f);
    while (!g_log.empty() && now - g_log.front().t > life_ms + 900)
        g_log.erase(g_log.begin());

    // Only attribute damage while the local player is up; otherwise just
    // re-baseline so spectator-time crossfire never lands in the feed.
    const bool track = game.local_alive();

    // Shot edge: m_iShotsFired rises on every local trigger pull.
    const int shots = game.shots_fired();
    if (g_last_shots >= 0 && shots > g_last_shots)
        g_last_shot_ms = now;
    g_last_shots = shots;

    // Lock memory: whoever we recently aimed at.
    const uintptr_t lock = combat_aim_pawn();
    if (lock) {
        g_lock_pawn = lock;
        g_lock_bone = combat_aim_bone();
        g_lock_ms = now;
    } else if (g_lock_pawn && now - g_lock_ms > kLockWindowMs) {
        g_lock_pawn = 0;
        g_lock_bone = -1;
    }

    for (const Player& p : game.players()) {
        if (!p.pawn || p.pawn == game.local_pawn())
            continue;
        const bool enemy = p.team != game.local_team();
        auto it = g_hp.find(p.pawn);
        if (it == g_hp.end()) {
            g_hp.emplace(p.pawn, Track{ p.health, p.name, 0, 2 });
            continue;
        }
        Track& tr = it->second;
        if (!p.name.empty())
            tr.name = p.name;
        // Victim must be an enemy AND the damage must fall inside our shot
        // or lock window. Incoming, team damage, and environment stay out.
        if (track && enemy && p.health < tr.hp &&
            (now - g_last_shot_ms < kShotWindowMs ||
             (p.pawn == g_lock_pawn && now - g_lock_ms < kLockWindowMs))) {
            const int dmg = tr.hp - p.health;
            const int zone = (p.pawn == g_lock_pawn) ? bone_to_zone(g_lock_bone) : 2;
            char buf[128]{};
            std::snprintf(buf, sizeof(buf), "%s  %d",
                          (p.name.empty() ? tr.name : p.name).c_str(), dmg);
            push_entry(Entry{ buf, zone, false, {}, now });
            tr.last_dmg = now;
            tr.last_zone = zone;
            if (g_menu.hit_enable) {
                g_marker_until = now + static_cast<ULONGLONG>((std::max)(60, g_menu.hit_time));
                g_marker_head = (zone == 0);
            }
        }
        tr.hp = p.health;
    }

    // Vanished shortly after taking damage: count as a kill, but only when
    // the kill itself is attributable to us (recent shot or recent lock on
    // that pawn). The killing blow already fired the hitmarker, so this
    // only adds the feed row.
    for (auto it = g_hp.begin(); it != g_hp.end();) {
        bool seen = false;
        for (const Player& p : game.players()) {
            if (p.pawn == it->first) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            const bool ours = (now - g_last_shot_ms < kKillShotWindowMs) ||
                (it->first == g_lock_pawn && now - g_lock_ms < kKillLockWindowMs);
            if (track && ours && it->second.last_dmg && now - it->second.last_dmg < 2000) {
                std::string icon;
                if (g_menu.hitlog_kill_icon)
                    icon = weapon_icons::resolve(std::string{}, static_cast<uint16_t>(game.weapon_def()));
                push_entry(Entry{ it->second.name, it->second.last_zone, true, icon, now });
            }
            it = g_hp.erase(it);
        } else {
            ++it;
        }
    }
}

void hitlog_draw(ImDrawList* dl, const Game& game, float screen_w, float screen_h)
{
    if (!dl || !game.attached())
        return;
    const ULONGLONG now = GetTickCount64();

    // Center hitmarker: X of four short lines, quick fade-in, soft fade-out.
    // Dark outline pass first so it reads on any background; headshots pop
    // with a short scale punch on top of their own color.
    if (g_menu.hit_enable && now < g_marker_until) {
        const int dur = (std::max)(60, g_menu.hit_time);
        const float age = static_cast<float>(dur) - static_cast<float>(g_marker_until - now);
        float a = (std::min)(age / 40.f, 1.f) * (1.f - age / static_cast<float>(dur));
        a *= clampf(g_menu.hit_alpha, 0.f, 1.f);
        if (a > 0.01f) {
            const float* c = g_marker_head ? g_menu.hit_head : g_menu.hit_normal;
            const ImU32 col = ImGui::ColorConvertFloat4ToU32(
                ImVec4(c[0], c[1], c[2], clampf(c[3] * a, 0.f, 1.f)));
            const ImU32 edge = IM_COL32(0, 0, 0, static_cast<int>(210.f * a));
            const float thick = static_cast<float>(std::clamp(g_menu.hit_thick, 1, 5));
            const float cx = screen_w * 0.5f;
            const float cy = screen_h * 0.5f;
            const float gap = 5.f;
            float len = gap + static_cast<float>(std::clamp(g_menu.hit_size, 4, 24));
            if (g_marker_head)
                len *= 1.f + 0.55f * std::exp(-age / 70.f);
            const ImVec2 p[4][2] = {
                { ImVec2(cx - gap, cy - gap), ImVec2(cx - len, cy - len) },
                { ImVec2(cx + gap, cy - gap), ImVec2(cx + len, cy - len) },
                { ImVec2(cx - gap, cy + gap), ImVec2(cx - len, cy + len) },
                { ImVec2(cx + gap, cy + gap), ImVec2(cx + len, cy + len) },
            };
            for (int i = 0; i < 4; ++i)
                dl->AddLine(p[i][0], p[i][1], edge, thick + 2.f);
            for (int i = 0; i < 4; ++i)
                dl->AddLine(p[i][0], p[i][1], col, thick);
        }
    }

    if (!g_menu.hitlog_enable || g_log.empty())
        return;

    const float life_ms = (std::max)(0.5f, g_menu.hitlog_time) * 1000.f;
    const int want = std::clamp(g_menu.hitlog_max, 1, 8);

    // Newest last; keep the newest `want` entries still inside their fade.
    std::vector<const Entry*> vis;
    for (int i = static_cast<int>(g_log.size()) - 1; i >= 0 && static_cast<int>(vis.size()) < want; --i) {
        const Entry& e = g_log[i];
        if (static_cast<float>(now - e.t) > life_ms + 800.f)
            continue;
        vis.push_back(&e);
    }
    if (vis.empty())
        return;
    std::reverse(vis.begin(), vis.end());

    // Measure for a hugging panel.
    const float icon_px = 16.f;
    float text_w = 0.f;
    for (const Entry* e : vis) {
        float w = ImGui::CalcTextSize(e->text.c_str()).x;
        if (e->kill && !e->icon.empty() && font::weapon_icons)
            w += font::weapon_icons->CalcTextSizeA(icon_px, FLT_MAX, 0.f, e->icon.c_str()).x + 6.f;
        text_w = (std::max)(text_w, w);
    }

    const float pad = 12.f;
    const float row_h = 20.f;
    const float panel_w = pad * 2.f + 6.f + 6.f + text_w;
    const float panel_h = pad * 2.f + static_cast<float>(vis.size()) * row_h - 4.f;

    const ImVec2 a = panel_anchor(g_menu.hitlog_anchor, panel_w, panel_h, screen_w, screen_h,
                                  static_cast<float>(g_menu.hitlog_off_x),
                                  static_cast<float>(g_menu.hitlog_off_y));
    const ImVec2 b(a.x + panel_w, a.y + panel_h);

    dl->AddShadowRect(a, b, IM_COL32(0, 0, 0, 70), 16.f, ImVec2(0, 4),
                      ImDrawFlags_ShadowCutOutShapeBackground, 11.f);
    dl->AddRectFilled(a, b, IM_COL32(10, 12, 16, 150), 11.f);
    dl->AddRect(a, b, IM_COL32(220, 230, 240, 26), 11.f, 0, 1.f);

    for (size_t i = 0; i < vis.size(); ++i) {
        const Entry* e = vis[i];
        const float age = static_cast<float>(now - e->t);
        const float slide = (1.f - (std::min)(age / 150.f, 1.f)) * 10.f;
        const float fade = 1.f - clampf((age - life_ms) / 800.f, 0.f, 1.f);
        const float a_in = (std::min)(age / 100.f, 1.f);
        const float alpha = a_in * fade;
        if (alpha <= 0.01f)
            continue;

        const float cy = a.y + pad + i * row_h + row_h * 0.5f + slide;
        const ImVec4 zc = zone_color(e->zone);
        dl->AddCircleFilled(ImVec2(a.x + pad + 3.f, cy), 3.f,
            ImGui::ColorConvertFloat4ToU32(ImVec4(zc.x, zc.y, zc.z, zc.w * alpha)), 12);

        float tx = a.x + pad + 6.f + 6.f;
        if (e->kill && !e->icon.empty() && font::weapon_icons) {
            const ImVec2 isz = font::weapon_icons->CalcTextSizeA(icon_px, FLT_MAX, 0.f, e->icon.c_str());
            dl->AddText(font::weapon_icons, icon_px,
                        ImVec2(tx + 1.f, cy - isz.y * 0.5f + 1.f),
                        IM_COL32(2, 4, 6, static_cast<int>(180.f * alpha)), e->icon.c_str());
            dl->AddText(font::weapon_icons, icon_px, ImVec2(tx, cy - isz.y * 0.5f),
                        ImGui::ColorConvertFloat4ToU32(ImVec4(0.95f, 0.95f, 0.98f, 0.95f * alpha)),
                        e->icon.c_str());
            tx += isz.x + 6.f;
        }
        const ImVec2 tsz = ImGui::CalcTextSize(e->text.c_str());
        dl->AddText(ImVec2(tx + 1.f, cy - tsz.y * 0.5f + 1.f),
                    IM_COL32(3, 5, 8, static_cast<int>(175.f * alpha)), e->text.c_str());
        dl->AddText(ImVec2(tx, cy - tsz.y * 0.5f),
                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.90f, 0.93f, 0.96f, alpha)),
                    e->text.c_str());
    }
}
