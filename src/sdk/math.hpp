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
