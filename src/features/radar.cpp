#include "features/radar.hpp"
#include "features/panels.hpp"
#include "sdk/game.hpp"
#include "sdk/math.hpp"

#include "imgui.h"

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
#include <cstring>
#include <string>
#include <unordered_map>

RadarSettings g_radar;

namespace {

constexpr float kUnitsToM = 0.0254f;
constexpr float kPi = 3.14159265358979323846f;

// ---- separate Persistenz (radar.json neben der EXE) ----

std::string radar_exe_dir()
{
    char buf[MAX_PATH]{};
    const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (!n)
        return {};
    std::string p(buf, buf + n);
    const auto slash = p.find_last_of("\\/");
    return slash == std::string::npos ? std::string{} : p.substr(0, slash);
}

std::string radar_path()
{
    const std::string d = radar_exe_dir();
    return d.empty() ? std::string("radar.json") : d + "\\radar.json";
}

bool j_bool(const std::string& j, const char* k, bool& d)
{
    const std::string n = std::string("\"") + k + "\"";
    const size_t f = j.find(n);
    if (f == std::string::npos)
        return false;
    const size_t c = j.find(':', f + n.size());
    if (c == std::string::npos)
        return false;
    size_t p = c + 1;
    while (p < j.size() && static_cast<unsigned char>(j[p]) <= 32)
        ++p;
    if (j.compare(p, 4, "true") == 0) { d = true; return true; }
    if (j.compare(p, 5, "false") == 0) { d = false; return true; }
    return false;
}

bool j_num(const std::string& j, const char* k, double& d)
{
    const std::string n = std::string("\"") + k + "\"";
    const size_t f = j.find(n);
    if (f == std::string::npos)
        return false;
    const size_t c = j.find(':', f + n.size());
    if (c == std::string::npos)
        return false;
    const char* p = j.c_str() + c + 1;
    char* end = nullptr;
    const double v = std::strtod(p, &end);
    if (!end || end == p)
        return false;
    d = v;
    return true;
}

bool j_int(const std::string& j, const char* k, int& d, int lo, int hi)
{
    double v = 0;
    if (!j_num(j, k, v))
        return false;
    d = std::clamp(static_cast<int>(std::lround(v)), lo, hi);
    return true;
}

bool j_float(const std::string& j, const char* k, float& d, float lo, float hi)
{
    double v = 0;
    if (!j_num(j, k, v))
        return false;
    d = std::clamp(static_cast<float>(v), lo, hi);
    return true;
}

bool j_vec4(const std::string& j, const char* k, float d[4])
{
    const std::string n = std::string("\"") + k + "\"";
    const size_t f = j.find(n);
    if (f == std::string::npos)
        return false;
    const size_t b = j.find('[', f + n.size());
    if (b == std::string::npos)
        return false;
    const char* p = j.c_str() + b + 1;
    for (int i = 0; i < 4; ++i) {
        char* end = nullptr;
        const double v = std::strtod(p, &end);
        if (!end || end == p)
            return false;
        d[i] = std::clamp(static_cast<float>(v), 0.f, 1.f);
        p = end;
        if (i < 3) {
            while (*p && *p != ',')
                ++p;
            if (*p == ',')
                ++p;
        }
    }
    return true;
}

bool g_dirty = false;
ULONGLONG g_dirty_ms = 0;

// ---- weiche Blip-Bewegung ----

struct Blip {
    ImVec2 pos{};
    ULONGLONG last = 0;
    bool init = false;
};
std::unordered_map<uintptr_t, Blip> g_blips;

ImU32 scaled(const float c[4], float mul)
{
    return ImGui::ColorConvertFloat4ToU32(ImVec4(
        c[0], c[1], c[2], std::clamp(c[3] * mul, 0.f, 1.f)));
}

} // namespace

void radar_mark_dirty()
{
    g_dirty = true;
    g_dirty_ms = GetTickCount64();
}

void radar_save_now()
{
    char b[1024]{};
    std::snprintf(b, sizeof(b),
        "{\n"
        "  \"version\": 1,\n"
        "  \"enable\": %s,\n"
        "  \"anchor\": %d,\n"
        "  \"off_x\": %d,\n"
        "  \"off_y\": %d,\n"
        "  \"size\": %d,\n"
        "  \"opacity\": %.3f,\n"
        "  \"range\": %.1f,\n"
        "  \"rotate\": %s,\n"
        "  \"filter\": %d,\n"
        "  \"enemy\": [%.4f, %.4f, %.4f, %.4f],\n"
        "  \"team\": [%.4f, %.4f, %.4f, %.4f],\n"
        "  \"local\": [%.4f, %.4f, %.4f, %.4f],\n"
        "  \"ring\": [%.4f, %.4f, %.4f, %.4f]\n"
        "}\n",
        g_radar.enable ? "true" : "false",
        g_radar.anchor, g_radar.off_x, g_radar.off_y, g_radar.size,
        g_radar.opacity, g_radar.range,
        g_radar.rotate ? "true" : "false", g_radar.filter,
        g_radar.enemy[0], g_radar.enemy[1], g_radar.enemy[2], g_radar.enemy[3],
        g_radar.team[0], g_radar.team[1], g_radar.team[2], g_radar.team[3],
        g_radar.local[0], g_radar.local[1], g_radar.local[2], g_radar.local[3],
        g_radar.ring[0], g_radar.ring[1], g_radar.ring[2], g_radar.ring[3]);

    FILE* fp = nullptr;
    fopen_s(&fp, radar_path().c_str(), "w");
    if (!fp)
        return;
    std::fputs(b, fp);
    std::fclose(fp);
    g_dirty = false;
}

void radar_startup()
{
    FILE* fp = nullptr;
    fopen_s(&fp, radar_path().c_str(), "r");
    if (!fp)
        return;
    std::string j;
    char chunk[1024];
    size_t n = 0;
    while ((n = std::fread(chunk, 1, sizeof(chunk), fp)) > 0)
        j.append(chunk, n);
    std::fclose(fp);
    if (j.size() < 8)
        return;

    j_bool(j, "enable", g_radar.enable);
    j_int(j, "anchor", g_radar.anchor, 0, 3);
    j_int(j, "off_x", g_radar.off_x, -600, 600);
    j_int(j, "off_y", g_radar.off_y, -600, 600);
    j_int(j, "size", g_radar.size, 140, 320);
    j_float(j, "opacity", g_radar.opacity, 0.30f, 1.f);
    j_float(j, "range", g_radar.range, 10.f, 60.f);
    j_bool(j, "rotate", g_radar.rotate);
    j_int(j, "filter", g_radar.filter, 0, 2);
    j_vec4(j, "enemy", g_radar.enemy);
    j_vec4(j, "team", g_radar.team);
    j_vec4(j, "local", g_radar.local);
    j_vec4(j, "ring", g_radar.ring);
}

void radar_housekeep(bool menu_open)
{
    // Schreiben nur bei geschlossenem Menue + Ruhe, damit weder Overlay
    // noch Spiel-Input je blockiert.
    if (g_dirty && !menu_open && GetTickCount64() - g_dirty_ms > 1200)
        radar_save_now();
}

void radar_draw(ImDrawList* dl, const Game& game, float screen_w, float screen_h)
{
    const ULONGLONG now = GetTickCount64();
    if (!g_radar.enable || !dl || !game.attached()) {
        if (!game.attached())
            g_blips.clear();
        return;
    }

    const float o = std::clamp(g_radar.opacity, 0.30f, 1.f);
    const float S = static_cast<float>(std::clamp(g_radar.size, 140, 320));
    const float header_h = 30.f;
    const float W = S, H = S + header_h;

    const ImVec2 a = panel_anchor(g_radar.anchor, W, H, screen_w, screen_h,
                                  static_cast<float>(g_radar.off_x),
                                  static_cast<float>(g_radar.off_y));
    const ImVec2 b(a.x + W, a.y + H);

    dl->AddShadowRect(a, b, IM_COL32(0, 0, 0, static_cast<int>(70 * o)), 16.f,
                      ImVec2(0, 4), ImDrawFlags_ShadowCutOutShapeBackground, 11.f);
    dl->AddRectFilled(a, b, IM_COL32(10, 12, 16, static_cast<int>(150 * o)), 11.f);
    dl->AddRect(a, b, IM_COL32(220, 230, 240, static_cast<int>(26 * o)), 11.f, 0, 1.f);

    // Kopfzeile: Punkt-Glyphe + Titel links, Reichweite rechts.
    const float hcy = a.y + header_h * 0.5f;
    const ImVec2 dot(a.x + 18.f, hcy);
    dl->AddCircle(dot, 6.f, IM_COL32(132, 196, 164, static_cast<int>(230 * o)), 16, 1.6f);
    dl->AddCircleFilled(dot, 2.2f, IM_COL32(132, 196, 164, static_cast<int>(230 * o)), 12);
    dl->AddText(ImVec2(dot.x + 11.f, hcy - ImGui::CalcTextSize("Radar").y * 0.5f),
                IM_COL32(220, 228, 236, static_cast<int>(205 * o)), "Radar");
    char range_s[16]{};
    std::snprintf(range_s, sizeof(range_s), "%dm", static_cast<int>(std::lround(g_radar.range)));
    const ImVec2 rsz = ImGui::CalcTextSize(range_s);
    dl->AddText(ImVec2(b.x - 14.f - rsz.x, hcy - rsz.y * 0.5f),
                IM_COL32(150, 158, 168, static_cast<int>(170 * o)), range_s);
    const float div_y = a.y + header_h;
    dl->AddLine(ImVec2(a.x + 12.f, div_y), ImVec2(b.x - 12.f, div_y),
                IM_COL32(220, 230, 240, static_cast<int>(24 * o)), 1.f);

    // Scope.
    const ImVec2 c(a.x + W * 0.5f, div_y + (H - header_h) * 0.5f);
    const float r = S * 0.5f - 14.f;
    const float range_m = (std::max)(5.f, g_radar.range);

    dl->AddCircleFilled(c, r, IM_COL32(13, 16, 21, static_cast<int>(110 * o)), 48);
    dl->AddCircle(c, r * 0.5f, scaled(g_radar.ring, 0.16f * o), 40, 1.f);
    dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x + r, c.y), scaled(g_radar.ring, 0.07f * o), 1.f);
    dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y + r), scaled(g_radar.ring, 0.07f * o), 1.f);
    dl->AddCircle(c, r, scaled(g_radar.ring, 0.32f * o), 48, 1.25f);

    const Vec3& org = game.local_origin();
    const float yaw = game.view_angles().y * kPi / 180.f;
    const float sy = std::sin(yaw), cy = std::cos(yaw);
    const float dt = std::clamp(ImGui::GetIO().DeltaTime, 0.001f, 0.05f);
    const float k = 1.f - std::exp(-dt * 12.f); // weich, kein Ruckeln

    for (const Player& p : game.players()) {
        if (!p.pawn || p.pawn == game.local_pawn())
            continue;
        if (p.health <= 0 || p.team < 2 || p.team > 3)
            continue;
        const bool mate = p.team == game.local_team();
        if (g_radar.filter == 1 && mate)
            continue;
        if (g_radar.filter == 2 && !mate)
            continue;

        const float dx = (p.origin.x - org.x) * kUnitsToM;
        const float dy = (p.origin.y - org.y) * kUnitsToM;
        float sx, sz;
        if (g_radar.rotate) {
            // Heading-up: Blickrichtung zeigt nach oben.
            sx = dx * sy - dy * cy;
            sz = -(dx * cy + dy * cy);
        } else {
            sx = dx;
            sz = -dy;
        }
        float px = sx / range_m * r;
        float py = sz / range_m * r;
        const float len = std::sqrt(px * px + py * py);
        const float max_r = r - 5.f;
        bool rim = false;
        if (len > max_r && len > 0.001f) {
            px = px / len * max_r;
            py = py / len * max_r;
            rim = true;
        }
        const ImVec2 target(c.x + px, c.y + py);

        Blip& bl = g_blips[p.pawn];
        if (!bl.init) {
            bl.pos = target;
            bl.init = true;
        } else {
            bl.pos.x += (target.x - bl.pos.x) * k;
            bl.pos.y += (target.y - bl.pos.y) * k;
        }
        bl.last = now;

        float alpha = o * (rim ? 0.45f : 1.f);
        const float* col = mate ? g_radar.team : g_radar.enemy;
        const float rad = mate ? 3.f : 3.5f;
        dl->AddShadowCircle(bl.pos, rad, scaled(col, 0.25f * alpha), 6.f, ImVec2(0, 0), 0, 12);
        dl->AddCircleFilled(bl.pos, rad + 1.f, IM_COL32(2, 3, 5, static_cast<int>(200 * alpha)), 14);
        dl->AddCircleFilled(bl.pos, rad, scaled(col, alpha), 14);
    }

    // Eigener Pfeil in der Mitte (Blickrichtung).
    {
        float fx = 0.f, fy = -1.f;
        if (!g_radar.rotate) {
            fx = cy;
            fy = -sy;
            const float l = std::sqrt(fx * fx + fy * fy);
            if (l > 0.001f) { fx /= l; fy /= l; }
        }
        const float px = -fy, py = fx;
        const ImVec2 tip(c.x + fx * 7.5f, c.y + fy * 7.5f);
        const ImVec2 l(c.x - fx * 5.f + px * 4.5f, c.y - fy * 5.f + py * 4.5f);
        const ImVec2 rr(c.x - fx * 5.f - px * 4.5f, c.y - fy * 5.f - py * 4.5f);
        dl->AddTriangleFilled(tip, l, rr, scaled(g_radar.local, o));
        dl->AddTriangle(tip, l, rr, IM_COL32(2, 3, 5, static_cast<int>(200 * o)), 1.2f);
    }

    // Alte Blips ohne Update entsorgen (kein Flackern bei Respawn/Disconnect).
    for (auto it = g_blips.begin(); it != g_blips.end();) {
        bool seen = false;
        for (const Player& p : game.players()) {
            if (p.pawn == it->first) { seen = true; break; }
        }
        if (!seen || now - it->second.last > 3000)
            it = g_blips.erase(it);
        else
            ++it;
    }
}
