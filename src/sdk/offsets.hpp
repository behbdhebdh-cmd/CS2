#pragma once

// -----------------------------------------------------------------------------
// Offset snapshot (SETUP ONLY)
// Source of truth: https://www.cheatoffsets.com/g/cs2
//
// Values below are a dated public snapshot from CheatOffsets (pinned CS2 build
// 24828357, page updated 2026-08-22). They go stale after every CS2 update.
// Before wiring any runtime code, paste a fresh dump from that page.
//
// This header is documentation + a typed placeholder. Nothing in this repo
// reads or writes the game process yet.
// -----------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>

namespace offsets {

// client.dll — absolute RVAs from the module base
namespace client {
    constexpr std::ptrdiff_t dwCSGOInput                            = 0x23BFB20;
    constexpr std::ptrdiff_t dwEntityList                           = 0x2555050;
    constexpr std::ptrdiff_t dwGameEntitySystem                     = 0x2555050;
    constexpr std::ptrdiff_t dwGameEntitySystem_highestEntityIndex  = 0x2090;
    constexpr std::ptrdiff_t dwGameRules                            = 0x23A9BD8;
    constexpr std::ptrdiff_t dwGlobalVars                           = 0x2095D48;
    constexpr std::ptrdiff_t dwGlowManager                          = 0x23A6908;
    constexpr std::ptrdiff_t dwLocalPlayerController                = 0x2384DB0;
    constexpr std::ptrdiff_t dwLocalPlayerPawn                      = 0x0000000; // paste from CheatOffsets
    constexpr std::ptrdiff_t dwPlantedC4                            = 0x0000000;
    constexpr std::ptrdiff_t dwPrediction                           = 0x0000000;
    constexpr std::ptrdiff_t dwSensitivity                          = 0x0000000;
    constexpr std::ptrdiff_t dwSensitivity_sensitivity              = 0x0000000;
    constexpr std::ptrdiff_t dwViewAngles                           = 0x0000000;
    constexpr std::ptrdiff_t dwViewMatrix                           = 0x0000000;
    constexpr std::ptrdiff_t dwViewRender                           = 0x0000000;
    constexpr std::ptrdiff_t dwWeaponC4                             = 0x0000000;
}

// engine2.dll
namespace engine2 {
    constexpr std::ptrdiff_t dwBuildNumber                          = 0x60F594;
    constexpr std::ptrdiff_t dwNetworkGameClient                    = 0x90D4B0;
    constexpr std::ptrdiff_t dwNetworkGameClient_clientTickCount    = 0x378;
    constexpr std::ptrdiff_t dwNetworkGameClient_deltaTick          = 0x24C;
    constexpr std::ptrdiff_t dwNetworkGameClient_localPlayer        = 0xF8;
    constexpr std::ptrdiff_t dwNetworkGameClient_maxClients         = 0x240;
    constexpr std::ptrdiff_t dwNetworkGameClient_signOnState        = 0x240;
    constexpr std::ptrdiff_t dwWindowHeight                         = 0x0000000;
    constexpr std::ptrdiff_t dwWindowWidth                          = 0x0000000;
}

// client.dll buttons
namespace buttons {
    constexpr std::ptrdiff_t attack       = 0x209A000;
    constexpr std::ptrdiff_t attack2      = 0x209A090;
    constexpr std::ptrdiff_t back         = 0x209A2D0;
    constexpr std::ptrdiff_t duck         = 0x209A5A0;
    constexpr std::ptrdiff_t forward      = 0x209A240;
    constexpr std::ptrdiff_t jump         = 0x209A510;
    constexpr std::ptrdiff_t left         = 0x209A360;
    constexpr std::ptrdiff_t lookatweapon = 0x23BFA40;
    constexpr std::ptrdiff_t reload       = 0x2099F70;
    constexpr std::ptrdiff_t right        = 0x209A3F0;
    constexpr std::ptrdiff_t showscores   = 0x23BF920;
    constexpr std::ptrdiff_t sprint       = 0x2099EE0;
    constexpr std::ptrdiff_t use          = 0x209A480;
    constexpr std::ptrdiff_t zoom         = 0x23BF9B0;
}

// Essential client schema fields (netvars). Refresh from CheatOffsets
// "Essential client schemas" section after every patch.
namespace schema {
    namespace C_BaseEntity {
        constexpr std::ptrdiff_t m_pGameSceneNode = 0x330;
        constexpr std::ptrdiff_t m_iHealth        = 0x34C;
        constexpr std::ptrdiff_t m_iMaxHealth     = 0x348;
        constexpr std::ptrdiff_t m_lifeState      = 0x354;
        constexpr std::ptrdiff_t m_iTeamNum       = 0x3E7;
        constexpr std::ptrdiff_t m_fFlags         = 0x3F4;
        constexpr std::ptrdiff_t m_hOwnerEntity   = 0x0000000;
    }
}

} // namespace offsets
