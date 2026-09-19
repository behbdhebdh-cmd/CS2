#pragma once

#include "sdk/math.hpp"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

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
    void set_search_dir(const std::string& dir) { dir_ = dir; }
    void tick(const std::string& map_name);
    bool ready() const { return mesh_.ready(); }
    bool has_map() const { return !map_.empty() && map_ != "<none>"; }
    const std::string& map() const { return map_; }
    size_t triangles() const { return mesh_.triangle_count(); }
    bool visible(const Vec3& from, const Vec3& to) const;

private:
    std::string dir_;
    std::string map_;
    std::string status_;
    VisMesh mesh_;
};
