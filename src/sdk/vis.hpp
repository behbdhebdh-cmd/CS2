#pragma once

#include "sdk/math.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::string sanitize_map_name(const std::string& raw);

class VisMesh {
public:
    bool load_file(const std::string& path);
    void build();
    bool ready() const { return ready_; }
    bool loaded() const { return loaded_; }
    size_t triangle_count() const { return tris_.size(); }

    // true if the segment origin->target is free of world collision
    bool visible(const Vec3& origin, const Vec3& target) const;

private:
    struct Tri { Vec3 a, b, c; };
    struct Node {
        Vec3 mn, mx;
        int left = -1;
        int right = -1;
        int start = 0;
        int count = 0;
    };

    int build_node(std::vector<int>& idx, int begin, int end, int depth);
    bool ray_aabb(const Vec3& o, const Vec3& inv, float tmax, const Vec3& mn, const Vec3& mx) const;
    bool ray_tri(const Vec3& o, const Vec3& d, float tmax, const Tri& t, float& hit) const;
    bool trace(const Vec3& o, const Vec3& d, float tmax) const;

    std::vector<Tri> tris_;
    std::vector<Node> nodes_;
    std::vector<int> order_;
    bool loaded_ = false;
    bool ready_ = false;
};

class VisCheck {
public:
    VisCheck() = default;
    ~VisCheck();

    VisCheck(const VisCheck&) = delete;
    VisCheck& operator=(const VisCheck&) = delete;

    void set_search_dir(const std::string& dir) { dir_ = dir; }
    void tick(const std::string& map_name);
    bool ready() const { return ready_.load(std::memory_order_acquire); }
    bool loading() const { return loading_.load(std::memory_order_acquire); }
    bool has_map() const;
    std::string map() const;
    std::string status() const;
    size_t triangles() const;
    bool visible(const Vec3& from, const Vec3& to) const;

private:
    void join_worker();
    void start_load(const std::string& key);
    std::string resolve_tri_path(const std::string& key) const;

    std::string dir_;
    std::string map_;
    std::string wanted_;
    std::string status_{ "idle" };
    std::string failed_key_;
    std::shared_ptr<const VisMesh> live_;
    std::thread worker_;
    mutable std::mutex mu_;
    std::atomic<bool> ready_{ false };
    std::atomic<bool> loading_{ false };
    std::atomic<uint64_t> gen_{ 0 };
};
