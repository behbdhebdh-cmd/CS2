# CS2 offsets — CheatOffsets

Primary documentation for this project comes from:

- Site: https://www.cheatoffsets.com/
- CS2 index: https://www.cheatoffsets.com/g/cs2

CheatOffsets republishes **cs2-dumper** output (module RVAs, button table,
interface pointers, schema fields). The site trims the huge `client_dll.hpp`
dump down to the fields you actually paste into an offset header.

## How to refresh

1. Open https://www.cheatoffsets.com/g/cs2
2. Confirm the **pinned build number** and timestamp at the top of the page.
3. Copy `client.dll`, `engine2.dll`, `buttons`, and the essential schemas.
4. Paste into `src/sdk/offsets.hpp`.
5. Write the build number + UTC time in a comment at the top of that file.
6. Never assume yesterday's values still work.

Optional mirrors if the site is down (still verify the build number):

- https://github.com/a2x/cs2-dumper
- https://github.com/sezzyaep/CS2-OFFSETS

## What the numbers mean

All `dw*` values on the CS2 page are **RVAs**: add them to the loaded module
base (`client.dll`, `engine2.dll`, …). Schema fields (`m_iHealth`, …) are
offsets from a class instance, not from the module base.

### client.dll (module RVAs)

| Name | Role |
| --- | --- |
| `dwEntityList` | Pointer into the entity list / game entity system |
| `dwGameEntitySystem` | Same family as the entity list on current builds |
| `dwGameEntitySystem_highestEntityIndex` | Highest occupied entity slot |
| `dwLocalPlayerController` | Local CCSPlayerController |
| `dwLocalPlayerPawn` | Local C_CSPlayerPawn |
| `dwViewMatrix` | 4x4 world-to-clip matrix |
| `dwViewAngles` | Client view angles |
| `dwGameRules` | CCSGameRules proxy |
| `dwGlobalVars` | `CGlobalVarsBase` |
| `dwGlowManager` | Glow object manager |
| `dwCSGOInput` | Input / usercmd path |
| `dwPlantedC4` | Planted bomb pawn (if any) |
| `dwWeaponC4` | C4 weapon instance |
| `dwPrediction` | Client prediction |
| `dwSensitivity` / `dwSensitivity_sensitivity` | Mouse sensitivity blob |

### engine2.dll

| Name | Role |
| --- | --- |
| `dwBuildNumber` | Engine build — sanity-check against CheatOffsets |
| `dwNetworkGameClient` | Networked client object |
| `dwNetworkGameClient_signOnState` | Sign-on / connected state |
| `dwNetworkGameClient_localPlayer` | Local slot on the net client |
| `dwWindowWidth` / `dwWindowHeight` | Game window size |

### buttons (client.dll)

`attack`, `attack2`, `jump`, `duck`, `forward`, `back`, `left`, `right`,
`reload`, `use`, `zoom`, `lookatweapon`, … — kbutton RVAs from the input table.

### Essential schemas

Copied from the CheatOffsets "Essential client schemas" block (snapshot
2026-08-22, **verify before use**):

```text
C_BaseEntity
  m_pGameSceneNode   0x330
  m_iHealth          0x34C
  m_iMaxHealth       0x348
  m_lifeState        0x354
  m_iTeamNum         0x3E7
  m_fFlags           0x3F4
```

Also pull, when you need them: `CGameSceneNode` origin/rotation,
`C_BasePlayerPawn` camera, `CCSPlayerController` pawn handle / name,
`C_CSPlayerPawn` weapon / scoped / flash.

## Dated snapshot (do not trust blindly)

CheatOffsets CS2 page, pinned build **24828357**, updated **2026-08-22 21:20 UTC**.
Several fields were truncated on the public page scrape; those are `0x0` in
`src/sdk/offsets.hpp` on purpose — fill them from the live site.

```text
client.dll
  dwCSGOInput                            0x23BFB20
  dwEntityList                           0x2555050
  dwGameEntitySystem                     0x2555050
  dwGameEntitySystem_highestEntityIndex  0x2090
  dwGameRules                            0x23A9BD8
  dwGlobalVars                           0x2095D48
  dwGlowManager                          0x23A6908
  dwLocalPlayerController                0x2384DB0

engine2.dll
  dwBuildNumber                          0x60F594
  dwNetworkGameClient                    0x90D4B0
  dwNetworkGameClient_clientTickCount    0x378
  dwNetworkGameClient_deltaTick          0x24C
  dwNetworkGameClient_localPlayer        0xF8
  dwNetworkGameClient_maxClients         0x240

buttons
  attack       0x209A000
  attack2      0x209A090
  jump         0x209A510
  duck         0x209A5A0
  forward      0x209A240
  back         0x209A2D0
  left         0x209A360
  right        0x209A3F0
```

## Vibe-coding rule

When an offset is wrong the overlay will look "fine" and still be useless.
Always:

1. Read `dwBuildNumber` and compare it to the CheatOffsets build.
2. If they differ, stop and re-paste the dump.
3. Keep the header compiling even when a value is still `0x0`.
