#include "app/config.hpp"
#include "app/settings.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_settings.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <commdlg.h>

#pragma comment(lib, "comdlg32.lib")

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <sstream>

namespace {

std::string exe_dir()
{
    char buf[MAX_PATH]{};
    const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (!n)
        return {};
    std::string p(buf, buf + n);
    const auto slash = p.find_last_of("\\/");
    return slash == std::string::npos ? p : p.substr(0, slash);
}

bool dir_ok(const std::string& path)
{
    const DWORD a = GetFileAttributesA(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

void skip_ws(const char*& p, const char* e)
{
    while (p < e && static_cast<unsigned char>(*p) <= 32)
        ++p;
}

bool find_key(const std::string& json, const char* key, const char*& p, const char* e)
{
    const std::string needle = std::string("\"") + key + "\"";
    size_t pos = 0;
    while ((pos = json.find(needle, pos)) != std::string::npos) {
        if (pos > 0) {
            const unsigned char before = static_cast<unsigned char>(json[pos - 1]);
            if (before > 32 && before != '{' && before != ',') {
                pos += 1;
                continue;
            }
        }
        p = json.c_str() + pos + needle.size();
        e = json.c_str() + json.size();
        skip_ws(p, e);
        if (p < e && *p == ':') {
            ++p;
            skip_ws(p, e);
            return p < e;
        }
        pos += 1;
    }
    return false;
}

bool read_bool(const std::string& json, const char* key, bool& dest)
{
    const char* p = nullptr;
    const char* e = nullptr;
    if (!find_key(json, key, p, e))
        return false;
    if (e - p >= 4 && std::strncmp(p, "true", 4) == 0) {
        dest = true;
        return true;
    }
    if (e - p >= 5 && std::strncmp(p, "false", 5) == 0) {
        dest = false;
        return true;
    }
    return false;
}

bool read_int(const std::string& json, const char* key, int& dest, int lo, int hi)
{
    const char* p = nullptr;
    const char* e = nullptr;
    if (!find_key(json, key, p, e))
        return false;
    char* end = nullptr;
    const long v = std::strtol(p, &end, 10);
    if (!end || end == p)
        return false;
    dest = static_cast<int>(v);
    if (dest < lo) dest = lo;
    if (dest > hi) dest = hi;
    return true;
}

bool read_float(const std::string& json, const char* key, float& dest, float lo, float hi)
{
    const char* p = nullptr;
    const char* e = nullptr;
    if (!find_key(json, key, p, e))
        return false;
    char* end = nullptr;
    const float v = std::strtof(p, &end);
    if (!end || end == p)
        return false;
    dest = std::clamp(v, lo, hi);
    return true;
}

bool read_vec4(const std::string& json, const char* key, float dest[4])
{
    const char* p = nullptr;
    const char* e = nullptr;
    if (!find_key(json, key, p, e) || *p != '[')
        return false;
    ++p;
    for (int i = 0; i < 4; ++i) {
        skip_ws(p, e);
        char* end = nullptr;
        const float v = std::strtof(p, &end);
        if (!end || end == p)
            return false;
        dest[i] = std::clamp(v, 0.f, 1.f);
        p = end;
        skip_ws(p, e);
        if (i < 3) {
            if (p >= e || *p != ',')
                return false;
            ++p;
        }
    }
    return true;
}

void json_bool(std::ostringstream& o, const char* k, bool v, bool comma = true)
{
    o << "  \"" << k << "\": " << (v ? "true" : "false") << (comma ? ",\n" : "\n");
}

void json_int(std::ostringstream& o, const char* k, int v, bool comma = true)
{
    o << "  \"" << k << "\": " << v << (comma ? ",\n" : "\n");
}

void json_float(std::ostringstream& o, const char* k, float v, bool comma = true)
{
    char buf[96]{};
    std::snprintf(buf, sizeof(buf), "  \"%s\": %.3f%s\n", k, v, comma ? "," : "");
    o << buf;
}

void json_vec4(std::ostringstream& o, const char* k, const float v[4], bool comma = true)
{
    char buf[96]{};
    std::snprintf(buf, sizeof(buf), "  \"%s\": [%.4f, %.4f, %.4f, %.4f]%s\n",
                  k, v[0], v[1], v[2], v[3], comma ? "," : "");
    o << buf;
}

void apply_accent(const float c[4])
{
    g_menu.accent[0] = c[0];
    g_menu.accent[1] = c[1];
    g_menu.accent[2] = c[2];
    g_menu.accent[3] = c[3];
    c::main_color = ImColor(c[0], c[1], c[2], c[3]);
}

std::string stem_of(const std::string& path)
{
    size_t s = path.find_last_of("\\/");
    size_t e = path.find_last_of('.');
    const size_t b = (s == std::string::npos) ? 0 : s + 1;
    if (e == std::string::npos || e < b)
        return path.substr(b);
    return path.substr(b, e - b);
}

std::string utf8_of(const std::wstring& w)
{
    if (w.empty())
        return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    if (n <= 0)
        return {};
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

std::wstring wide_of(const std::string& s)
{
    if (s.empty())
        return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (n <= 0)
        return {};
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

} // namespace

std::string sanitize_config_name(const std::string& raw)
{
    std::string s;
    s.reserve(raw.size());
    for (unsigned char c : raw) {
        if (c <= 32) {
            if (!s.empty() && s.back() != '_')
                s.push_back('_');
            continue;
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
            s.push_back(static_cast<char>(c));
    }
    while (!s.empty() && s.back() == '_')
        s.pop_back();
    if (s.size() > 48)
        s.resize(48);
    if (s.empty() || s == "." || s == "..")
        return {};
    return s;
}

ConfigStore& ConfigStore::instance()
{
    static ConfigStore g;
    return g;
}

void ConfigStore::log(const char* fmt, ...)
{
    char line[512]{};
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    status_ = line;

    OutputDebugStringA("[config] ");
    OutputDebugStringA(line);
    OutputDebugStringA("\n");
    std::printf("[config] %s\n", line);

    const std::string exe = exe_dir();
    const std::string logs[] = {
        dir_ + "\\config.log",
        exe + "\\config.log",
        "D:\\CS2\\configs\\config.log",
        "D:\\CS2\\release\\config.log",
    };
    SYSTEMTIME st{};
    GetLocalTime(&st);
    for (const auto& path : logs) {
        if (path.size() < 12)
            continue;
        FILE* fp = nullptr;
        fopen_s(&fp, path.c_str(), "a");
        if (!fp)
            continue;
        std::fprintf(fp, "%04d-%02d-%02d %02d:%02d:%02d  %s\n",
                     st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, line);
        std::fclose(fp);
        break;
    }
}

void ConfigStore::ensure_dir()
{
    const std::string exe = exe_dir();
    const std::string cands[] = {
        exe + "\\configs",
        "D:\\CS2\\release\\configs",
        "D:\\CS2\\configs",
        exe + "\\..\\configs",
        "configs",
    };
    for (const auto& c : cands) {
        if (c.empty())
            continue;
        CreateDirectoryA(c.c_str(), nullptr);
        if (dir_ok(c)) {
            dir_ = c;
            return;
        }
    }
    dir_ = exe.empty() ? std::string("D:\\CS2\\release\\configs") : exe + "\\configs";
    CreateDirectoryA(dir_.c_str(), nullptr);
}

std::string ConfigStore::path_for(const std::string& name) const
{
    return dir_ + "\\" + name + ".json";
}

// Zentrales JSON-Build: eine Stelle für save() und save_to_file().
// Alle Toggles (Team-Check, Visible-Check, Head, Skeleton, ...) werden
// explizit geschrieben, damit Laden den Zustand exakt wiederherstellt.
static std::string build_config_json(const std::string& clean)
{
    float accent[4] = {
        c::main_color.Value.x, c::main_color.Value.y,
        c::main_color.Value.z, c::main_color.Value.w
    };

    std::ostringstream o;
    o << "{\n";
    o << "  \"version\": 2,\n";
    o << "  \"name\": \"" << clean << "\",\n";
    json_bool(o, "vis_enable", g_menu.vis_enable);
    json_int(o, "vis_box_style", g_menu.vis_box_style);
    json_bool(o, "vis_health", g_menu.vis_health);
    json_int(o, "vis_health_position", g_menu.vis_health_position);
    json_int(o, "vis_health_width", g_menu.vis_health_width);
    json_bool(o, "vis_health_gradient", g_menu.vis_health_gradient);
    json_bool(o, "vis_health_value", g_menu.vis_health_value);
    json_bool(o, "vis_head", g_menu.vis_head);
    json_int(o, "vis_head_style", g_menu.vis_head_style);
    json_int(o, "vis_head_size", g_menu.vis_head_size);
    json_bool(o, "vis_skeleton", g_menu.vis_skeleton);
    json_int(o, "vis_skeleton_mode", g_menu.vis_skeleton_mode);
    json_int(o, "vis_skeleton_style", g_menu.vis_skeleton_style);
    json_int(o, "vis_skeleton_thickness", g_menu.vis_skeleton_thickness);
    json_bool(o, "vis_distance", g_menu.vis_distance);
    json_bool(o, "vis_weapon_icon", g_menu.vis_weapon_icon);
    json_int(o, "vis_weapon_icon_size", g_menu.vis_weapon_icon_size);
    json_bool(o, "vis_team_check", g_menu.vis_team_check);
    json_bool(o, "vis_visible_only", g_menu.vis_visible_only);
    json_int(o, "vis_thickness", g_menu.vis_thickness);
    json_int(o, "vis_glow", g_menu.vis_glow);
    json_int(o, "vis_corner", g_menu.vis_corner);
    json_int(o, "vis_max_distance", g_menu.vis_max_distance);
    json_bool(o, "misc_watermark", g_menu.misc_watermark);
    json_bool(o, "misc_hotkeys", g_menu.misc_hotkeys);
    json_bool(o, "vis_team_names", g_menu.vis_team_names);
    json_bool(o, "vis_team_distance", g_menu.vis_team_distance);
    json_bool(o, "spec_enable", g_menu.spec_enable);
    json_int(o, "spec_anchor", g_menu.spec_anchor);
    json_int(o, "spec_off_x", g_menu.spec_off_x);
    json_int(o, "spec_off_y", g_menu.spec_off_y);
    json_bool(o, "hitlog_enable", g_menu.hitlog_enable);
    json_int(o, "hitlog_max", g_menu.hitlog_max);
    json_float(o, "hitlog_time", g_menu.hitlog_time);
    json_int(o, "hitlog_anchor", g_menu.hitlog_anchor);
    json_int(o, "hitlog_off_x", g_menu.hitlog_off_x);
    json_int(o, "hitlog_off_y", g_menu.hitlog_off_y);
    json_bool(o, "hitlog_kill_icon", g_menu.hitlog_kill_icon);
    json_vec4(o, "hitlog_head", g_menu.hitlog_head);
    json_vec4(o, "hitlog_chest", g_menu.hitlog_chest);
    json_vec4(o, "hitlog_body", g_menu.hitlog_body);
    json_bool(o, "aim_enable", g_menu.aim_enable);
    json_bool(o, "aim_visible", g_menu.aim_visible);
    json_bool(o, "aim_recoil", g_menu.aim_recoil);
    json_bool(o, "aim_fov_draw", g_menu.aim_fov_draw);
    json_bool(o, "aim_humanize", g_menu.aim_humanize);
    json_bool(o, "aim_team_check", g_menu.aim_team_check);
    json_int(o, "aim_key", g_menu.aim_key);
    json_int(o, "aim_key_mode", g_menu.aim_key_mode);
    json_int(o, "aim_edit_profile", g_menu.aim_edit_profile);
    json_float(o, "rifle_fov", g_menu.aim_rifle.fov);
    json_float(o, "rifle_smooth", g_menu.aim_rifle.smooth);
    json_int(o, "rifle_bone", g_menu.aim_rifle.bone);
    json_float(o, "rifle_rcs_yaw", g_menu.aim_rifle.rcs_yaw);
    json_float(o, "rifle_rcs_pitch", g_menu.aim_rifle.rcs_pitch);
    json_float(o, "pistol_fov", g_menu.aim_pistol.fov);
    json_float(o, "pistol_smooth", g_menu.aim_pistol.smooth);
    json_int(o, "pistol_bone", g_menu.aim_pistol.bone);
    json_float(o, "pistol_rcs_yaw", g_menu.aim_pistol.rcs_yaw);
    json_float(o, "pistol_rcs_pitch", g_menu.aim_pistol.rcs_pitch);
    json_float(o, "sniper_fov", g_menu.aim_sniper.fov);
    json_float(o, "sniper_smooth", g_menu.aim_sniper.smooth);
    json_int(o, "sniper_bone", g_menu.aim_sniper.bone);
    json_float(o, "sniper_rcs_yaw", g_menu.aim_sniper.rcs_yaw);
    json_float(o, "sniper_rcs_pitch", g_menu.aim_sniper.rcs_pitch);
    json_float(o, "aim_reaction_min", g_menu.aim_reaction_min);
    json_float(o, "aim_reaction_max", g_menu.aim_reaction_max);
    json_float(o, "aim_noise", g_menu.aim_noise);
    json_float(o, "aim_overshoot", g_menu.aim_overshoot);
    json_float(o, "aim_miss", g_menu.aim_miss);
    json_bool(o, "aim_debug", g_menu.aim_debug);
    json_bool(o, "trigger_enable", g_menu.trigger_enable);
    json_bool(o, "trigger_visible", g_menu.trigger_visible);
    json_bool(o, "trigger_team_check", g_menu.trigger_team_check);
    json_bool(o, "trigger_scope", g_menu.trigger_scope);
    json_bool(o, "trigger_flash", g_menu.trigger_flash);
    json_bool(o, "trigger_weapon_filter", g_menu.trigger_weapon_filter);
    json_int(o, "trigger_hitbox", g_menu.trigger_hitbox);
    json_int(o, "trigger_first_ms", g_menu.trigger_first_ms);
    json_int(o, "trigger_next_ms", g_menu.trigger_next_ms);
    json_int(o, "trigger_jitter_ms", g_menu.trigger_jitter_ms);
    json_int(o, "trigger_key", g_menu.trigger_key);
    json_int(o, "trigger_key_mode", g_menu.trigger_key_mode);
    json_int(o, "menu_key", g_menu.menu_key);
    json_int(o, "menu_key_mode", g_menu.menu_key_mode);
    json_vec4(o, "accent", accent);
    json_vec4(o, "box_enemy", g_menu.box_enemy);
    json_vec4(o, "box_team", g_menu.box_team);
    json_vec4(o, "health_low", g_menu.health_low);
    json_vec4(o, "health_high", g_menu.health_high);
    json_vec4(o, "head_enemy", g_menu.head_enemy);
    json_vec4(o, "head_team", g_menu.head_team);
    json_vec4(o, "skeleton_visible", g_menu.skeleton_visible);
    json_vec4(o, "skeleton_hidden", g_menu.skeleton_hidden);
    json_vec4(o, "weapon_icon_color", g_menu.weapon_icon_color, false);
    o << "}\n";
    return o.str();
}

// Zentrales JSON-Parse: eine Stelle für load() und load_from_file().
// Jeder Toggle wird einzeln gelesen; fehlende Keys behalten Defaults
// (Abwärtskompatibilität mit alten Configs). Unbekannte/Alt-Keys
// (z.B. entfernter Hitmarker) werden still ignoriert.
static bool parse_config_json(const std::string& json, MenuState& next, int& got)
{
    got = 0;
    auto b = [&](const char* k, bool& d) { if (read_bool(json, k, d)) ++got; };
    auto i = [&](const char* k, int& d, int lo, int hi) { if (read_int(json, k, d, lo, hi)) ++got; };
    auto f = [&](const char* k, float& d, float lo, float hi) { if (read_float(json, k, d, lo, hi)) ++got; };
    auto c4 = [&](const char* k, float* d) { if (read_vec4(json, k, d)) ++got; };

    b("vis_enable", next.vis_enable);
    i("vis_box_style", next.vis_box_style, 0, 2);
    b("vis_health", next.vis_health);
    i("vis_health_position", next.vis_health_position, 0, 1);
    i("vis_health_width", next.vis_health_width, 2, 8);
    b("vis_health_gradient", next.vis_health_gradient);
    b("vis_health_value", next.vis_health_value);
    b("vis_head", next.vis_head);
    i("vis_head_style", next.vis_head_style, 0, 2);
    i("vis_head_size", next.vis_head_size, 5, 20);
    b("vis_skeleton", next.vis_skeleton);
    i("vis_skeleton_mode", next.vis_skeleton_mode, 0, 2);
    i("vis_skeleton_style", next.vis_skeleton_style, 0, 2);
    i("vis_skeleton_thickness", next.vis_skeleton_thickness, 8, 30);
    b("vis_distance", next.vis_distance);
    b("vis_weapon_icon", next.vis_weapon_icon);
    i("vis_weapon_icon_size", next.vis_weapon_icon_size, 10, 32);
    b("vis_team_check", next.vis_team_check);
    b("vis_visible_only", next.vis_visible_only);
    i("vis_thickness", next.vis_thickness, 8, 28);
    i("vis_glow", next.vis_glow, 10, 90);
    i("vis_corner", next.vis_corner, 16, 42);
    i("vis_max_distance", next.vis_max_distance, 20, 400);
    b("misc_watermark", next.misc_watermark);
    b("misc_hotkeys", next.misc_hotkeys);
    b("vis_team_names", next.vis_team_names);
    b("vis_team_distance", next.vis_team_distance);
    b("spec_enable", next.spec_enable);
    i("spec_anchor", next.spec_anchor, 0, 3);
    i("spec_off_x", next.spec_off_x, -400, 400);
    i("spec_off_y", next.spec_off_y, -400, 400);
    b("hitlog_enable", next.hitlog_enable);
    i("hitlog_max", next.hitlog_max, 1, 8);
    f("hitlog_time", next.hitlog_time, 2.f, 8.f);
    i("hitlog_anchor", next.hitlog_anchor, 0, 3);
    i("hitlog_off_x", next.hitlog_off_x, -400, 400);
    i("hitlog_off_y", next.hitlog_off_y, -400, 400);
    b("hitlog_kill_icon", next.hitlog_kill_icon);
    c4("hitlog_head", next.hitlog_head);
    c4("hitlog_chest", next.hitlog_chest);
    c4("hitlog_body", next.hitlog_body);
    b("aim_enable", next.aim_enable);
    b("aim_visible", next.aim_visible);
    b("aim_recoil", next.aim_recoil);
    b("aim_fov_draw", next.aim_fov_draw);
    b("aim_humanize", next.aim_humanize);
    b("aim_team_check", next.aim_team_check);
    i("aim_key", next.aim_key, 0, 255);
    i("aim_key_mode", next.aim_key_mode, 0, 2);
    i("aim_edit_profile", next.aim_edit_profile, 0, 2);
    const bool got_rifle = read_float(json, "rifle_fov", next.aim_rifle.fov, 0.5f, 15.f);
    if (got_rifle) ++got;
    f("rifle_smooth", next.aim_rifle.smooth, 0.f, 1.f);
    i("rifle_bone", next.aim_rifle.bone, 0, 5);
    f("rifle_rcs_yaw", next.aim_rifle.rcs_yaw, 0.f, 2.5f);
    f("rifle_rcs_pitch", next.aim_rifle.rcs_pitch, 0.f, 2.5f);
    f("pistol_fov", next.aim_pistol.fov, 0.5f, 15.f);
    f("pistol_smooth", next.aim_pistol.smooth, 0.f, 1.f);
    i("pistol_bone", next.aim_pistol.bone, 0, 5);
    f("pistol_rcs_yaw", next.aim_pistol.rcs_yaw, 0.f, 2.5f);
    f("pistol_rcs_pitch", next.aim_pistol.rcs_pitch, 0.f, 2.5f);
    f("sniper_fov", next.aim_sniper.fov, 0.5f, 15.f);
    f("sniper_smooth", next.aim_sniper.smooth, 0.f, 1.f);
    i("sniper_bone", next.aim_sniper.bone, 0, 5);
    f("sniper_rcs_yaw", next.aim_sniper.rcs_yaw, 0.f, 2.5f);
    f("sniper_rcs_pitch", next.aim_sniper.rcs_pitch, 0.f, 2.5f);
    f("aim_reaction_min", next.aim_reaction_min, 0.f, 400.f);
    f("aim_reaction_max", next.aim_reaction_max, 0.f, 400.f);
    f("aim_noise", next.aim_noise, 0.f, 1.f);
    f("aim_overshoot", next.aim_overshoot, 0.f, 1.f);
    f("aim_miss", next.aim_miss, 0.f, 0.25f);
    b("aim_debug", next.aim_debug);
    if (!got_rifle) {
        float old_fov = 0.f;
        if (read_float(json, "aim_fov", old_fov, 0.5f, 30.f)) {
            next.aim_rifle.fov = next.aim_pistol.fov = next.aim_sniper.fov = old_fov;
            ++got;
        }
        int old_smooth = 0;
        if (read_int(json, "aim_smooth", old_smooth, 1, 40)) {
            const float s = std::clamp(old_smooth / 40.f, 0.f, 1.f);
            next.aim_rifle.smooth = next.aim_pistol.smooth = next.aim_sniper.smooth = s;
            ++got;
        }
        int old_bone = 0;
        if (read_int(json, "aim_bone", old_bone, 0, 5)) {
            next.aim_rifle.bone = next.aim_pistol.bone = next.aim_sniper.bone = old_bone;
            ++got;
        }
    }
    b("trigger_enable", next.trigger_enable);
    b("trigger_visible", next.trigger_visible);
    b("trigger_team_check", next.trigger_team_check);
    b("trigger_scope", next.trigger_scope);
    b("trigger_flash", next.trigger_flash);
    b("trigger_weapon_filter", next.trigger_weapon_filter);
    i("trigger_hitbox", next.trigger_hitbox, 0, 2);
    i("trigger_first_ms", next.trigger_first_ms, 0, 400);
    i("trigger_next_ms", next.trigger_next_ms, 0, 500);
    i("trigger_jitter_ms", next.trigger_jitter_ms, 0, 80);
    i("trigger_key", next.trigger_key, 0, 255);
    i("trigger_key_mode", next.trigger_key_mode, 0, 2);
    int old_trig = 0;
    if (read_int(json, "trigger_delay_ms", old_trig, 0, 400) && next.trigger_first_ms == 70)
        next.trigger_first_ms = old_trig;
    i("menu_key", next.menu_key, 0, 255);
    i("menu_key_mode", next.menu_key_mode, 0, 2);
    c4("accent", next.accent);
    c4("box_enemy", next.box_enemy);
    c4("box_team", next.box_team);
    c4("health_low", next.health_low);
    c4("health_high", next.health_high);
    c4("head_enemy", next.head_enemy);
    c4("head_team", next.head_team);
    c4("skeleton_visible", next.skeleton_visible);
    c4("skeleton_hidden", next.skeleton_hidden);
    c4("weapon_icon_color", next.weapon_icon_color);
    return got > 0;
}

static bool write_text_file(const std::string& path, const std::string& body)
{
    HANDLE file = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    const BOOL ok = WriteFile(file, body.data(), static_cast<DWORD>(body.size()), &written, nullptr);
    CloseHandle(file);
    return ok && written == body.size();
}

static bool read_text_file(const std::string& path, std::string& out)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    out = ss.str();
    return true;
}

void ConfigStore::startup()
{
    static bool console = false;
    if (!console) {
        console = true;
        AllocConsole();
        SetConsoleTitleA("CS2 config log");
        FILE* out = nullptr;
        freopen_s(&out, "CONOUT$", "w", stdout);
    }
    ensure_dir();
    log("startup · config dir %s", dir_.c_str());
    refresh();
}

void ConfigStore::refresh()
{
    ensure_dir();
    names_.clear();
    WIN32_FIND_DATAA fd{};
    const std::string pat = dir_ + "\\*.json";
    HANDLE h = FindFirstFileA(pat.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) {
        log("refresh · 0 configs in %s", dir_.c_str());
        return;
    }
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        std::string n = fd.cFileName;
        if (n.size() > 5 && n.rfind(".json") == n.size() - 5)
            n.resize(n.size() - 5);
        if (!sanitize_config_name(n).empty())
            names_.push_back(n);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    std::sort(names_.begin(), names_.end());
    log("refresh · %zu configs", names_.size());
}

bool ConfigStore::save(const std::string& name)
{
    const std::string clean = sanitize_config_name(name);
    if (clean.empty()) {
        log("save failed · invalid name");
        return false;
    }
    ensure_dir();
    return save_to_file(path_for(clean));
}

bool ConfigStore::load(const std::string& name)
{
    const std::string clean = sanitize_config_name(name);
    if (clean.empty()) {
        log("load failed · invalid name");
        return false;
    }
    ensure_dir();
    return load_from_file(path_for(clean));
}

bool ConfigStore::save_to_file(const std::string& path)
{
    if (path.empty()) {
        log("save failed · empty path");
        return false;
    }
    const std::string stem = sanitize_config_name(stem_of(path));
    const std::string body = build_config_json(stem.empty() ? std::string("config") : stem);
    if (!write_text_file(path, body)) {
        log("save failed · %s err=%lu", path.c_str(), GetLastError());
        return false;
    }
    if (!stem.empty())
        last_loaded_ = stem;
    refresh();
    log("saved · %s (team=%d visible_only=%d)", path.c_str(),
        (int)g_menu.vis_team_check, (int)g_menu.vis_visible_only);
    return true;
}

bool ConfigStore::load_from_file(const std::string& path)
{
    if (path.empty()) {
        log("load failed · empty path");
        return false;
    }
    std::string json;
    if (!read_text_file(path, json)) {
        log("load failed · missing %s", path.c_str());
        return false;
    }
    if (json.size() < 8 || json.find('{') == std::string::npos) {
        log("load failed · corrupt %s", path.c_str());
        return false;
    }

    MenuState next{};
    int got = 0;
    if (!parse_config_json(json, next, got)) {
        log("load failed · no fields %s", path.c_str());
        return false;
    }

    g_menu = next;
    apply_accent(g_menu.accent);
    const std::string stem = sanitize_config_name(stem_of(path));
    if (!stem.empty())
        last_loaded_ = stem;
    log("loaded · %s fields=%d team=%d visible_only=%d head=%d skel=%d",
        path.c_str(), got, (int)g_menu.vis_team_check, (int)g_menu.vis_visible_only,
        (int)g_menu.vis_head, (int)g_menu.vis_skeleton);
    return true;
}

namespace {

const wchar_t* cfg_file_filter()
{
    // Doppelt-nullterminierte Filterliste fuer OPENFILENAMEW.
    return L"Config (*.json;*.cfg)\0*.json;*.cfg\0"
           L"JSON config (*.json)\0*.json\0"
           L"CFG config (*.cfg)\0*.cfg\0"
           L"All files (*.*)\0*.*\0";
}

} // namespace

bool ConfigStore::save_as_dialog(HWND owner, const std::string& initial_name)
{
    ensure_dir();
    const std::string base = sanitize_config_name(
        initial_name.empty() ? last_loaded_ : initial_name);
    const std::wstring dir_w = wide_of(dir_);
    wchar_t file[MAX_PATH]{};
    const std::wstring init_w = wide_of(base.empty() ? std::string("config") : base);
    wcsncpy_s(file, init_w.c_str(), _TRUNCATE);

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = cfg_file_filter();
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = dir_w.empty() ? nullptr : dir_w.c_str();
    ofn.lpstrDefExt = L"json";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Save config as...";
    if (!GetSaveFileNameW(&ofn))
        return false; // Benutzerabbruch, kein Fehlerstatus.

    std::string path = utf8_of(file);
    if (path.find_last_of('.') == std::string::npos ||
        path.find_last_of('.') < path.find_last_of("\\/"))
        path += ".json";
    return save_to_file(path);
}

bool ConfigStore::open_dialog(HWND owner)
{
    ensure_dir();
    const std::wstring dir_w = wide_of(dir_);
    wchar_t file[MAX_PATH]{};

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = cfg_file_filter();
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = dir_w.empty() ? nullptr : dir_w.c_str();
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Open config...";
    if (!GetOpenFileNameW(&ofn))
        return false; // Benutzerabbruch, kein Fehlerstatus.

    return load_from_file(utf8_of(file));
}

bool ConfigStore::remove(const std::string& name)
{
    const std::string clean = sanitize_config_name(name);
    if (clean.empty()) {
        log("delete failed · invalid name");
        return false;
    }
    ensure_dir();
    const std::string path = path_for(clean);
    if (DeleteFileA(path.c_str()) == 0) {
        log("delete failed · %s (%lu)", clean.c_str(), GetLastError());
        return false;
    }
    if (last_loaded_ == clean)
        last_loaded_.clear();
    refresh();
    log("deleted · %s", clean.c_str());
    return true;
}

void ConfigStore::reset_defaults()
{
    g_menu = MenuState{};
    apply_accent(g_menu.accent);
    last_loaded_.clear();
    log("reset · defaults");
}

const char* ConfigStore::preset_label(int id)
{
    switch (id) {
    case 0: return "Legit";
    case 1: return "Legit with Aim";
    case 2: return "Semi Rage";
    default: return "?";
    }
}

// Ein-Klick-Presets: nur Combat/Trigger/Humanize/RCS, Visuals & Farben bleiben.
// Keys werden bewusst nicht ueberschrieben.
bool ConfigStore::apply_preset(int id)
{
    if (id < 0 || id >= preset_count()) {
        log("preset failed · bad id %d", id);
        return false;
    }

    if (id == 0) { // Legit: kleiner FOV, viel Smooth, Humanize an, RCS aus/minimal
        g_menu.aim_enable = true;
        g_menu.aim_visible = true;
        g_menu.aim_team_check = true;
        g_menu.aim_fov_draw = true;
        g_menu.aim_humanize = true;
        g_menu.aim_recoil = false;
        g_menu.aim_rifle = { 2.0f, 0.82f, 0, 0.f, 0.f };
        g_menu.aim_pistol = { 1.6f, 0.82f, 0, 0.f, 0.f };
        g_menu.aim_sniper = { 1.4f, 0.78f, 0, 0.f, 0.f };
        g_menu.aim_reaction_min = 90.f;
        g_menu.aim_reaction_max = 180.f;
        g_menu.aim_noise = 0.22f;
        g_menu.aim_overshoot = 0.14f;
        g_menu.aim_miss = 0.04f;
        g_menu.trigger_enable = false;
        g_menu.trigger_visible = true;
        g_menu.trigger_team_check = true;
        g_menu.trigger_scope = true;
        g_menu.trigger_flash = true;
        g_menu.trigger_weapon_filter = true;
        g_menu.trigger_hitbox = 0;
        g_menu.trigger_first_ms = 80;
        g_menu.trigger_next_ms = 170;
        g_menu.trigger_jitter_ms = 20;
    } else if (id == 1) { // Legit with Aim: moderat, RCS an, Aim + Trigger an
        g_menu.aim_enable = true;
        g_menu.aim_visible = true;
        g_menu.aim_team_check = true;
        g_menu.aim_fov_draw = true;
        g_menu.aim_humanize = true;
        g_menu.aim_recoil = true;
        g_menu.aim_rifle = { 4.0f, 0.55f, 0, 2.0f, 2.0f };
        g_menu.aim_pistol = { 3.0f, 0.50f, 0, 1.5f, 1.5f };
        g_menu.aim_sniper = { 2.5f, 0.35f, 0, 1.0f, 1.0f };
        g_menu.aim_reaction_min = 60.f;
        g_menu.aim_reaction_max = 130.f;
        g_menu.aim_noise = 0.16f;
        g_menu.aim_overshoot = 0.10f;
        g_menu.aim_miss = 0.02f;
        g_menu.trigger_enable = true;
        g_menu.trigger_visible = true;
        g_menu.trigger_team_check = true;
        g_menu.trigger_scope = true;
        g_menu.trigger_flash = true;
        g_menu.trigger_weapon_filter = true;
        g_menu.trigger_hitbox = 0;
        g_menu.trigger_first_ms = 70;
        g_menu.trigger_next_ms = 150;
        g_menu.trigger_jitter_ms = 18;
    } else { // Semi Rage: grosser FOV, kaum Smooth, RCS aggressiv, Trigger minimal
        g_menu.aim_enable = true;
        g_menu.aim_visible = true; // Vis-Check priorisiert
        g_menu.aim_team_check = true;
        g_menu.aim_fov_draw = true;
        g_menu.aim_humanize = false;
        g_menu.aim_recoil = true;
        g_menu.aim_rifle = { 10.0f, 0.05f, 0, 2.5f, 2.5f };
        g_menu.aim_pistol = { 8.0f, 0.05f, 0, 2.5f, 2.5f };
        g_menu.aim_sniper = { 7.0f, 0.05f, 0, 2.5f, 2.5f };
        g_menu.aim_reaction_min = 0.f;
        g_menu.aim_reaction_max = 25.f;
        g_menu.aim_noise = 0.05f;
        g_menu.aim_overshoot = 0.f;
        g_menu.aim_miss = 0.f;
        g_menu.trigger_enable = true;
        g_menu.trigger_visible = true; // Vis-Check priorisiert
        g_menu.trigger_team_check = true;
        g_menu.trigger_scope = false;
        g_menu.trigger_flash = false;
        g_menu.trigger_weapon_filter = false;
        g_menu.trigger_hitbox = 2; // Body
        g_menu.trigger_first_ms = 12;
        g_menu.trigger_next_ms = 45;
        g_menu.trigger_jitter_ms = 6;
    }

    last_loaded_.clear();
    log("preset · %s applied", preset_label(id));
    return true;
}
