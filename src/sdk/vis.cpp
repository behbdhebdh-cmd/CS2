#include "sdk/vis.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <utility>

namespace {
constexpr float kEps = 1e-7f;
constexpr float kNearSkip = 1.6f;
constexpr float kFarPad = 5.f;
constexpr float kAabbPad = 0.08f;

Vec3 vmin(const Vec3& a, const Vec3& b) { return { std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z) }; }
Vec3 vmax(const Vec3& a, const Vec3& b) { return { std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z) }; }

bool dir_has_tri(const std::string& dir)
{
    if (dir.empty())
        return false;
    WIN32_FIND_DATAA fd{};
    const std::string pat = dir + "\\*.tri";
    HANDLE h = FindFirstFileA(pat.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    FindClose(h);
    return true;
}

std::string join_path(const std::string& a, const std::string& b)
{
    if (a.empty())
        return b;
    if (a.back() == '\\' || a.back() == '/')
        return a + b;
    return a + "\\" + b;
}

std::string exe_dir()
{
    char buf[MAX_PATH]{};
    const DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (!n)
        return {};
    std::string p(buf, buf + n);
    const auto slash = p.find_last_of("\\/");
    if (slash == std::string::npos)
        return {};
    return p.substr(0, slash);
}

std::string cwd_dir()
{
    char buf[MAX_PATH]{};
    const DWORD n = GetCurrentDirectoryA(MAX_PATH, buf);
    if (!n)
        return {};
    return std::string(buf, buf + n);
}

bool file_ok(const std::string& path)
{
    const DWORD a = GetFileAttributesA(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

std::string find_maps_root(const std::string& hint)
{
    const std::string exe = exe_dir();
    const std::string cwd = cwd_dir();
    const std::string cands[] = {
        hint,
        join_path(hint, "maps"),
        "D:\\CS2\\maps",
        join_path(exe, "maps"),
        join_path(exe, "..\\maps"),
        join_path(cwd, "maps"),
        join_path(cwd, "..\\maps"),
    };

    for (const auto& c : cands) {
        if (c.empty())
            continue;
        if (dir_has_tri(join_path(c, "tri")))
            return c;
        if (dir_has_tri(c))
            return c;
    }
    return hint.empty() ? std::string("D:\\CS2\\maps") : hint;
}

std::string ascii_lower(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
}

std::string sanitize_map_name(const std::string& raw)
{
    std::string s = raw;
    while (!s.empty() && (unsigned char)s.front() <= 32)
        s.erase(s.begin());
    while (!s.empty() && (unsigned char)s.back() <= 32)
        s.pop_back();
    while (!s.empty() && (s.front() == '<' || s.front() == '"' || s.front() == '\''))
        s.erase(s.begin());
    while (!s.empty() && (s.back() == '>' || s.back() == '"' || s.back() == '\''))
        s.pop_back();
    for (char& c : s) {
        if (c == '/')
            c = '\\';
    }

    auto slash = s.find_last_of('\\');
    if (slash != std::string::npos)
        s = s.substr(slash + 1);

    s = ascii_lower(std::move(s));
    if (s.rfind("maps\\", 0) == 0)
        s = s.substr(5);

    while (true) {
        const auto dot = s.rfind('.');
        if (dot == std::string::npos || dot == 0)
            break;
        const std::string ext = s.substr(dot);
        if (ext == ".vpk" || ext == ".bsp" || ext == ".tri" || ext == ".vmdl" || ext == ".vmdl_c")
            s.resize(dot);
        else
            break;
    }

    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
            out.push_back(c);
    }

    if (out.empty() || out == "empty" || out == "unconnected" || out == "none" || out == "invalid")
        return {};
    return out;
}

bool VisMesh::load_file(const std::string& path)
{
    ready_ = false;
    loaded_ = false;
    tris_.clear();
    nodes_.clear();
    order_.clear();

    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in)
        return false;

    const auto pos = in.tellg();
    if (pos < 36)
        return false;

    const size_t bytes = static_cast<size_t>(pos);
    if (bytes % 36 != 0)
        return false;

    const size_t count = bytes / 36;
    std::vector<Tri> raw(count);
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(bytes));
    if (static_cast<size_t>(in.gcount()) != bytes)
        return false;

    tris_.reserve(count);
    for (const Tri& t : raw) {
        const Vec3 e1 = t.b - t.a;
        const Vec3 e2 = t.c - t.a;
        const float cx = e1.y * e2.z - e1.z * e2.y;
        const float cy = e1.z * e2.x - e1.x * e2.z;
        const float cz = e1.x * e2.y - e1.y * e2.x;
        if (!std::isfinite(t.a.x) || !std::isfinite(t.b.x) || !std::isfinite(t.c.x))
            continue;
        if ((cx * cx + cy * cy + cz * cz) < 1e-6f)
            continue;
        tris_.push_back(t);
    }

    loaded_ = !tris_.empty();
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
    node.mn = node.mn - Vec3{ kAabbPad, kAabbPad, kAabbPad };
    node.mx = node.mx + Vec3{ kAabbPad, kAabbPad, kAabbPad };

    const int count = end - begin;
    const int self = static_cast<int>(nodes_.size());
    nodes_.push_back(node);

    if (count <= 8 || depth > 28) {
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
    if (dist <= kNearSkip || dist >= tmax)
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
    const float tmax = std::max(kNearSkip + 1.f, len - kFarPad);
    return !trace(origin, dir, tmax);
}

VisCheck::~VisCheck()
{
    gen_.fetch_add(1, std::memory_order_acq_rel);
    join_worker();
}

void VisCheck::join_worker()
{
    if (worker_.joinable())
        worker_.join();
}

bool VisCheck::has_map() const
{
    std::lock_guard<std::mutex> lock(mu_);
    return !map_.empty() && map_ != "<none>";
}

std::string VisCheck::map() const
{
    std::lock_guard<std::mutex> lock(mu_);
    return map_;
}

std::string VisCheck::status() const
{
    std::lock_guard<std::mutex> lock(mu_);
    return status_;
}

size_t VisCheck::triangles() const
{
    auto mesh = [&] {
        std::lock_guard<std::mutex> lock(mu_);
        return live_;
    }();
    return mesh ? mesh->triangle_count() : 0;
}

std::string VisCheck::resolve_tri_path(const std::string& key) const
{
    const std::string root = find_maps_root(dir_);
    const std::string names[] = {
        key + ".tri",
        ascii_lower(key) + ".tri",
    };

    const std::string folders[] = {
        join_path(root, "tri"),
        root,
        join_path(root, "maps\\tri"),
        "D:\\CS2\\maps\\tri",
        "D:\\CS2\\maps",
    };

    for (const auto& folder : folders) {
        for (const auto& name : names) {
            const std::string p = join_path(folder, name);
            if (file_ok(p))
                return p;
        }
    }
    return {};
}

void VisCheck::start_load(const std::string& key)
{
    if (loading_.load(std::memory_order_acquire))
        return;

    join_worker();

    const uint64_t gen = gen_.fetch_add(1, std::memory_order_acq_rel) + 1;
    loading_.store(true, std::memory_order_release);
    ready_.store(false, std::memory_order_release);

    {
        std::lock_guard<std::mutex> lock(mu_);
        wanted_ = key;
        status_ = "loading " + key;
        live_.reset();
        map_ = key;
    }

    worker_ = std::thread([this, key, gen]() {
        std::string status;
        auto mesh = std::make_shared<VisMesh>();
        bool ok = false;

        try {
            const std::string path = this->resolve_tri_path(key);

            if (path.empty()) {
                status = "missing " + key + ".tri";
            } else if (!mesh->load_file(path)) {
                status = "bad " + key + ".tri";
            } else {
                mesh->build();
                if (mesh->ready()) {
                    ok = true;
                    status = key;
                } else {
                    status = "empty " + key;
                }
            }
        } catch (...) {
            status = "fail " + key;
            ok = false;
        }

        if (gen_.load(std::memory_order_acquire) != gen) {
            loading_.store(false, std::memory_order_release);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mu_);
            if (wanted_ == key) {
                live_ = ok ? std::shared_ptr<const VisMesh>(std::move(mesh)) : nullptr;
                map_ = key;
                status_ = std::move(status);
                failed_key_ = ok ? std::string{} : key;
            }
        }
        ready_.store(ok, std::memory_order_release);
        loading_.store(false, std::memory_order_release);
    });
}

void VisCheck::tick(const std::string& map_name)
{
    const std::string key = sanitize_map_name(map_name);
    if (key.empty())
        return;

    if (loading_.load(std::memory_order_acquire)) {
        std::lock_guard<std::mutex> lock(mu_);
        wanted_ = key;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mu_);
        if (key == map_ && (live_ || failed_key_ == key))
            return;
        if (failed_key_ == key && !live_)
            return;
    }

    start_load(key);
}

bool VisCheck::visible(const Vec3& from, const Vec3& to) const
{
    std::shared_ptr<const VisMesh> mesh;
    {
        std::lock_guard<std::mutex> lock(mu_);
        mesh = live_;
    }
    if (!mesh || !mesh->ready())
        return true;
    return mesh->visible(from, to);
}
