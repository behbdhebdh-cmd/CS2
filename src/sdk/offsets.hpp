#pragma once

// CS2 offsets — baked fallback from a2x/cs2-dumper 2026-09-10 12:36 UTC (update 14181).
// Live values are overwritten at runtime by src/sdk/offset_update.cpp from
// https://www.cheatoffsets.com/api/games/cs2/current (offsets_flat + ETag poll).

#include <cstddef>
#include <cstdint>

namespace offsets {

inline char kDumpUtc[80]     = "2026-09-10 12:36:27 UTC";
inline char kDumpUpdate[48]  = "14181";
inline char kDumpSource[24]  = "baked";

namespace client {
    inline std::ptrdiff_t dwCSGOInput                           = 0x23E2610;
    inline std::ptrdiff_t dwEntityList                          = 0x2577BE0;
    inline std::ptrdiff_t dwGameEntitySystem                    = 0x2577BE0;
    inline std::ptrdiff_t dwGameEntitySystem_highestEntityIndex = 0x2090;
    inline std::ptrdiff_t dwGameRules                           = 0x23CC6C8;
    inline std::ptrdiff_t dwGlobalVars                          = 0x20B57C0;
    inline std::ptrdiff_t dwGlowManager                         = 0x23C93F8;
    inline std::ptrdiff_t dwLocalPlayerController               = 0x23A78D0;
    inline std::ptrdiff_t dwLocalPlayerPawn                     = 0x23CCC08;
    inline std::ptrdiff_t dwPlantedC4                           = 0x23973B8;
    inline std::ptrdiff_t dwPrediction                          = 0x23CCB10;
    inline std::ptrdiff_t dwSensitivity                         = 0x23C9F18;
    inline std::ptrdiff_t dwSensitivity_sensitivity             = 0x58;
    inline std::ptrdiff_t dwViewAngles                          = 0x23E2C98;
    inline std::ptrdiff_t dwViewMatrix                          = 0x23D21F0;
    inline std::ptrdiff_t dwViewRender                          = 0x23D2258;
    inline std::ptrdiff_t dwWeaponC4                            = 0x2345728;
}

namespace engine2 {
    inline std::ptrdiff_t dwBuildNumber                       = 0x6105A4;
    inline std::ptrdiff_t dwNetworkGameClient                 = 0x90E6A0;
    inline std::ptrdiff_t dwNetworkGameClient_clientTickCount = 0x378;
    inline std::ptrdiff_t dwNetworkGameClient_signOnState     = 0x230;
    inline std::ptrdiff_t dwNetworkGameClient_localPlayer     = 0xF8;
    inline std::ptrdiff_t dwNetworkGameClient_maxClients      = 0x240;
    inline std::ptrdiff_t dwWindowHeight                      = 0x912AC4;
    inline std::ptrdiff_t dwWindowWidth                       = 0x912AC0;
}

// Entity-list walk (CGameEntitySystem). Stride has been 0x70 across recent CS2 builds.
namespace globalvars {
    constexpr std::ptrdiff_t m_nFrameCount          = 0x04;
    constexpr std::ptrdiff_t m_flAbsoluteFrameTime  = 0x08;
}

namespace entity_system {
    constexpr std::ptrdiff_t kListOffset  = 0x10;
    constexpr std::ptrdiff_t kEntryStride = 0x70;
    constexpr std::uint32_t  kHandleIndexMask = 0x7FFF;
}

namespace schema {
    namespace C_BaseEntity {
        inline std::ptrdiff_t m_CBodyComponent = 0x30;
        inline std::ptrdiff_t m_pGameSceneNode = 0x330;
        inline std::ptrdiff_t m_pCollision     = 0x340;
        inline std::ptrdiff_t m_iMaxHealth     = 0x348;
        inline std::ptrdiff_t m_iHealth        = 0x34C;
        inline std::ptrdiff_t m_lifeState      = 0x354;
        inline std::ptrdiff_t m_iTeamNum       = 0x3E7;
        inline std::ptrdiff_t m_fFlags         = 0x3F4;
        inline std::ptrdiff_t m_vecAbsVelocity = 0x3F8;
        inline std::ptrdiff_t m_hOwnerEntity   = 0x520;
    }

    namespace CGameSceneNode {
        inline std::ptrdiff_t m_vecAbsOrigin = 0xC8;
        inline std::ptrdiff_t m_bDormant     = 0x103;
    }

    namespace CSkeletonInstance {
        // a2x/cs2-dumper 2026-09-10: 0x140 (older dumps used 0x160/0x190).
        inline std::ptrdiff_t m_modelState = 0x140;
    }

    namespace CModelState {
        // Undocumented bone-cache pointer inside CModelState. Not a schema field.
        constexpr std::ptrdiff_t m_boneArray = 0x80;
    }

    namespace CBodyComponentSkeletonInstance {
        inline std::ptrdiff_t m_skeletonInstance = 0x80;
    }

    namespace C_BaseModelEntity {
        inline std::ptrdiff_t m_Collision     = 0xD28;
        inline std::ptrdiff_t m_vecViewOffset = 0xE78;
    }

    namespace CCollisionProperty {
        inline std::ptrdiff_t m_vecMins = 0x40;
        inline std::ptrdiff_t m_vecMaxs = 0x4C;
    }

    namespace C_BasePlayerPawn {
        inline std::ptrdiff_t m_pWeaponServices = 0x1208;
        inline std::ptrdiff_t m_pCameraServices = 0x1240;
        inline std::ptrdiff_t m_vOldOrigin      = 0x13B8;
        inline std::ptrdiff_t m_hController     = 0x13D0;
    }

    namespace C_CSPlayerPawnBase {
        inline std::ptrdiff_t m_flFlashOverlayAlpha = 0x141C;
        inline std::ptrdiff_t m_flFlashMaxAlpha     = 0x1424;
        inline std::ptrdiff_t m_flFlashDuration     = 0x1428;
    }

    namespace C_CSPlayerPawn {
        inline std::ptrdiff_t m_pAimPunchServices = 0x14B8;
        inline std::ptrdiff_t m_bIsScoped         = 0x1C78;
        inline std::ptrdiff_t m_iShotsFired       = 0x1C8C;
    }

    namespace CCSPlayer_AimPunchServices {
        inline std::ptrdiff_t m_predictableBaseAngle     = 0x50;
        inline std::ptrdiff_t m_unpredictableBaseAngle   = 0xA4;
    }

    namespace CPlayer_WeaponServices {
        inline std::ptrdiff_t m_hActiveWeapon = 0x60;
    }

    namespace CCSPlayerBase_CameraServices {
        inline std::ptrdiff_t m_iFOV = 0x290;
    }

    namespace C_EconEntity {
        inline std::ptrdiff_t m_AttributeManager = 0x11A8;
    }

    namespace C_AttributeContainer {
        inline std::ptrdiff_t m_Item = 0x50;
    }

    namespace C_EconItemView {
        inline std::ptrdiff_t m_iItemDefinitionIndex = 0x1BA;
    }

    namespace CBasePlayerController {
        inline std::ptrdiff_t m_hPawn          = 0x6BC;
        inline std::ptrdiff_t m_iszPlayerName  = 0x6F4;
        inline std::ptrdiff_t m_steamID        = 0x780;
        inline std::ptrdiff_t m_bIsLocalPlayerController = 0x788;
    }

    namespace CCSPlayerController {
        inline std::ptrdiff_t m_iPing                = 0x830;
        inline std::ptrdiff_t m_sSanitizedPlayerName = 0x868;
        inline std::ptrdiff_t m_hPlayerPawn          = 0x914;
        inline std::ptrdiff_t m_bPawnIsAlive         = 0x91C;
        inline std::ptrdiff_t m_iPawnHealth          = 0x920;
        inline std::ptrdiff_t m_iPawnArmor           = 0x924;
    }
}

namespace life {
    constexpr std::uint8_t kAlive = 0;
}

namespace flags {
    constexpr std::uint32_t FL_DUCKING = 1u << 1;
}

namespace skeleton {
    constexpr int kBoneCount = 96;
    constexpr std::ptrdiff_t kBoneStride = 0x20;
}

} // namespace offsets
