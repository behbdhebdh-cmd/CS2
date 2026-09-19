#pragma once

enum class BoxStyle {
    Corner = 0,
    Box3D  = 1,
    Filled = 2,
};

struct MenuState {
    bool aim_enable = false;
    bool aim_visible = true;
    bool aim_recoil = false;
    bool trigger_enable = false;

    bool vis_enable = true;
    bool vis_health = true;
    bool vis_team_check = true;
    bool vis_visible_only = true;
    int  vis_box_style = static_cast<int>(BoxStyle::Corner);
    int  vis_thickness = 11;     // 0.1 px units
    int  vis_glow = 28;
    int  vis_corner = 24;
    int  vis_max_distance = 220;

    bool misc_watermark = true;

    int aim_fov = 8;
    int aim_smooth = 12;
    int aim_bone = 0;
    int trigger_delay_ms = 20;

    int aim_key = 0;
    int aim_key_mode = 1;
    int trigger_key = 0;
    int trigger_key_mode = 1;
    int menu_key = 0;
    int menu_key_mode = 0;

    float accent[4] = { 0.78f, 0.82f, 0.90f, 1.f };
    float box_enemy[4]  = { 0.82f, 0.86f, 0.92f, 0.92f };
    float box_team[4]   = { 0.55f, 0.78f, 0.92f, 0.75f };
};

inline MenuState g_menu;
inline bool g_menu_open = false;
inline bool g_want_quit = false;
inline float g_menu_x = 80.f;
inline float g_menu_y = 80.f;
inline float g_menu_w = 780.f;
inline float g_menu_h = 540.f;

struct HitRect { float x, y, w, h; };
inline HitRect g_hits[24]{};
inline int g_hitn = 0;
