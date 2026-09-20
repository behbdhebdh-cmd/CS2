#pragma once

#include "sdk/math.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <utility>

// Logical joints. Drawing never uses raw cache indices — CS2 models insert
// helper/twist/weapon bones, so cache slot 8 is not "left shoulder".
enum class Skel : int {
    Head = 0,
    Neck,
    SpineUpper,
    SpineLower,
    Pelvis,
    LShoulder,
    LElbow,
    LHand,
    RShoulder,
    RElbow,
    RHand,
    LHip,
    LKnee,
    LFoot,
    RHip,
    RKnee,
    RFoot,
    Count
};

inline constexpr int kSkelCount = static_cast<int>(Skel::Count);

struct SkelSeg {
    Skel a;
    Skel b;
    float min_len;
    float max_len;
};

inline constexpr SkelSeg kSkelSegs[] = {
    { Skel::Pelvis,     Skel::SpineLower,  2.f, 50.f },
    { Skel::SpineLower, Skel::SpineUpper,  2.f, 45.f },
    { Skel::SpineUpper, Skel::Neck,        2.f, 40.f },
    { Skel::Neck,       Skel::Head,        2.f, 28.f },
    { Skel::Neck,       Skel::LShoulder,   2.f, 50.f },
    { Skel::LShoulder,  Skel::LElbow,      4.f, 55.f },
    { Skel::LElbow,     Skel::LHand,       2.f, 50.f },
    { Skel::Neck,       Skel::RShoulder,   2.f, 50.f },
    { Skel::RShoulder,  Skel::RElbow,      4.f, 55.f },
    { Skel::RElbow,     Skel::RHand,       2.f, 50.f },
    { Skel::Pelvis,     Skel::LHip,        2.f, 50.f },
    { Skel::LHip,       Skel::LKnee,       4.f, 65.f },
    { Skel::LKnee,      Skel::LFoot,       2.f, 60.f },
    { Skel::Pelvis,     Skel::RHip,        2.f, 50.f },
    { Skel::RHip,       Skel::RKnee,       4.f, 65.f },
    { Skel::RKnee,      Skel::RFoot,       2.f, 60.f },
};

inline bool skel_pos_ok(const Vec3& p, const Vec3& origin)
{
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
        return false;
    if (std::fabs(p.x) < 0.001f && std::fabs(p.y) < 0.001f && std::fabs(p.z) < 0.001f)
        return false;
    return p.dist(origin) < 256.f;
}

inline bool skel_seg_ok(const Vec3& a, const Vec3& b, float mn, float mx)
{
    const float d = a.dist(b);
    return d >= mn && d <= mx;
}

// Cache-index maps for known CS2 player layouts. -1 = unused.
// Order matches Skel enum.
using SkelLayout = std::array<int, kSkelCount>;

inline constexpr SkelLayout kLayoutClassic{{
    6, 5, 4, 2, 0,
    8, 9, 10,
    13, 14, 15,
    22, 23, 24,
    25, 26, 27
}};

inline constexpr SkelLayout kLayoutAnim2026{{
    7, 6, 4, 2, 1,
    9, 10, 11,
    13, 14, 15,
    17, 18, 19,
    20, 21, 22
}};

inline constexpr SkelLayout kLayoutAnimClav{{
    7, 6, 4, 2, 1,
    8, 10, 11,
    12, 14, 15,
    17, 18, 19,
    20, 21, 22
}};

inline constexpr SkelLayout kLayoutLegs18{{
    6, 5, 4, 2, 0,
    8, 9, 10,
    13, 14, 15,
    18, 19, 20,
    21, 22, 23
}};

// CS2 player (origin at feet). Pelvis is NOT slot 0.
inline constexpr SkelLayout kLayoutFeetOrigin{{
    7, 6, 4, 3, 1,
    9, 10, 11,
    13, 14, 15,
    17, 18, 19,
    20, 21, 22
}};

inline int skel_score_layout(const Vec3* cache, int cache_n, const Vec3& origin, const SkelLayout& layout)
{
    Vec3 j[kSkelCount]{};
    bool ok[kSkelCount]{};
    int filled = 0;
    for (int i = 0; i < kSkelCount; ++i) {
        const int idx = layout[i];
        if (idx < 0 || idx >= cache_n)
            continue;
        if (!skel_pos_ok(cache[idx], origin))
            continue;
        j[i] = cache[idx];
        ok[i] = true;
        ++filled;
    }
    if (filled < 5)
        return -1000;

    int score = filled;
    for (const SkelSeg& s : kSkelSegs) {
        const int a = static_cast<int>(s.a);
        const int b = static_cast<int>(s.b);
        if (!ok[a] || !ok[b]) {
            score -= 8;
            continue;
        }
        const float d = j[a].dist(j[b]);
        if (d < s.min_len || d > s.max_len)
            score -= 40;
        else
            score += 12;
    }

    const int h = static_cast<int>(Skel::Head);
    const int n = static_cast<int>(Skel::Neck);
    const int su = static_cast<int>(Skel::SpineUpper);
    const int sl = static_cast<int>(Skel::SpineLower);
    const int p = static_cast<int>(Skel::Pelvis);
    if (ok[h] && ok[n] && ok[su] && ok[sl] && ok[p]) {
        if (j[h].z > j[n].z && j[n].z > j[su].z && j[su].z > j[sl].z && j[sl].z > j[p].z)
            score += 35;
        else
            score -= 30;
        // Origin is at the feet. A "pelvis" sitting on the origin is the root bone.
        if (j[p].z < origin.z + 18.f)
            score -= 80;
        else if (j[p].z < origin.z + 48.f)
            score += 20;
        if (j[h].z > origin.z + 48.f)
            score += 15;
    }

    const int lhp = static_cast<int>(Skel::LHip);
    const int lkn = static_cast<int>(Skel::LKnee);
    const int lft = static_cast<int>(Skel::LFoot);
    const int rhp = static_cast<int>(Skel::RHip);
    const int rkn = static_cast<int>(Skel::RKnee);
    const int rft = static_cast<int>(Skel::RFoot);
    auto leg_ok = [&](int hip, int knee, int foot) {
        if (ok[hip] && ok[knee] && j[knee].z > j[hip].z + 6.f)
            score -= 35;
        if (ok[knee] && ok[foot] && j[foot].z > j[knee].z + 6.f)
            score -= 35;
        if (ok[hip] && ok[foot] && j[foot].z < j[hip].z)
            score += 8;
    };
    leg_ok(lhp, lkn, lft);
    leg_ok(rhp, rkn, rft);

    const int ls = static_cast<int>(Skel::LShoulder);
    const int rs = static_cast<int>(Skel::RShoulder);
    if (ok[ls] && ok[rs]) {
        const float across = j[ls].dist(j[rs]);
        if (across > 6.f && across < 55.f)
            score += 12;
        else
            score -= 20;
    }
    return score;
}

inline int skel_closest(const Vec3* cache, const bool* ok, const bool* used, int n,
                        const Vec3& target, float min_d, float max_d, float min_z, float max_z)
{
    int best = -1;
    float best_d = 1e9f;
    for (int i = 0; i < n; ++i) {
        if (!ok[i] || used[i])
            continue;
        if (cache[i].z < min_z || cache[i].z > max_z)
            continue;
        const float d = cache[i].dist(target);
        if (d < min_d || d > max_d)
            continue;
        if (d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return best;
}

inline bool skel_resolve_geometric(const Vec3* cache, const bool* ok, int n, const Vec3& origin,
                                   Vec3* out, bool* out_ok)
{
    for (int i = 0; i < kSkelCount; ++i) {
        out[i] = {};
        out_ok[i] = false;
    }

    int head = -1;
    float head_z = -1e9f;
    for (int i = 0; i < n; ++i) {
        if (!ok[i])
            continue;
        const float xy = std::sqrt((cache[i].x - origin.x) * (cache[i].x - origin.x) +
                                   (cache[i].y - origin.y) * (cache[i].y - origin.y));
        if (xy > 28.f)
            continue;
        if (cache[i].z > head_z) {
            head_z = cache[i].z;
            head = i;
        }
    }
    int pelvis = -1;
    float pelvis_d = 1e9f;
    for (int i = 0; i < n; ++i) {
        if (!ok[i] || i == head)
            continue;
        if (cache[i].z < origin.z + 20.f || cache[i].z > origin.z + 48.f)
            continue;
        const float xy = std::sqrt((cache[i].x - origin.x) * (cache[i].x - origin.x) +
                                   (cache[i].y - origin.y) * (cache[i].y - origin.y));
        if (xy > 16.f)
            continue;
        const float d = cache[i].dist(origin);
        if (d < pelvis_d) {
            pelvis_d = d;
            pelvis = i;
        }
    }
    if (head < 0 || pelvis < 0 || cache[head].z - cache[pelvis].z < 36.f)
        return false;

    bool used[128]{};
    if (n > 128)
        return false;
    used[head] = used[pelvis] = true;

    const Vec3 axis = cache[head] - cache[pelvis];
    auto along = [&](float t) { return cache[pelvis] + axis * t; };

    const int neck = skel_closest(cache, ok, used, n, along(0.82f), 1.f, 22.f,
                                  cache[pelvis].z + 20.f, cache[head].z - 2.f);
    if (neck < 0)
        return false;
    used[neck] = true;
    const int spine_u = skel_closest(cache, ok, used, n, along(0.55f), 1.f, 22.f,
                                     cache[pelvis].z + 8.f, cache[neck].z);
    const int spine_l = skel_closest(cache, ok, used, n, along(0.28f), 1.f, 22.f,
                                     cache[pelvis].z, cache[neck].z);
    if (spine_u >= 0) used[spine_u] = true;
    if (spine_l >= 0) used[spine_l] = true;

    auto setj = [&](Skel j, int idx) {
        if (idx < 0)
            return;
        out[static_cast<int>(j)] = cache[idx];
        out_ok[static_cast<int>(j)] = true;
        used[idx] = true;
    };
    setj(Skel::Head, head);
    setj(Skel::Neck, neck);
    setj(Skel::SpineUpper, spine_u);
    setj(Skel::SpineLower, spine_l);
    setj(Skel::Pelvis, pelvis);

    int sh[2] = { -1, -1 };
    float sh_spread = -1.f;
    for (int i = 0; i < n; ++i) {
        if (!ok[i] || used[i])
            continue;
        const float dn = cache[i].dist(cache[neck]);
        if (dn < 8.f || dn > 36.f)
            continue;
        if (cache[i].z < cache[neck].z - 16.f || cache[i].z > cache[neck].z + 10.f)
            continue;
        for (int k = i + 1; k < n; ++k) {
            if (!ok[k] || used[k])
                continue;
            const float dn2 = cache[k].dist(cache[neck]);
            if (dn2 < 8.f || dn2 > 36.f)
                continue;
            if (cache[k].z < cache[neck].z - 16.f || cache[k].z > cache[neck].z + 10.f)
                continue;
            const float spread = cache[i].dist(cache[k]);
            if (spread > 8.f && spread < 52.f && spread > sh_spread) {
                sh_spread = spread;
                sh[0] = i;
                sh[1] = k;
            }
        }
    }
    if (sh[0] < 0)
        return out_ok[static_cast<int>(Skel::Head)] && out_ok[static_cast<int>(Skel::Pelvis)];

    if (cache[sh[0]].x + cache[sh[0]].y > cache[sh[1]].x + cache[sh[1]].y)
        std::swap(sh[0], sh[1]);

    auto chain_arm = [&](int shoulder, Skel es, Skel ee, Skel eh) {
        setj(es, shoulder);
        const Vec3 away = cache[shoulder] * 2.f - cache[neck];
        const int elbow = skel_closest(cache, ok, used, n, away, 8.f, 42.f,
                                       origin.z + 8.f, cache[shoulder].z + 8.f);
        if (elbow < 0)
            return;
        setj(ee, elbow);
        const Vec3 hand_hint = cache[elbow] * 2.f - cache[shoulder];
        const int hand = skel_closest(cache, ok, used, n, hand_hint, 6.f, 38.f,
                                      origin.z, cache[elbow].z + 10.f);
        if (hand >= 0)
            setj(eh, hand);
    };
    chain_arm(sh[0], Skel::LShoulder, Skel::LElbow, Skel::LHand);
    chain_arm(sh[1], Skel::RShoulder, Skel::RElbow, Skel::RHand);

    int hip[2] = { -1, -1 };
    float hip_spread = -1.f;
    for (int i = 0; i < n; ++i) {
        if (!ok[i] || used[i])
            continue;
        const float dp = cache[i].dist(cache[pelvis]);
        if (dp < 4.f || dp > 28.f)
            continue;
        if (cache[i].z > cache[pelvis].z + 8.f)
            continue;
        for (int k = i + 1; k < n; ++k) {
            if (!ok[k] || used[k])
                continue;
            const float dp2 = cache[k].dist(cache[pelvis]);
            if (dp2 < 4.f || dp2 > 28.f)
                continue;
            if (cache[k].z > cache[pelvis].z + 8.f)
                continue;
            const float spread = cache[i].dist(cache[k]);
            if (spread > 4.f && spread < 36.f && spread > hip_spread) {
                hip_spread = spread;
                hip[0] = i;
                hip[1] = k;
            }
        }
    }
    if (hip[0] >= 0) {
        if (cache[hip[0]].x + cache[hip[0]].y > cache[hip[1]].x + cache[hip[1]].y)
            std::swap(hip[0], hip[1]);
        auto chain_leg = [&](int start, Skel hs, Skel kn, Skel ft) {
            setj(hs, start);
            const int knee = skel_closest(cache, ok, used, n, cache[start] + Vec3{ 0, 0, -20.f },
                                          8.f, 48.f, origin.z - 8.f, cache[start].z);
            if (knee < 0)
                return;
            setj(kn, knee);
            const int foot = skel_closest(cache, ok, used, n, cache[knee] + Vec3{ 0, 0, -20.f },
                                          6.f, 42.f, origin.z - 12.f, cache[knee].z);
            if (foot >= 0)
                setj(ft, foot);
        };
        chain_leg(hip[0], Skel::LHip, Skel::LKnee, Skel::LFoot);
        chain_leg(hip[1], Skel::RHip, Skel::RKnee, Skel::RFoot);
    }
    return out_ok[static_cast<int>(Skel::Head)] && out_ok[static_cast<int>(Skel::Pelvis)];
}

inline int skel_apply_layout(const Vec3* cache, int cache_n, const Vec3& origin, const SkelLayout& layout,
                             Vec3* out, bool* out_ok)
{
    int n = 0;
    for (int i = 0; i < kSkelCount; ++i) {
        out[i] = {};
        out_ok[i] = false;
        const int idx = layout[i];
        if (idx < 0 || idx >= cache_n)
            continue;
        if (!skel_pos_ok(cache[idx], origin))
            continue;
        out[i] = cache[idx];
        out_ok[i] = true;
        ++n;
    }
    return n;
}

inline const char* skel_resolve(const Vec3* cache, int cache_n, const Vec3& origin, Vec3* out, std::uint32_t& mask)
{
    bool ok[96]{};
    const int n = cache_n < 96 ? cache_n : 96;
    Vec3 tmp[96];
    for (int i = 0; i < n; ++i) {
        tmp[i] = cache[i];
        ok[i] = skel_pos_ok(cache[i], origin);
    }

    struct Cand {
        const char* name;
        int score;
        Vec3 j[kSkelCount];
        bool o[kSkelCount];
    };
    Cand best{ "none", -10000, {}, {} };

    auto consider = [&](const char* name, const Vec3* j, const bool* o, int score) {
        if (score > best.score) {
            best.name = name;
            best.score = score;
            for (int i = 0; i < kSkelCount; ++i) {
                best.j[i] = j[i];
                best.o[i] = o[i];
            }
        }
    };

    auto score_joints = [&](const Vec3* j, const bool* o) {
        SkelLayout dummy{};
        Vec3 packed[96]{};
        for (int i = 0; i < kSkelCount; ++i) {
            packed[i] = j[i];
            dummy[i] = o[i] ? i : -1;
        }
        return skel_score_layout(packed, kSkelCount, origin, dummy);
    };

    const SkelLayout* layouts[] = {
        &kLayoutFeetOrigin, &kLayoutAnim2026, &kLayoutAnimClav, &kLayoutClassic, &kLayoutLegs18
    };
    const char* names[] = { "feet_origin", "anim2026", "anim_clav", "classic28", "legs18" };
    for (int li = 0; li < 5; ++li) {
        Vec3 j[kSkelCount]{};
        bool o[kSkelCount]{};
        skel_apply_layout(tmp, n, origin, *layouts[li], j, o);
        consider(names[li], j, o, skel_score_layout(tmp, n, origin, *layouts[li]));
    }

    Vec3 gj[kSkelCount]{};
    bool go[kSkelCount]{};
    if (skel_resolve_geometric(tmp, ok, n, origin, gj, go))
        consider("geometric", gj, go, score_joints(gj, go) + 8);

    mask = 0;
    for (int i = 0; i < kSkelCount; ++i) {
        out[i] = {};
        if (!best.o[i] || best.score < 20)
            continue;
        // Drop joints that break a required parent segment.
        out[i] = best.j[i];
        mask |= (1u << i);
    }
    for (const SkelSeg& s : kSkelSegs) {
        const int a = static_cast<int>(s.a);
        const int b = static_cast<int>(s.b);
        if ((mask & (1u << a)) && (mask & (1u << b))) {
            if (!skel_seg_ok(out[a], out[b], s.min_len, s.max_len))
                mask &= ~(1u << b);
        }
    }
    return best.name;
}
