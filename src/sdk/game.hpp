#pragma once

#include "sdk/math.hpp"
#include "sdk/memory.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct Player {
    uintptr_t controller = 0;
    uintptr_t pawn = 0;
    Vec3 origin{};
    Vec3 head{};
    Vec3 eye{};
    Vec3 mins{};
    Vec3 maxs{};
    std::array<Vec3, 8> corners{};
    int health = 0;
    int max_health = 100;
    int team = 0;
    float distance = 0.f;
    float speed = 0.f;
    bool ducked = false;
};

class Game {
public:
    bool tick();
    bool attached() const { return attached_; }
    uint32_t build_number() const { return build_number_; }
    const Mat4x4& view_matrix() const { return view_; }
    const std::vector<Player>& players() const { return players_; }
    int local_team() const { return local_team_; }
    const Vec3& local_origin() const { return local_origin_; }
    const Vec3& local_head() const { return local_head_; }
    const std::string& map_name() const { return map_name_; }
    const std::string& local_name() const { return local_name_; }
    int local_ping() const { return local_ping_; }
    int fps() const { return fps_; }

private:
    uintptr_t entity_by_index(int index) const;
    uintptr_t pawn_from_handle(uint32_t handle) const;
    void detect_map();

    Memory mem_;
    uintptr_t client_ = 0;
    uintptr_t engine_ = 0;
    uintptr_t entity_list_ = 0;
    bool attached_ = false;
    uint32_t build_number_ = 0;
    int local_team_ = 0;
    Vec3 local_origin_{};
    Vec3 local_head_{};
    Mat4x4 view_{};
    std::vector<Player> players_;
    std::string map_name_;
    std::string local_name_;
    int local_ping_ = 0;
    int fps_ = 0;
    float fps_smooth_ = 0.f;
    int fps_last_frames_ = 0;
    DWORD fps_last_ms_ = 0;
    DWORD last_attach_ms_ = 0;
    DWORD last_map_ms_ = 0;
};
