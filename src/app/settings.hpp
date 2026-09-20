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

enum class AimBone {
    Head = 0,
    Neck = 1,
    Chest = 2,
    UpperChest = 3,
    HeadThenChest = 4,
    Priority = 5,
};

enum class TriggerHitbox {
    Head = 0,
    Chest = 1,
    Body = 2,
};

struct CombatProfile {
    float fov = 3.5f;
    float smooth = 0.65f;
    int bone = static_cast<int>(AimBone::Head);
    float rcs_yaw = 2.f;
    float rcs_pitch = 2.f;
};

struct MenuState {
    bool aim_enable = false;
    bool aim_visible = true;
    bool aim_recoil = false;
    bool aim_fov_draw = true;
    bool aim_humanize = true;
    bool aim_team_check = true;
    int  aim_key = 0x12; // VK_MENU
    int  aim_key_mode = 0;
    int  aim_edit_profile = 0; // 0 rifle, 1 pistol, 2 sniper
    CombatProfile aim_rifle{ 3.5f, 0.65f, static_cast<int>(AimBone::Head), 2.f, 2.f };
    CombatProfile aim_pistol{ 2.4f, 0.45f, static_cast<int>(AimBone::Head), 1.2f, 1.2f };
    CombatProfile aim_sniper{ 2.0f, 0.22f, static_cast<int>(AimBone::Head), 0.f, 0.f };
    float aim_reaction_min = 70.f;
    float aim_reaction_max = 150.f;
    float aim_noise = 0.18f;
    float aim_overshoot = 0.12f;
    float aim_miss = 0.03f;
    bool aim_debug = false;

    bool trigger_enable = false;
    bool trigger_visible = true;
    bool trigger_team_check = true;
    bool trigger_scope = true;
    bool trigger_flash = true;
    bool trigger_weapon_filter = true;

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
    bool vis_weapon_icon = true;
    int  vis_weapon_icon_size = 18;
    int  vis_box_style = static_cast<int>(BoxStyle::Corner);
    int  vis_thickness = 11;     // 0.1 px units
    int  vis_glow = 28;
    int  vis_corner = 24;
    int  vis_max_distance = 220;

    bool misc_watermark = true;
    bool misc_hotkeys = true;

    // Team ESP + spectator panel
    bool vis_team_names = true;
    bool vis_team_distance = true;
    bool spec_enable = true;
    int  spec_anchor = 0; // 0 TR, 1 TL, 2 BL, 3 BR
    int  spec_off_x = 0;
    int  spec_off_y = 0;

    // Hitmarker
    bool hit_enable = true;
    int  hit_size = 14;      // half-length px, 6..20
    int  hit_thick = 3;      // line px, 1..5
    float hit_alpha = 1.f;   // 0.2..1
    int  hit_time = 300;     // ms, 150..500
    float hit_normal[4] = { 1.f, 1.f, 1.f, 1.f };
    float hit_head[4] = { 0.98f, 0.72f, 0.30f, 1.f };

    // Damage log
    bool hitlog_enable = true;
    int  hitlog_max = 5;       // 1..8
    float hitlog_time = 4.f;   // s, 2..8
    int  hitlog_anchor = 3;    // 0 TR, 1 TL, 2 BL, 3 BR
    int  hitlog_off_x = 0;
    int  hitlog_off_y = 0;
    bool hitlog_kill_icon = true;
    float hitlog_head[4] = { 0.95f, 0.75f, 0.35f, 1.f };
    float hitlog_chest[4] = { 0.98f, 0.54f, 0.30f, 1.f };
    float hitlog_body[4] = { 0.55f, 0.78f, 0.92f, 1.f };

    int trigger_hitbox = static_cast<int>(TriggerHitbox::Head);
    int trigger_first_ms = 70;
    int trigger_next_ms = 155;
    int trigger_jitter_ms = 18;
    int trigger_key = 0x12; // VK_MENU
    int trigger_key_mode = 0;
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
    float weapon_icon_color[4] = { 0.95f, 0.95f, 0.98f, 0.95f };
};

inline MenuState g_menu;
inline bool g_menu_open = false;
inline bool g_want_quit = false;
inline float g_menu_x = 80.f;
inline float g_menu_y = 80.f;
inline float g_menu_w = 860.f;
inline float g_menu_h = 640.f;

struct HitRect { float x, y, w, h; };
inline HitRect g_hits[24]{};
inline int g_hitn = 0;
