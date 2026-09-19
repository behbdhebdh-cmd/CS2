#pragma once

// CS2 offsets — a2x/cs2-dumper output, 2026-09-10 12:36 UTC (game update 14181)
// Cross-check: https://www.cheatoffsets.com/g/cs2
// Refresh this file after every CS2 patch. Schema fields live in client.dll classes.

#include <cstddef>
#include <cstdint>

namespace offsets {

inline constexpr const char* kDumpUtc     = "2026-09-10 12:36:27 UTC";
inline constexpr const char* kDumpUpdate  = "14181";

namespace client {
    constexpr std::ptrdiff_t dwCSGOInput                           = 0x23E2610;
    constexpr std::ptrdiff_t dwEntityList                          = 0x2577BE0;
    constexpr std::ptrdiff_t dwGameEntitySystem                    = 0x2577BE0;
    constexpr std::ptrdiff_t dwGameEntitySystem_highestEntityIndex = 0x2090;
    constexpr std::ptrdiff_t dwGameRules                           = 0x23CC6C8;
    constexpr std::ptrdiff_t dwGlobalVars                          = 0x20B57C0;
    constexpr std::ptrdiff_t dwGlowManager                         = 0x23C93F8;
    constexpr std::ptrdiff_t dwLocalPlayerController               = 0x23A78D0;
    constexpr std::ptrdiff_t dwLocalPlayerPawn                     = 0x23CCC08;
    constexpr std::ptrdiff_t dwPlantedC4                           = 0x23973B8;
    constexpr std::ptrdiff_t dwPrediction                          = 0x23CCB10;
    constexpr std::ptrdiff_t dwSensitivity                         = 0x23C9F18;
    constexpr std::ptrdiff_t dwSensitivity_sensitivity             = 0x58;
    constexpr std::ptrdiff_t dwViewAngles                          = 0x23E2C98;
    constexpr std::ptrdiff_t dwViewMatrix                          = 0x23D21F0;
    constexpr std::ptrdiff_t dwViewRender                          = 0x23D2258;
    constexpr std::ptrdiff_t dwWeaponC4                            = 0x2345728;
}

namespace engine2 {
    constexpr std::ptrdiff_t dwBuildNumber                       = 0x6105A4;
    constexpr std::ptrdiff_t dwNetworkGameClient                 = 0x90E6A0;
    constexpr std::ptrdiff_t dwNetworkGameClient_clientTickCount = 0x378;
    constexpr std::ptrdiff_t dwNetworkGameClient_signOnState     = 0x230;
    constexpr std::ptrdiff_t dwNetworkGameClient_localPlayer     = 0xF8;
    constexpr std::ptrdiff_t dwNetworkGameClient_maxClients      = 0x240;
    constexpr std::ptrdiff_t dwWindowHeight                      = 0x912AC4;
    constexpr std::ptrdiff_t dwWindowWidth                       = 0x912AC0;
}

// Entity-list walk (CGameEntitySystem). Stride has been 0x70 across recent CS2 builds.
namespace entity_system {
    constexpr std::ptrdiff_t kListOffset  = 0x10;
    constexpr std::ptrdiff_t kEntryStride = 0x70;
    constexpr std::uint32_t  kHandleIndexMask = 0x7FFF;
}

namespace schema {
    namespace C_BaseEntity {
        constexpr std::ptrdiff_t m_pGameSceneNode = 0x330;
        constexpr std::ptrdiff_t m_pCollision     = 0x340;
        constexpr std::ptrdiff_t m_iMaxHealth     = 0x348;
        constexpr std::ptrdiff_t m_iHealth        = 0x34C;
        constexpr std::ptrdiff_t m_lifeState      = 0x354;
        constexpr std::ptrdiff_t m_iTeamNum       = 0x3E7;
        constexpr std::ptrdiff_t m_fFlags         = 0x3F4;
        constexpr std::ptrdiff_t m_vecAbsVelocity = 0x3F8;
        constexpr std::ptrdiff_t m_hOwnerEntity   = 0x520;
    }

    namespace CGameSceneNode {
        constexpr std::ptrdiff_t m_vecAbsOrigin = 0xC8;
        constexpr std::ptrdiff_t m_bDormant     = 0x103;
    }

    namespace C_BaseModelEntity {
        constexpr std::ptrdiff_t m_Collision     = 0xD28; // embedded CCollisionProperty
        constexpr std::ptrdiff_t m_vecViewOffset = 0xE78;
    }

    namespace CCollisionProperty {
        constexpr std::ptrdiff_t m_vecMins = 0x40;
        constexpr std::ptrdiff_t m_vecMaxs = 0x4C;
    }

    namespace C_BasePlayerPawn {
        constexpr std::ptrdiff_t m_vOldOrigin  = 0x13B8;
        constexpr std::ptrdiff_t m_hController = 0x13D0;
    }

    namespace CBasePlayerController {
        constexpr std::ptrdiff_t m_hPawn          = 0x6BC;
        constexpr std::ptrdiff_t m_iszPlayerName  = 0x6F4; // char[128]
        constexpr std::ptrdiff_t m_steamID        = 0x780;
        constexpr std::ptrdiff_t m_bIsLocalPlayerController = 0x788;
    }

    namespace CCSPlayerController {
        constexpr std::ptrdiff_t m_sSanitizedPlayerName = 0x868;
        constexpr std::ptrdiff_t m_hPlayerPawn          = 0x914;
        constexpr std::ptrdiff_t m_bPawnIsAlive         = 0x91C;
        constexpr std::ptrdiff_t m_iPawnHealth          = 0x920;
        constexpr std::ptrdiff_t m_iPawnArmor           = 0x924;
    }
}

namespace life {
    constexpr std::uint8_t kAlive = 0;
}

namespace flags {
    constexpr std::uint32_t FL_DUCKING = 1u << 1;
}

} // namespace offsets
