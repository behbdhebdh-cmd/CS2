#pragma once

#include <cmath>
#include <cstdint>

struct Vec3 {
    float x = 0.f, y = 0.f, z = 0.f;

    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }

    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float dist(const Vec3& o) const { return (*this - o).length(); }
};

struct Vec2 {
    float x = 0.f, y = 0.f;
};

struct Mat4x4 {
    float m[4][4]{};
};

inline bool world_to_screen(const Vec3& world, const Mat4x4& vm, float screen_w, float screen_h, Vec2& out)
{
    const float w = vm.m[3][0] * world.x + vm.m[3][1] * world.y + vm.m[3][2] * world.z + vm.m[3][3];
    if (w < 0.001f)
        return false;

    const float inv = 1.f / w;
    const float nx = vm.m[0][0] * world.x + vm.m[0][1] * world.y + vm.m[0][2] * world.z + vm.m[0][3];
    const float ny = vm.m[1][0] * world.x + vm.m[1][1] * world.y + vm.m[1][2] * world.z + vm.m[1][3];

    out.x = (screen_w * 0.5f) + (nx * inv) * (screen_w * 0.5f);
    out.y = (screen_h * 0.5f) - (ny * inv) * (screen_h * 0.5f);
    return std::isfinite(out.x) && std::isfinite(out.y);
}

inline constexpr float kDeg2Rad = 0.017453292519943295f;
inline constexpr float kRad2Deg = 57.29577951308232f;

inline float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

inline void normalize_angles(Vec3& a)
{
    while (a.y > 180.f) a.y -= 360.f;
    while (a.y < -180.f) a.y += 360.f;
    a.x = clampf(a.x, -89.f, 89.f);
    a.z = 0.f;
}

inline Vec3 calc_angle(const Vec3& src, const Vec3& dst)
{
    const Vec3 d = dst - src;
    const float hyp = std::sqrt(d.x * d.x + d.y * d.y);
    Vec3 a{};
    a.x = -std::atan2(d.z, hyp) * kRad2Deg;
    a.y = std::atan2(d.y, d.x) * kRad2Deg;
    normalize_angles(a);
    return a;
}

inline Vec3 angle_delta(const Vec3& from, const Vec3& to)
{
    Vec3 d{ to.x - from.x, to.y - from.y, 0.f };
    while (d.y > 180.f) d.y -= 360.f;
    while (d.y < -180.f) d.y += 360.f;
    d.x = clampf(d.x, -89.f, 89.f);
    return d;
}

inline float fov_degrees(const Vec3& from, const Vec3& to)
{
    const Vec3 d = angle_delta(from, to);
    return std::sqrt(d.x * d.x + d.y * d.y);
}
