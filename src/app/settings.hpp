#pragma once

enum class BoxStyle {
    Corner = 0,
    Box3D  = 1,
    Filled = 2,
};

enum class HealthBarPosition {
    Left = 0,
    Right = 1,
};

enum class HeadMarkerStyle {
    Circle = 0,
    Dot = 1,
    Box = 2,
};

enum class SkeletonBodyMode {
    Head = 0,
    Upper = 1,
    Full = 2,
};

enum class SkeletonStyle {
    Lines = 0,
    Points = 1,
    LinesAndPoints = 2,
};

struct MenuState {
    bool aim_enable = false;
    bool aim_visible = true;
    bool aim_recoil = false;
    bool trigger_enable = false;

    bool vis_enable = true;
    bool vis_health = true;
    int  vis_health_position = static_cast<int>(HealthBarPosition::Right);
    int  vis_health_width = 3;
    bool vis_health_gradient = true;
    bool vis_health_value = false;
    bool vis_head = true;
    int  vis_head_style = static_cast<int>(HeadMarkerStyle::Circle);
    int  vis_head_size = 10;
    bool vis_skeleton = true;
    int  vis_skeleton_mode = static_cast<int>(SkeletonBodyMode::Full);
    int  vis_skeleton_style = static_cast<int>(SkeletonStyle::LinesAndPoints);
    int  vis_skeleton_thickness = 12; // 0.1 px units
    bool vis_team_check = true;
    bool vis_visible_only = true;
    bool vis_distance = true;
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
    float health_low[4]  = { 0.86f, 0.18f, 0.20f, 1.f };
    float health_high[4] = { 0.28f, 0.84f, 0.42f, 1.f };
    float head_enemy[4]  = { 0.98f, 0.54f, 0.30f, 0.96f };
    float head_team[4]   = { 0.55f, 0.78f, 0.92f, 0.82f };
    float skeleton_visible[4] = { 0.48f, 0.90f, 0.72f, 0.94f };
    float skeleton_hidden[4]  = { 0.94f, 0.43f, 0.46f, 0.78f };
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
