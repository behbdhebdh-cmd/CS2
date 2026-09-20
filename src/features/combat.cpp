#include "features/combat.hpp"
#include "app/settings.hpp"
#include "sdk/game.hpp"
#include "sdk/math.hpp"
#include "sdk/skeleton.hpp"
#include "sdk/vis.hpp"

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
#include <cstdint>
#include <cstdio>
#include <random>

namespace {

std::mt19937& rng()
{
    static std::mt19937 g{ std::random_device{}() };
    return g;
}

float urand(float lo, float hi)
{
    if (hi <= lo)
        return lo;
    std::uniform_real_distribution<float> d(lo, hi);
    return d(rng());
}

float nrand()
{
    std::normal_distribution<float> d(0.f, 1.f);
    return d(rng());
}

float lognormal_ms(float lo, float hi)
{
    lo = (std::max)(0.f, lo);
    hi = (std::max)(lo, hi);
    if (hi - lo < 1.f)
        return lo;
    const float mu = std::log(std::sqrt((std::max)(lo, 1.f) * hi));
    const float sigma = std::log(hi / (std::max)(lo, 1.f)) * 0.25f;
    std::lognormal_distribution<float> d(mu, (std::max)(0.05f, sigma));
    return clampf(d(rng()), lo, hi);
}

bool key_held(int vk)
{
    if (vk <= 0 || vk > 255)
        return false;
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool game_focused()
{
    HWND fg = GetForegroundWindow();
    if (!fg)
        return false;
    wchar_t title[256]{};
    GetWindowTextW(fg, title, 256);
    return wcsstr(title, L"Counter-Strike 2") != nullptr;
}

void mouse_move(int dx, int dy)
{
    if (!dx && !dy)
        return;
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dx = dx;
    in.mi.dy = dy;
    in.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &in, sizeof(INPUT));
}

void mouse_btn(bool down)
{
    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    SendInput(1, &in, sizeof(INPUT));
}

bool is_knife(int def)
{
    if (def == 41 || def == 42 || def == 59)
        return true;
    return def >= 500;
}

bool is_grenade_or_bomb(int def)
{
    switch (def) {
    case 43: case 44: case 45: case 46: case 47: case 48:
    case 49: case 57: case 68: case 84:
        return true;
    default:
        return false;
    }
}

bool is_sniper(int def)
{
    return def == 9 || def == 11 || def == 38 || def == 40;
}

bool is_pistol(int def)
{
    switch (def) {
    case 1: case 2: case 3: case 4: case 30: case 31:
    case 32: case 36: case 61: case 63: case 64:
        return true;
    default:
        return false;
    }
}

Vec3 bone_world(const Player& p, Skel j)
{
    if (p.has_joint(j))
        return p.joints[static_cast<int>(j)];
    const float chest_z = p.ducked ? 32.f : 48.f;
    switch (j) {
    case Skel::Head:
        return p.eye.z > p.origin.z + 8.f ? p.eye : p.head;
    case Skel::Neck:
        return p.origin + Vec3{ 0.f, 0.f, chest_z + 12.f };
    case Skel::SpineUpper:
        return p.origin + Vec3{ 0.f, 0.f, chest_z + 6.f };
    case Skel::SpineLower:
        return p.origin + Vec3{ 0.f, 0.f, chest_z };
    default:
        return p.eye;
    }
}

void bones_for_mode(int mode, Skel* out, int& n)
{
    n = 0;
    auto push = [&](Skel s) { if (n < 6) out[n++] = s; };
    switch (mode) {
    case static_cast<int>(AimBone::Neck):
        push(Skel::Neck); push(Skel::Head); push(Skel::SpineUpper);
        break;
    case static_cast<int>(AimBone::Chest):
        push(Skel::SpineLower); push(Skel::SpineUpper); push(Skel::Head);
        break;
    case static_cast<int>(AimBone::UpperChest):
        push(Skel::SpineUpper); push(Skel::SpineLower); push(Skel::Head);
        break;
    case static_cast<int>(AimBone::HeadThenChest):
        push(Skel::Head); push(Skel::SpineUpper); push(Skel::SpineLower);
        break;
    case static_cast<int>(AimBone::Priority):
        push(Skel::Head); push(Skel::Neck); push(Skel::SpineUpper); push(Skel::SpineLower);
        break;
    case static_cast<int>(AimBone::Head):
    default:
        push(Skel::Head); push(Skel::Neck); push(Skel::SpineUpper);
        break;
    }
}

bool player_visible(const Game& game, const VisCheck& vis, const Player& p, const Vec3& bone)
{
    if (!vis.ready())
        return true;
    const Vec3 from = game.local_head();
    const Vec3 head = bone_world(p, Skel::Head);
    return vis.visible(from, bone) || vis.visible(from, head);
}

bool enemy_ok(const Game& game, const Player& p, bool team_check)
{
    if (p.health <= 0)
        return false;
    if (p.team < 2 || p.team > 3)
        return false;
    if (team_check && p.team == game.local_team())
        return false;
    return true;
}

ULONGLONG now_ms()
{
    return GetTickCount64();
}

struct AimRt {
    uintptr_t pawn = 0;
    Skel bone = Skel::Head;
    ULONGLONG acquire_ms = 0;
    ULONGLONG react_until = 0;
    ULONGLONG lost_ms = 0;
    ULONGLONG overshoot_until = 0;
    ULONGLONG miss_until = 0;
    Vec3 lock_off{};
    Vec3 overshoot{};
    Vec3 miss_off{};
    float ou_yaw = 0.f;
    float ou_pitch = 0.f;
    float acc_x = 0.f;
    float acc_y = 0.f;
    Vec3 last_punch{};
    bool punching = false;
};

struct TrigRt {
    bool pending = false;
    bool holding = false;
    bool burst = false;
    ULONGLONG fire_at = 0;
    ULONGLONG release_at = 0;
    ULONGLONG cooldown_until = 0;
};

AimRt g_aim;
TrigRt g_trig;

void reset_aim()
{
    g_aim.pawn = 0;
    g_aim.acquire_ms = 0;
    g_aim.react_until = 0;
    g_aim.overshoot_until = 0;
    g_aim.miss_until = 0;
    g_aim.lost_ms = 0;
    g_aim.punching = false;
}

void reset_trig()
{
    if (g_trig.holding)
        mouse_btn(false);
    g_trig = {};
}

void ou_step(float& x, float dt, float theta, float sigma)
{
    const float sdt = std::sqrt((std::max)(dt, 0.0005f));
    x += (-theta * x) * dt + sigma * sdt * nrand();
    x = clampf(x, -1.6f, 1.6f);
}

const char* skel_dbg_name(Skel b)
{
    switch (b) {
    case Skel::Head: return "Head";
    case Skel::Neck: return "Neck";
    case Skel::SpineUpper: return "SpineUpper";
    case Skel::SpineLower: return "SpineLower";
    default: return "Unknown";
    }
}

// Throttled aim debug log: eye, bone world, want/view angles, delta pre/post
// smooth, sensitivity, resulting mouse deltas + view-matrix sanity.
// Aim path is angle-based (calc_angle -> angle_delta -> SendInput); world_to_screen
// is NOT used for aiming (only ESP), so a W2S error cannot explain away-aim.
// Log goes to OutputDebugString + configs/aim_debug.log when g_menu.aim_debug is on.
void aim_debug_log(const Game& game, const Vec3& eye, const Vec3& world, Skel bone,
                   const Vec3& want, const Vec3& view, const Vec3& delta_raw,
                   const Vec3& delta_applied, float smooth_a, float sens,
                   int mx, int my, float fov, bool force)
{
    if (!g_menu.aim_debug)
        return;
    const ULONGLONG t = now_ms();
    static ULONGLONG last = 0;
    if (!force && t - last < 150)
        return;
    last = t;

    const Mat4x4& vm = game.view_matrix();
    bool vm_ok = true;
    for (int r = 0; r < 4 && vm_ok; ++r)
        for (int c = 0; c < 4 && vm_ok; ++c)
            if (!std::isfinite(vm.m[r][c]))
                vm_ok = false;

    char line[768]{};
    std::snprintf(line, sizeof(line),
        "[aim] bone=%s eye=(%.1f,%.1f,%.1f) world=(%.1f,%.1f,%.1f) "
        "view=(p %.2f y %.2f) want=(p %.2f y %.2f) "
        "dRaw=(p %.3f y %.3f) dSm=(p %.3f y %.3f a=%.3f) "
        "fov=%.2f sens=%.3f mouse=(%d,%d) vm=%s",
        skel_dbg_name(bone),
        eye.x, eye.y, eye.z, world.x, world.y, world.z,
        view.x, view.y, want.x, want.y,
        delta_raw.x, delta_raw.y, delta_applied.x, delta_applied.y, smooth_a,
        fov, sens, mx, my, vm_ok ? "ok" : "NAN");
    OutputDebugStringA(line);
    OutputDebugStringA("\n");

    static FILE* fp = nullptr;
    static bool tried = false;
    if (!tried) {
        tried = true;
        fopen_s(&fp, "D:\\CS2\\configs\\aim_debug.log", "a");
        if (fp) {
            std::fprintf(fp, "--- aim debug session ---\n");
            std::fflush(fp);
        }
    } else if (!fp) {
        fopen_s(&fp, "D:\\CS2\\configs\\aim_debug.log", "a");
    }
    if (fp) {
        std::fprintf(fp, "%s\n", line);
        std::fflush(fp);
    }
}

bool pick_target(const Game& game, const VisCheck& vis, const CombatProfile& prof,
                 bool vis_only, bool team_check, uintptr_t sticky, Skel sticky_bone,
                 const Player*& out_p, Skel& out_bone, float& out_fov)
{
    out_p = nullptr;
    out_bone = Skel::Head;
    out_fov = 1e9f;

    Skel order[6]{};
    int n = 0;
    bones_for_mode(prof.bone, order, n);

    const Vec3 eye = game.local_head();
    const Vec3 view = game.view_angles();
    const float max_fov = (std::max)(0.35f, prof.fov);

    auto consider = [&](const Player& p, Skel bone, float fov) {
        if (fov > max_fov)
            return;
        const bool sticky_hit = sticky && p.pawn == sticky;
        const float score = sticky_hit ? fov * 0.72f : fov;
        if (score < out_fov) {
            out_fov = fov;
            out_p = &p;
            out_bone = bone;
        }
    };

    if (sticky) {
        for (const Player& p : game.players()) {
            if (p.pawn != sticky || !enemy_ok(game, p, team_check))
                continue;
            const Vec3 pos = bone_world(p, sticky_bone);
            if (vis_only && !player_visible(game, vis, p, pos))
                break;
            const float fov = fov_degrees(view, calc_angle(eye, pos));
            if (fov <= max_fov * 1.18f) {
                out_p = &p;
                out_bone = sticky_bone;
                out_fov = fov;
                return true;
            }
            break;
        }
    }

    for (const Player& p : game.players()) {
        if (!enemy_ok(game, p, team_check))
            continue;
        for (int i = 0; i < n; ++i) {
            const Vec3 pos = bone_world(p, order[i]);
            if (vis_only && !player_visible(game, vis, p, pos))
                continue;
            const float fov = fov_degrees(view, calc_angle(eye, pos));
            consider(p, order[i], fov);
            if (prof.bone != static_cast<int>(AimBone::Priority) &&
                prof.bone != static_cast<int>(AimBone::HeadThenChest))
                break;
            if (out_p == &p)
                break;
        }
    }
    return out_p != nullptr;
}

void run_aim(const Game& game, const VisCheck& vis, const CombatProfile& prof, float dt)
{
    if (!g_menu.aim_enable || !game.local_alive() || !key_held(g_menu.aim_key)) {
        reset_aim();
        g_aim.acc_x = g_aim.acc_y = 0.f;
        return;
    }
    if (is_knife(game.weapon_def()) || is_grenade_or_bomb(game.weapon_def())) {
        reset_aim();
        return;
    }

    const Player* tgt = nullptr;
    Skel bone = Skel::Head;
    float fov = 0.f;
    if (!pick_target(game, vis, prof, g_menu.aim_visible, g_menu.aim_team_check,
                     g_aim.pawn, g_aim.bone, tgt, bone, fov)) {
        if (g_aim.pawn && g_aim.lost_ms == 0)
            g_aim.lost_ms = now_ms();
        if (!g_aim.pawn || now_ms() - g_aim.lost_ms > 90)
            reset_aim();
        return;
    }
    g_aim.lost_ms = 0;

    const ULONGLONG t = now_ms();
    if (tgt->pawn != g_aim.pawn) {
        const bool reacquire = g_aim.pawn != 0 && t - g_aim.lost_ms < 80;
        g_aim.pawn = tgt->pawn;
        g_aim.bone = bone;
        g_aim.acquire_ms = t;
        g_aim.lock_off = { urand(-1.1f, 1.1f), urand(-1.1f, 1.1f), urand(-0.8f, 0.8f) };
        g_aim.overshoot = {};
        g_aim.miss_off = {};
        g_aim.miss_until = 0;
        g_aim.overshoot_until = 0;
        if (!reacquire && g_menu.aim_humanize) {
            const float lo = (std::min)(g_menu.aim_reaction_min, g_menu.aim_reaction_max);
            const float hi = (std::max)(g_menu.aim_reaction_min, g_menu.aim_reaction_max);
            g_aim.react_until = t + static_cast<ULONGLONG>(lognormal_ms(lo, hi) + 0.5f);
        } else {
            g_aim.react_until = t;
        }
        if (g_menu.aim_humanize && urand(0.f, 1.f) < clampf(g_menu.aim_overshoot, 0.f, 1.f)) {
            const float mag = urand(0.12f, 0.38f);
            g_aim.overshoot = { urand(-mag, mag), urand(-mag, mag) * 1.2f, 0.f };
            g_aim.overshoot_until = t + static_cast<ULONGLONG>(urand(80.f, 170.f));
        }
        if (g_menu.aim_humanize && urand(0.f, 1.f) < clampf(g_menu.aim_miss, 0.f, 0.25f)) {
            g_aim.miss_off = { urand(-12.f, 12.f), urand(-12.f, 12.f), urand(-2.f, 6.f) };
            g_aim.miss_until = t + static_cast<ULONGLONG>(urand(160.f, 320.f));
        }
    } else {
        g_aim.bone = bone;
    }

    if (t < g_aim.react_until)
        return;

    Vec3 world = bone_world(*tgt, g_aim.bone);
    if (g_menu.aim_humanize) {
        g_aim.lock_off.x += nrand() * dt * 0.35f;
        g_aim.lock_off.y += nrand() * dt * 0.35f;
        g_aim.lock_off.z += nrand() * dt * 0.2f;
        g_aim.lock_off.x = clampf(g_aim.lock_off.x, -1.6f, 1.6f);
        g_aim.lock_off.y = clampf(g_aim.lock_off.y, -1.6f, 1.6f);
        g_aim.lock_off.z = clampf(g_aim.lock_off.z, -1.2f, 1.2f);
        world = world + g_aim.lock_off;
        if (t < g_aim.miss_until)
            world = world + g_aim.miss_off;
    }

    Vec3 want = calc_angle(game.local_head(), world);
    if (g_menu.aim_recoil && game.shots_fired() > 0) {
        const Vec3 punch = game.punch_angles();
        want.x -= punch.x * clampf(prof.rcs_pitch, 0.f, 2.5f);
        want.y -= punch.y * clampf(prof.rcs_yaw, 0.f, 2.5f);
        g_aim.last_punch = punch;
        g_aim.punching = true;
    }

    if (g_menu.aim_humanize) {
        const float sigma = 0.08f + clampf(g_menu.aim_noise, 0.f, 1.f) * 0.55f;
        ou_step(g_aim.ou_yaw, dt, 6.5f, sigma);
        ou_step(g_aim.ou_pitch, dt, 6.5f, sigma * 0.7f);
        want.y += g_aim.ou_yaw * 0.22f;
        want.x += g_aim.ou_pitch * 0.18f;
        if (t < g_aim.overshoot_until) {
            want.x += g_aim.overshoot.x;
            want.y += g_aim.overshoot.y;
        }
    }
    normalize_angles(want);

    const Vec3 view_now = game.view_angles();
    const Vec3 delta_raw = angle_delta(view_now, want);
    Vec3 delta = delta_raw;
    float smooth_a = 1.f;
    const float smooth = clampf(prof.smooth, 0.f, 1.f);
    if (smooth > 0.02f) {
        const float tau = 0.035f + smooth * 0.20f;
        smooth_a = 1.f - std::exp(-dt / tau);
        delta.x *= smooth_a;
        delta.y *= smooth_a;
    }

    float sens = game.sensitivity();
    if (sens < 0.05f)
        sens = 1.f;
    // Source engine mouse mapping (in_mouse.cpp):
    //   view.yaw   -= m_yaw   * dx   (mouse right -> yaw decreases / turn right)
    //   view.pitch += m_pitch * dy   (mouse down -> pitch increases / look down)
    // Delta here is (want - view), so the correct pixel conversion is:
    //   dx = -delta.yaw / (sens * m_yaw), dy = +delta.pitch / (sens * m_pitch).
    // BUG WAS: dx used +delta.yaw (missing minus) -> horizontal spiegelung,
    // der Aim lief seitlich vom Gegner weg statt drauf.
    constexpr float kMouse = 0.022f; // m_yaw == m_pitch == 0.022 default
    g_aim.acc_x -= delta.y / (sens * kMouse); // yaw: negiert (Fix)
    g_aim.acc_y += delta.x / (sens * kMouse); // pitch: positiv (war korrekt)

    int mx = static_cast<int>(g_aim.acc_x);
    int my = static_cast<int>(g_aim.acc_y);
    g_aim.acc_x -= static_cast<float>(mx);
    g_aim.acc_y -= static_cast<float>(my);

    const int cap = 28 + static_cast<int>((1.f - smooth) * 36.f);
    const int mx_capped = std::clamp(mx, -cap, cap);
    const int my_capped = std::clamp(my, -cap, cap);
    aim_debug_log(game, game.local_head(), world, g_aim.bone, want, view_now,
                  delta_raw, delta, smooth_a, sens, mx_capped, my_capped, fov, false);
    mouse_move(mx_capped, my_capped);
}

float hitbox_radius(Skel bone)
{
    switch (bone) {
    case Skel::Head: return 5.4f;
    case Skel::Neck: return 4.8f;
    case Skel::SpineUpper: return 8.5f;
    case Skel::SpineLower: return 9.5f;
    default: return 7.f;
    }
}

bool on_crosshair(const Game& game, const VisCheck& vis, const Player& p, int hitbox)
{
    Skel bones[4]{};
    int n = 0;
    if (hitbox == static_cast<int>(TriggerHitbox::Chest)) {
        bones[n++] = Skel::SpineUpper;
        bones[n++] = Skel::SpineLower;
    } else if (hitbox == static_cast<int>(TriggerHitbox::Body)) {
        bones[n++] = Skel::Head;
        bones[n++] = Skel::Neck;
        bones[n++] = Skel::SpineUpper;
        bones[n++] = Skel::SpineLower;
    } else {
        bones[n++] = Skel::Head;
        bones[n++] = Skel::Neck;
    }

    const Vec3 eye = game.local_head();
    const Vec3 view = game.view_angles();
    for (int i = 0; i < n; ++i) {
        const Vec3 pos = bone_world(p, bones[i]);
        if (g_menu.trigger_visible && !player_visible(game, vis, p, pos))
            continue;
        const float dist = eye.dist(pos);
        if (dist < 8.f)
            continue;
        const float ang = fov_degrees(view, calc_angle(eye, pos));
        const float size = std::atan(hitbox_radius(bones[i]) / dist) * kRad2Deg;
        if (ang <= size * 1.08f)
            return true;
    }
    return false;
}

int jittered(int base, int jitter)
{
    const int j = (std::max)(0, jitter);
    const int b = (std::max)(0, base);
    if (j <= 0)
        return b;
    std::uniform_int_distribution<int> d(-j, j);
    return (std::max)(0, b + d(rng()));
}

void run_trigger(const Game& game, const VisCheck& vis)
{
    const bool want = g_menu.trigger_enable && game.local_alive() && key_held(g_menu.trigger_key);
    if (!want) {
        reset_trig();
        return;
    }

    const int def = game.weapon_def();
    if (g_menu.trigger_weapon_filter && (is_knife(def) || is_grenade_or_bomb(def) || def <= 0)) {
        reset_trig();
        return;
    }
    if (g_menu.trigger_scope && is_sniper(def) && !game.scoped()) {
        reset_trig();
        return;
    }
    if (g_menu.trigger_flash && game.flash_alpha() > 80.f) {
        reset_trig();
        return;
    }
    if (key_held(VK_LBUTTON) && !g_trig.holding)
        return;

    bool hit = false;
    for (const Player& p : game.players()) {
        if (!enemy_ok(game, p, g_menu.trigger_team_check))
            continue;
        if (on_crosshair(game, vis, p, g_menu.trigger_hitbox)) {
            hit = true;
            break;
        }
    }

    const ULONGLONG t = now_ms();
    if (g_trig.holding) {
        if (t >= g_trig.release_at) {
            mouse_btn(false);
            g_trig.holding = false;
            g_trig.cooldown_until = t + static_cast<ULONGLONG>(
                jittered(g_menu.trigger_next_ms, g_menu.trigger_jitter_ms));
        }
        return;
    }

    if (!hit) {
        g_trig.pending = false;
        return;
    }
    if (t < g_trig.cooldown_until)
        return;

    if (!g_trig.pending) {
        g_trig.pending = true;
        const int delay = g_trig.burst
            ? jittered(0, g_menu.trigger_jitter_ms)
            : jittered(g_menu.trigger_first_ms, g_menu.trigger_jitter_ms);
        g_trig.fire_at = t + static_cast<ULONGLONG>(delay);
        return;
    }
    if (t < g_trig.fire_at)
        return;

    mouse_btn(true);
    g_trig.holding = true;
    g_trig.pending = false;
    g_trig.burst = true;
    g_trig.release_at = t + static_cast<ULONGLONG>(urand(22.f, 42.f));
}

} // namespace

WpnClass classify_weapon(int def)
{
    if (is_knife(def) || is_grenade_or_bomb(def) || def <= 0)
        return WpnClass::Utility;
    if (is_sniper(def))
        return WpnClass::Sniper;
    if (is_pistol(def))
        return WpnClass::Pistol;
    return WpnClass::Rifle;
}

const char* weapon_class_name(WpnClass c)
{
    switch (c) {
    case WpnClass::Pistol: return "Pistol";
    case WpnClass::Sniper: return "Sniper";
    case WpnClass::Utility: return "Utility";
    default: return "Rifle";
    }
}

const CombatProfile& combat_active_profile(const Game& game)
{
    switch (classify_weapon(game.weapon_def())) {
    case WpnClass::Pistol: return g_menu.aim_pistol;
    case WpnClass::Sniper: return g_menu.aim_sniper;
    default: return g_menu.aim_rifle;
    }
}

uintptr_t combat_aim_pawn()
{
    return g_aim.pawn;
}

int combat_aim_bone()
{
    return g_aim.pawn ? static_cast<int>(g_aim.bone) : -1;
}

void combat_tick(const Game& game, const VisCheck& vis, float dt, bool menu_open)
{
    if (!game.attached() || menu_open || !game_focused()) {
        reset_aim();
        reset_trig();
        return;
    }
    dt = clampf(dt, 0.001f, 0.05f);
    const CombatProfile& prof = combat_active_profile(game);
    run_aim(game, vis, prof, dt);
    run_trigger(game, vis);
}

void combat_draw(ImDrawList* dl, const Game& game, float screen_w, float screen_h)
{
    if (!dl || !g_menu.aim_enable || !g_menu.aim_fov_draw || !game.attached())
        return;
    const CombatProfile& prof = combat_active_profile(game);
    float gfov = static_cast<float>(game.camera_fov());
    if (gfov < 10.f)
        gfov = 90.f;
    const float radius = std::tan(prof.fov * kDeg2Rad) / std::tan(gfov * 0.5f * kDeg2Rad) * (screen_h * 0.5f);
    if (!std::isfinite(radius) || radius < 2.f || radius > screen_h)
        return;
    const ImVec2 c(screen_w * 0.5f, screen_h * 0.5f);
    const ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(
        g_menu.accent[0], g_menu.accent[1], g_menu.accent[2], 0.38f));
    dl->AddCircle(c, radius, col, 64, 1.15f);
}
