#include "sdk/vis.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <utility>

namespace {
constexpr float kEps = 1e-7f;

Vec3 vmin(const Vec3& a, const Vec3& b) { return { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) }; }
Vec3 vmax(const Vec3& a, const Vec3& b) { return { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) }; }
}

bool VisMesh::load_file(const std::string& path)
{
    ready_ = false;
    loaded_ = false;
    tris_.clear();
    nodes_.clear();

    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
        return false;

    const auto bytes = static_cast<size_t>(in.tellg());
    if (bytes < 36 || bytes % 36 != 0)
        return false;

    const size_t count = bytes / 36;
    tris_.resize(count);
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(tris_.data()), static_cast<std::streamsize>(bytes));
    loaded_ = in.good();
    return loaded_;
}

void VisMesh::build()
{
    nodes_.clear();
    order_.clear();
    ready_ = false;
    if (tris_.empty())
        return;

    std::vector<int> idx(tris_.size());
    for (int i = 0; i < static_cast<int>(idx.size()); ++i)
        idx[i] = i;

    nodes_.reserve(tris_.size() * 2);
    order_.reserve(tris_.size());
    build_node(idx, 0, static_cast<int>(idx.size()), 0);
    ready_ = !nodes_.empty();
}

int VisMesh::build_node(std::vector<int>& idx, int begin, int end, int depth)
{
    Node node{};
    node.mn = { 1e12f, 1e12f, 1e12f };
    node.mx = { -1e12f, -1e12f, -1e12f };

    for (int i = begin; i < end; ++i) {
        const Tri& t = tris_[idx[i]];
        node.mn = vmin(node.mn, vmin(t.a, vmin(t.b, t.c)));
        node.mx = vmax(node.mx, vmax(t.a, vmax(t.b, t.c)));
    }

    const int count = end - begin;
    const int self = static_cast<int>(nodes_.size());
    nodes_.push_back(node);

    if (count <= 8 || depth > 22) {
        nodes_[self].left = -1;
        nodes_[self].right = -1;
        nodes_[self].start = static_cast<int>(order_.size());
        nodes_[self].count = count;
        for (int i = begin; i < end; ++i)
            order_.push_back(idx[i]);
        return self;
    }

    const Vec3 ext = node.mx - node.mn;
    int axis = 0;
    if (ext.y > ext.x) axis = 1;
    if (ext.z > (axis == 0 ? ext.x : ext.y)) axis = 2;

    const int mid = begin + count / 2;
    std::nth_element(idx.begin() + begin, idx.begin() + mid, idx.begin() + end,
        [&](int a, int b) {
            const Tri& ta = tris_[a];
            const Tri& tb = tris_[b];
            const float ca = (axis == 0) ? (ta.a.x + ta.b.x + ta.c.x)
                           : (axis == 1) ? (ta.a.y + ta.b.y + ta.c.y)
                                         : (ta.a.z + ta.b.z + ta.c.z);
            const float cb = (axis == 0) ? (tb.a.x + tb.b.x + tb.c.x)
                           : (axis == 1) ? (tb.a.y + tb.b.y + tb.c.y)
                                         : (tb.a.z + tb.b.z + tb.c.z);
            return ca < cb;
        });

    const int L = build_node(idx, begin, mid, depth + 1);
    const int R = build_node(idx, mid, end, depth + 1);
    nodes_[self].left = L;
    nodes_[self].right = R;
    nodes_[self].start = 0;
    nodes_[self].count = 0;
    return self;
}

bool VisMesh::ray_aabb(const Vec3& o, const Vec3& inv, float tmax, const Vec3& mn, const Vec3& mx) const
{
    float tmin = 0.f;
    const float tx1 = (mn.x - o.x) * inv.x;
    const float tx2 = (mx.x - o.x) * inv.x;
    tmin = std::max(tmin, std::min(tx1, tx2));
    tmax = std::min(tmax, std::max(tx1, tx2));
    const float ty1 = (mn.y - o.y) * inv.y;
    const float ty2 = (mx.y - o.y) * inv.y;
    tmin = std::max(tmin, std::min(ty1, ty2));
    tmax = std::min(tmax, std::max(ty1, ty2));
    const float tz1 = (mn.z - o.z) * inv.z;
    const float tz2 = (mx.z - o.z) * inv.z;
    tmin = std::max(tmin, std::min(tz1, tz2));
    tmax = std::min(tmax, std::max(tz1, tz2));
    return tmax >= tmin;
}

bool VisMesh::ray_tri(const Vec3& o, const Vec3& d, float tmax, const Tri& t, float& hit) const
{
    const Vec3 e1 = t.b - t.a;
    const Vec3 e2 = t.c - t.a;
    const Vec3 p{
        d.y * e2.z - d.z * e2.y,
        d.z * e2.x - d.x * e2.z,
        d.x * e2.y - d.y * e2.x
    };
    const float det = e1.x * p.x + e1.y * p.y + e1.z * p.z;
    if (std::fabs(det) < kEps)
        return false;
    const float inv = 1.f / det;
    const Vec3 s = o - t.a;
    const float u = (s.x * p.x + s.y * p.y + s.z * p.z) * inv;
    if (u < 0.f || u > 1.f)
        return false;
    const Vec3 q{
        s.y * e1.z - s.z * e1.y,
        s.z * e1.x - s.x * e1.z,
        s.x * e1.y - s.y * e1.x
    };
    const float v = (d.x * q.x + d.y * q.y + d.z * q.z) * inv;
    if (v < 0.f || u + v > 1.f)
        return false;
    const float dist = (e2.x * q.x + e2.y * q.y + e2.z * q.z) * inv;
    if (dist <= 0.05f || dist >= tmax)
        return false;
    hit = dist;
    return true;
}

bool VisMesh::trace(const Vec3& o, const Vec3& d, float tmax) const
{
    if (nodes_.empty())
        return false;

    const Vec3 inv{
        (std::fabs(d.x) < kEps) ? 1e12f : 1.f / d.x,
        (std::fabs(d.y) < kEps) ? 1e12f : 1.f / d.y,
        (std::fabs(d.z) < kEps) ? 1e12f : 1.f / d.z
    };

    int stack[64];
    int sp = 0;
    stack[sp++] = 0;

    while (sp) {
        const Node& n = nodes_[stack[--sp]];
        if (!ray_aabb(o, inv, tmax, n.mn, n.mx))
            continue;
        if (n.left < 0) {
            for (int i = 0; i < n.count; ++i) {
                float hit = 0.f;
                if (ray_tri(o, d, tmax, tris_[order_[n.start + i]], hit))
                    return true;
            }
        } else {
            if (n.right >= 0 && sp < 63) stack[sp++] = n.right;
            if (n.left >= 0 && sp < 63) stack[sp++] = n.left;
        }
    }
    return false;
}

bool VisMesh::visible(const Vec3& origin, const Vec3& target) const
{
    if (!ready_)
        return true;
    const Vec3 d = target - origin;
    const float len = d.length();
    if (len < 8.f)
        return true;
    const Vec3 dir = d * (1.f / len);
    const float tmax = len - 6.f;
    return !trace(origin, dir, tmax);
}

void VisCheck::tick(const std::string& map_name)
{
    std::string key = map_name;
    if (key.size() > 1 && key[0] == '<')
        key = key.substr(1);
    if (!key.empty() && key.back() == '>')
        key.pop_back();
    const auto slash = key.find_last_of("/\\");
    if (slash != std::string::npos)
        key = key.substr(slash + 1);
    if (key.rfind("maps/", 0) == 0)
        key = key.substr(5);

    if (key.empty() || key == "<empty>" || key == "unconnected")
        return;
    if (key == map_ && mesh_.loaded())
        return;

    map_ = key;
    mesh_ = VisMesh{};

    const std::string candidates[] = {
        dir_ + "\\tri\\" + key + ".tri",
        dir_ + "\\maps\\tri\\" + key + ".tri",
        dir_ + "\\" + key + ".tri",
        std::string("D:\\CS2\\maps\\tri\\") + key + ".tri",
        std::string("D:\\CS2\\maps\\") + key + ".tri",
    };

    for (const auto& p : candidates) {
        if (mesh_.load_file(p)) {
            mesh_.build();
            return;
        }
    }
}

bool VisCheck::visible(const Vec3& from, const Vec3& to) const
{
    if (!mesh_.ready())
        return true;
    return mesh_.visible(from, to);
}
