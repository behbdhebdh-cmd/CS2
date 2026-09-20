#include "sdk/game.hpp"
#include "sdk/offsets.hpp"
#include "sdk/skel_log.hpp"
#include "sdk/skeleton.hpp"
#include "sdk/vis.hpp"

#include <Windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace {

struct BoneEntry {
    Vec3 position{};
    std::uint8_t padding[0x14]{};
};

static_assert(sizeof(BoneEntry) == offsets::skeleton::kBoneStride);

bool valid_bone_position(const Vec3& position, const Vec3& origin)
{
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
        return false;
    if (std::fabs(position.x) < 0.001f && std::fabs(position.y) < 0.001f && std::fabs(position.z) < 0.001f)
        return false;
    return position.dist(origin) < 256.f;
}

bool plausible_pointer(uintptr_t p)
{
    return p >= 0x10000 && (p & 7ull) == 0;
}

uintptr_t bone_array_from_skeleton(const Memory& mem, uintptr_t skeleton)
{
    if (!plausible_pointer(skeleton))
        return 0;
    const uintptr_t array = mem.read<uintptr_t>(
        skeleton + offsets::schema::CSkeletonInstance::m_modelState
                 + offsets::schema::CModelState::m_boneArray);
    return plausible_pointer(array) ? array : 0;
}

} // namespace

uintptr_t Game::entity_by_index(int index) const
{
    if (!entity_list_ || index <= 0)
        return 0;

    const int clamped = index & static_cast<int>(offsets::entity_system::kHandleIndexMask);
    const uintptr_t chunk = mem_.read<uintptr_t>(
        entity_list_ + offsets::entity_system::kListOffset + 8ull * (static_cast<unsigned>(clamped) >> 9));
    if (!chunk)
        return 0;

    return mem_.read<uintptr_t>(
        chunk + offsets::entity_system::kEntryStride * (clamped & 0x1FF));
}

void Game::detect_map()
{
    const uintptr_t gv = mem_.read<uintptr_t>(client_ + offsets::client::dwGlobalVars);
    if (!gv)
        return;

    uint8_t blob[0x280]{};
    if (!mem_.read_raw(gv, blob, sizeof(blob)))
        return;

    auto looks_like_map = [](const char* s) -> bool {
        if (!s || !s[0] || s[0] < 32)
            return false;
        if (std::strncmp(s, "maps/", 5) == 0)
            return true;
        if (std::strncmp(s, "de_", 3) == 0 || std::strncmp(s, "cs_", 3) == 0 || std::strncmp(s, "ar_", 3) == 0)
            return true;
        if (s[0] == '<' && (std::strstr(s, "de_") || std::strstr(s, "cs_")))
            return true;
        return false;
    };

    const std::ptrdiff_t prefer[] = { 0x180, 0x188, 0x190, 0x1B8, 0x1C0, 0x218, 0x228, 0x230, 0x238 };
    char name[96]{};

    auto take = [&](uintptr_t p) -> bool {
        if (!p || p < 0x10000)
            return false;
        std::memset(name, 0, sizeof(name));
        if (!mem_.read_raw(p, name, sizeof(name) - 1))
            return false;
        name[sizeof(name) - 1] = 0;
        if (!looks_like_map(name))
            return false;
        std::string cleaned = sanitize_map_name(name);
        if (cleaned.empty())
            return false;
        map_name_ = std::move(cleaned);
        return true;
    };

    for (auto off : prefer) {
        uintptr_t p = 0;
        std::memcpy(&p, blob + off, sizeof(p));
        if (take(p))
            return;
    }

    for (size_t off = 0x80; off + 8 <= sizeof(blob); off += 8) {
        uintptr_t p = 0;
        std::memcpy(&p, blob + off, sizeof(p));
        if (take(p))
            return;
    }
}

uintptr_t Game::pawn_from_handle(uint32_t handle) const
{
    if (!handle || handle == 0xFFFFFFFF)
        return 0;
    return entity_by_index(static_cast<int>(handle & offsets::entity_system::kHandleIndexMask));
}

bool Game::tick(bool read_bones)
{
    players_.clear();

    const DWORD now = GetTickCount();
    if (!attached_ || !mem_.ok()) {
        if (now - last_attach_ms_ < 800)
            return false;
        last_attach_ms_ = now;
        if (!mem_.attach(L"cs2.exe")) {
            attached_ = false;
            client_ = engine_ = 0;
            fps_ = 0;
            fps_smooth_ = 0.f;
            local_ping_ = 0;
            if (read_bones)
                skel_log::write("tick: cs2.exe not attached");
            return false;
        }
        client_ = mem_.module_base(L"client.dll");
        engine_ = mem_.module_base(L"engine2.dll");
        attached_ = client_ && engine_;
        if (!attached_) {
            if (read_bones)
                skel_log::write("tick: attached process but client.dll/engine2.dll base missing");
            return false;
        }
        skel_log::write("tick: attached pid=%u client=%p engine=%p",
                        mem_.pid(), reinterpret_cast<void*>(client_), reinterpret_cast<void*>(engine_));
    }

    build_number_ = mem_.read<uint32_t>(engine_ + offsets::engine2::dwBuildNumber);
    entity_list_ = mem_.read<uintptr_t>(client_ + offsets::client::dwEntityList);
    if (!mem_.read_raw(client_ + offsets::client::dwViewMatrix, &view_, sizeof(view_))) {
        if (read_bones)
            skel_log::write("tick: view matrix rpm failed client=%p dwViewMatrix=0x%tx",
                            reinterpret_cast<void*>(client_), offsets::client::dwViewMatrix);
        return false;
    }

    {
        const uintptr_t gv = mem_.read<uintptr_t>(client_ + offsets::client::dwGlobalVars);
        float instant = 0.f;
        if (gv) {
            const float abs_ft = mem_.read<float>(gv + offsets::globalvars::m_flAbsoluteFrameTime);
            const int frames = mem_.read<int>(gv + offsets::globalvars::m_nFrameCount);
            if (std::isfinite(abs_ft) && abs_ft > 0.0008f && abs_ft < 0.2f)
                instant = 1.f / abs_ft;
            if (fps_last_ms_ && frames > fps_last_frames_ && now > fps_last_ms_) {
                const float dt = static_cast<float>(now - fps_last_ms_) * 0.001f;
                if (dt >= 0.18f) {
                    const float sampled = static_cast<float>(frames - fps_last_frames_) / dt;
                    if (sampled > 5.f && sampled < 1000.f && instant < 1.f)
                        instant = sampled;
                    fps_last_frames_ = frames;
                    fps_last_ms_ = now;
                }
            } else {
                fps_last_frames_ = frames;
                fps_last_ms_ = now;
            }
        }
        if (instant > 1.f) {
            fps_smooth_ = (fps_smooth_ < 1.f) ? instant : (fps_smooth_ * 0.82f + instant * 0.18f);
            fps_ = static_cast<int>(fps_smooth_ + 0.5f);
        }
    }

    if (now - last_map_ms_ > 1500) {
        last_map_ms_ = now;
        detect_map();
    }

    const uintptr_t local_pawn = mem_.read<uintptr_t>(client_ + offsets::client::dwLocalPlayerPawn);
    const uintptr_t local_controller = mem_.read<uintptr_t>(client_ + offsets::client::dwLocalPlayerController);
    local_team_ = local_pawn ? static_cast<int>(mem_.read<uint8_t>(local_pawn + offsets::schema::C_BaseEntity::m_iTeamNum)) : 0;

    if (local_controller) {
        char raw[128]{};
        mem_.read_raw(local_controller + offsets::schema::CBasePlayerController::m_iszPlayerName, raw, sizeof(raw) - 1);
        raw[sizeof(raw) - 1] = 0;
        std::string name = raw;
        if (name.empty()) {
            const uintptr_t sp = mem_.read<uintptr_t>(local_controller + offsets::schema::CCSPlayerController::m_sSanitizedPlayerName);
            if (sp) {
                char buf[64]{};
                mem_.read_raw(sp, buf, sizeof(buf) - 1);
                name = buf;
            }
        }
        while (!name.empty() && static_cast<unsigned char>(name.back()) < 32)
            name.pop_back();
        if (!name.empty() && name[0] >= 32)
            local_name_ = std::move(name);
        const uint32_t ping = mem_.read<uint32_t>(local_controller + offsets::schema::CCSPlayerController::m_iPing);
        local_ping_ = (ping > 0 && ping < 1000u) ? static_cast<int>(ping) : 0;
    }

    if (local_pawn) {
        const uintptr_t node = mem_.read<uintptr_t>(local_pawn + offsets::schema::C_BaseEntity::m_pGameSceneNode);
        if (node)
            local_origin_ = mem_.read<Vec3>(node + offsets::schema::CGameSceneNode::m_vecAbsOrigin);
        else
            local_origin_ = mem_.read<Vec3>(local_pawn + offsets::schema::C_BasePlayerPawn::m_vOldOrigin);
        const uint32_t lf = mem_.read<uint32_t>(local_pawn + offsets::schema::C_BaseEntity::m_fFlags);
        const bool ducked = (lf & offsets::flags::FL_DUCKING) != 0;
        const Vec3 view_off = mem_.read<Vec3>(local_pawn + offsets::schema::C_BaseModelEntity::m_vecViewOffset);
        if (view_off.length() > 8.f && view_off.length() < 90.f)
            local_head_ = local_origin_ + view_off;
        else
            local_head_ = local_origin_ + Vec3{ 0.f, 0.f, ducked ? 46.f : 64.f };

        if (read_bones && node) {
            static bool logged_local = false;
            if (!logged_local) {
                logged_local = true;
                const uintptr_t live = bone_array_from_skeleton(mem_, node);
                const uintptr_t a190 = mem_.read<uintptr_t>(node + 0x190 + 0x80);
                skel_log::write("local pawn=%p origin=(%.1f,%.1f,%.1f) array=%p probe190=%p",
                                reinterpret_cast<void*>(local_pawn),
                                local_origin_.x, local_origin_.y, local_origin_.z,
                                reinterpret_cast<void*>(live), reinterpret_cast<void*>(a190));
                if (plausible_pointer(live)) {
                    std::array<BoneEntry, offsets::skeleton::kBoneCount> raw{};
                    if (mem_.read_raw(live, raw.data(), sizeof(raw))) {
                        Vec3 cache[offsets::skeleton::kBoneCount]{};
                        for (int i = 0; i < offsets::skeleton::kBoneCount; ++i)
                            cache[i] = raw[i].position;
                        Vec3 joints[kSkelCount]{};
                        std::uint32_t mask = 0;
                        const char* layout = skel_resolve(cache, offsets::skeleton::kBoneCount, local_origin_, joints, mask);
                        skel_log::write("local resolve layout=%s mask=0x%05x", layout, mask);
                        static const char* kNames[] = {
                            "head","neck","spineU","spineL","pelvis",
                            "Lsh","Lel","Lhd","Rsh","Rel","Rhd",
                            "Lhp","Lkn","Lft","Rhp","Rkn","Rft"
                        };
                        for (int i = 0; i < kSkelCount; ++i) {
                            if ((mask & (1u << i)) == 0) {
                                skel_log::write("joint %-7s MISSING", kNames[i]);
                                continue;
                            }
                            const Vec3 d = joints[i] - local_origin_;
                            skel_log::write("joint %-7s d=(%6.1f,%6.1f,%6.1f)", kNames[i], d.x, d.y, d.z);
                        }
                        for (int i = 0; i < 24; ++i) {
                            const Vec3 d = cache[i] - local_origin_;
                            skel_log::write("raw[%02d] d=(%6.1f,%6.1f,%6.1f)", i, d.x, d.y, d.z);
                        }
                    }
                }
            }
        }
    }

    for (int i = 1; i <= 64; ++i) {
        const uintptr_t controller = entity_by_index(i);
        if (!controller || controller == local_controller)
            continue;

        if (!mem_.read<bool>(controller + offsets::schema::CCSPlayerController::m_bPawnIsAlive))
            continue;

        const uint32_t pawn_handle = mem_.read<uint32_t>(controller + offsets::schema::CCSPlayerController::m_hPlayerPawn);
        const uintptr_t pawn = pawn_from_handle(pawn_handle);
        if (!pawn || pawn == local_pawn)
            continue;

        const uint8_t life = mem_.read<uint8_t>(pawn + offsets::schema::C_BaseEntity::m_lifeState);
        if (life != offsets::life::kAlive)
            continue;

        const uintptr_t node = mem_.read<uintptr_t>(pawn + offsets::schema::C_BaseEntity::m_pGameSceneNode);
        if (!node)
            continue;
        if (mem_.read<bool>(node + offsets::schema::CGameSceneNode::m_bDormant))
            continue;

        Player p{};
        p.controller = controller;
        p.pawn = pawn;
        p.origin = mem_.read<Vec3>(node + offsets::schema::CGameSceneNode::m_vecAbsOrigin);
        p.health = mem_.read<int>(pawn + offsets::schema::C_BaseEntity::m_iHealth);
        p.max_health = mem_.read<int>(pawn + offsets::schema::C_BaseEntity::m_iMaxHealth);
        p.team = static_cast<int>(mem_.read<uint8_t>(pawn + offsets::schema::C_BaseEntity::m_iTeamNum));
        const uint32_t f = mem_.read<uint32_t>(pawn + offsets::schema::C_BaseEntity::m_fFlags);
        p.ducked = (f & offsets::flags::FL_DUCKING) != 0;
        p.speed = mem_.read<Vec3>(pawn + offsets::schema::C_BaseEntity::m_vecAbsVelocity).length();

        if (p.health <= 0)
            p.health = static_cast<int>(mem_.read<uint32_t>(controller + offsets::schema::CCSPlayerController::m_iPawnHealth));
        if (p.max_health <= 0)
            p.max_health = 100;
        p.health = std::clamp(p.health, 0, p.max_health > 0 ? p.max_health : 100);

        Vec3 mins = mem_.read<Vec3>(pawn + offsets::schema::C_BaseModelEntity::m_Collision + offsets::schema::CCollisionProperty::m_vecMins);
        Vec3 maxs = mem_.read<Vec3>(pawn + offsets::schema::C_BaseModelEntity::m_Collision + offsets::schema::CCollisionProperty::m_vecMaxs);

        const float height = (maxs.z - mins.z > 8.f) ? (maxs.z - mins.z) : (p.ducked ? 54.f : 72.f);
        const float hx = (maxs.x - mins.x > 4.f) ? (maxs.x - mins.x) * 0.5f : 16.f;
        const float hy = (maxs.y - mins.y > 4.f) ? (maxs.y - mins.y) * 0.5f : 16.f;

        p.mins = { -hx, -hy, 0.f };
        p.maxs = { hx, hy, height };
        p.head = p.origin + Vec3{ 0.f, 0.f, height };
        p.eye = p.origin + Vec3{ 0.f, 0.f, p.ducked ? 46.f : 64.f };

        const Vec3 o = p.origin;
        p.corners = {
            o + Vec3{ -hx, -hy, 0.f },
            o + Vec3{  hx, -hy, 0.f },
            o + Vec3{  hx,  hy, 0.f },
            o + Vec3{ -hx,  hy, 0.f },
            o + Vec3{ -hx, -hy, height },
            o + Vec3{  hx, -hy, height },
            o + Vec3{  hx,  hy, height },
            o + Vec3{ -hx,  hy, height },
        };

        p.distance = p.origin.dist(local_origin_);
        if (p.team < 2 || p.team > 3)
            continue;

        if (read_bones) {
            const std::ptrdiff_t model_state = offsets::schema::CSkeletonInstance::m_modelState;
            const std::ptrdiff_t bone_off = offsets::schema::CModelState::m_boneArray;
            const uintptr_t body = mem_.read<uintptr_t>(pawn + offsets::schema::C_BaseEntity::m_CBodyComponent);
            const uintptr_t embedded = body
                ? (body + offsets::schema::CBodyComponentSkeletonInstance::m_skeletonInstance)
                : 0;

            uintptr_t bone_array = bone_array_from_skeleton(mem_, node);
            const char* source = "scene_node";
            if (!bone_array && embedded) {
                bone_array = bone_array_from_skeleton(mem_, embedded);
                source = "body_component";
            }

            const uintptr_t probe_140 = mem_.read<uintptr_t>(node + 0x140 + 0x80);
            const uintptr_t probe_190 = mem_.read<uintptr_t>(node + 0x190 + 0x80);

            if (!bone_array) {
                skel_log::write(
                    "FAIL pawn=%p node=%p body=%p m_modelState=0x%tx +0x%tx array=null probe140=%p probe190=%p",
                    reinterpret_cast<void*>(pawn), reinterpret_cast<void*>(node),
                    reinterpret_cast<void*>(body), model_state, bone_off,
                    reinterpret_cast<void*>(probe_140), reinterpret_cast<void*>(probe_190));
            } else {
                std::array<BoneEntry, offsets::skeleton::kBoneCount> raw{};
                if (!mem_.read_raw(bone_array, raw.data(), sizeof(raw))) {
                    skel_log::write(
                        "FAIL rpm pawn=%p array=%p src=%s bytes=%zu",
                        reinterpret_cast<void*>(pawn), reinterpret_cast<void*>(bone_array),
                        source, sizeof(raw));
                } else {
                    Vec3 cache[offsets::skeleton::kBoneCount]{};
                    int accepted = 0;
                    int nan_or_zero = 0;
                    int far_from_origin = 0;
                    for (int bone = 0; bone < offsets::skeleton::kBoneCount; ++bone) {
                        const Vec3& pos = raw[bone].position;
                        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z) ||
                            (std::fabs(pos.x) < 0.001f && std::fabs(pos.y) < 0.001f && std::fabs(pos.z) < 0.001f)) {
                            ++nan_or_zero;
                            continue;
                        }
                        if (!valid_bone_position(pos, p.origin)) {
                            ++far_from_origin;
                            continue;
                        }
                        cache[bone] = pos;
                        ++accepted;
                    }
                    if (accepted == 0) {
                        skel_log::write(
                            "FAIL none-valid pawn=%p src=%s array=%p nan/zero=%d far=%d first=(%.2f,%.2f,%.2f) dist=%.1f probe140=%p probe190=%p",
                            reinterpret_cast<void*>(pawn), source, reinterpret_cast<void*>(bone_array),
                            nan_or_zero, far_from_origin,
                            raw[0].position.x, raw[0].position.y, raw[0].position.z,
                            raw[0].position.dist(p.origin),
                            reinterpret_cast<void*>(probe_140), reinterpret_cast<void*>(probe_190));
                    } else {
                        const char* layout = skel_resolve(cache, offsets::skeleton::kBoneCount, p.origin,
                                                          p.joints.data(), p.joint_mask);
                        static bool logged_ok = false;
                        if (!logged_ok) {
                            logged_ok = true;
                            skel_log::write(
                                "resolve layout=%s pawn=%p accepted_raw=%d joint_mask=0x%05x origin=(%.1f,%.1f,%.1f)",
                                layout, reinterpret_cast<void*>(pawn), accepted, p.joint_mask,
                                p.origin.x, p.origin.y, p.origin.z);
                            for (int i = 0; i < 32; ++i) {
                                const Vec3 d = cache[i] - p.origin;
                                skel_log::write("raw[%02d] d=(%6.1f,%6.1f,%6.1f) abs=(%.1f,%.1f,%.1f)",
                                                i, d.x, d.y, d.z, cache[i].x, cache[i].y, cache[i].z);
                            }
                            static const char* kNames[] = {
                                "head","neck","spineU","spineL","pelvis",
                                "Lsh","Lel","Lhd","Rsh","Rel","Rhd",
                                "Lhp","Lkn","Lft","Rhp","Rkn","Rft"
                            };
                            for (int i = 0; i < kSkelCount; ++i) {
                                if ((p.joint_mask & (1u << i)) == 0) {
                                    skel_log::write("joint %-7s MISSING", kNames[i]);
                                    continue;
                                }
                                const Vec3 d = p.joints[i] - p.origin;
                                skel_log::write("joint %-7s d=(%6.1f,%6.1f,%6.1f) world=(%.1f,%.1f,%.1f)",
                                                kNames[i], d.x, d.y, d.z,
                                                p.joints[i].x, p.joints[i].y, p.joints[i].z);
                            }
                        }
                    }
                }
            }
        }

        players_.push_back(p);
    }

    if (read_bones) {
        int with_bones = 0;
        for (const Player& p : players_) {
            if (p.joint_mask)
                ++with_bones;
        }
        static int last_players = -1;
        static int last_with = -1;
        if (static_cast<int>(players_.size()) != last_players || with_bones != last_with) {
            last_players = static_cast<int>(players_.size());
            last_with = with_bones;
            skel_log::write(
                "tick attached=%d build=%u players=%zu with_bones=%d m_modelState=0x%tx map=%s",
                attached_ ? 1 : 0, build_number_, players_.size(), with_bones,
                offsets::schema::CSkeletonInstance::m_modelState,
                map_name_.empty() ? "-" : map_name_.c_str());
        }
    }

    return true;
}
