#pragma once

#include "sdk/math.hpp"
#include "sdk/memory.hpp"
#include "sdk/skeleton.hpp"

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
    std::array<Vec3, kSkelCount> joints{};
    std::uint32_t joint_mask = 0;
    int health = 0;
    int max_health = 100;
    int team = 0;
    float distance = 0.f;
    float speed = 0.f;
    bool ducked = false;
    std::string name;
    std::string weapon_class;
    uint16_t weapon_def_index = 0;
    std::string weapon_icon_utf8;

    bool has_joint(Skel j) const
    {
        const int i = static_cast<int>(j);
        return i >= 0 && i < kSkelCount && (joint_mask & (1u << i)) != 0;
    }
};

class Game {
public:
    bool tick(bool read_bones = false);
    bool attached() const { return attached_; }
    uint32_t build_number() const { return build_number_; }
    const Mat4x4& view_matrix() const { return view_; }
    const std::vector<Player>& players() const { return players_; }
    const std::vector<std::string>& spectators() const { return spectators_; }
    uintptr_t local_pawn() const { return local_pawn_; }
    int local_team() const { return local_team_; }
    const Vec3& local_origin() const { return local_origin_; }
    const Vec3& local_head() const { return local_head_; }
    const std::string& map_name() const { return map_name_; }
    const std::string& local_name() const { return local_name_; }
    int local_ping() const { return local_ping_; }
    int fps() const { return fps_; }
    const Vec3& view_angles() const { return view_angles_; }
    const Vec3& punch_angles() const { return punch_angles_; }
    float sensitivity() const { return sensitivity_; }
    float flash_alpha() const { return flash_alpha_; }
    int shots_fired() const { return shots_fired_; }
    int weapon_def() const { return weapon_def_; }
    int camera_fov() const { return camera_fov_; }
    bool scoped() const { return scoped_; }
    bool local_alive() const { return local_alive_; }

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
    std::vector<std::string> spectators_;
    uintptr_t local_pawn_ = 0;
    std::string map_name_;
    std::string local_name_;
    int local_ping_ = 0;
    int fps_ = 0;
    Vec3 view_angles_{};
    Vec3 punch_angles_{};
    float sensitivity_ = 1.f;
    float flash_alpha_ = 0.f;
    int shots_fired_ = 0;
    int weapon_def_ = 0;
    int camera_fov_ = 90;
    bool scoped_ = false;
    bool local_alive_ = false;
    float fps_smooth_ = 0.f;
    int fps_last_frames_ = 0;
    DWORD fps_last_ms_ = 0;
    DWORD last_attach_ms_ = 0;
    DWORD last_map_ms_ = 0;
};
