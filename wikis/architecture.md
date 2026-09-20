# Architecture

[Wiki index](index.md)

External process overlay. No injection, no Detours. The overlay opens `cs2.exe` with `PROCESS_VM_READ | PROCESS_QUERY_INFORMATION`, reads `client.dll` / `engine2.dll`, and draws a layered DX11 window on top of the game client.

Older [`docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md) still describes the setup-only scaffold. Ignore it; this page matches the current `CMakeLists.txt`.

## Tree

```
D:\CS2\
  CMakeLists.txt
  README.md
  scripts/build.bat
  config/
    settings.json            legacy placeholder; live store is <config dir>/*.json
    offsets_cache.json       runtime (ETag + offsets_flat)
  maps/tri/*.tri             generated collision meshes
  cphys-extractor/           .NET 9 map dumper
  src/
    app/main.cpp             Win32 + DX11 + LianFlow + frame loop
    app/settings.hpp         MenuState, hit rects, globals
    app/config.cpp|.hpp      ConfigStore JSON + presets
    features/esp.cpp|.hpp    draw_players, draw_watermark
    features/combat.cpp|.hpp aimbot, triggerbot, weapon profiles
    features/weapon_icons.cpp|.hpp active-weapon glyph resolve
    sdk/
      memory.cpp|.hpp        attach, RPM, module base
      game.cpp|.hpp          entity walk, view matrix, map, fps, ping, bones
      skeleton.hpp           logical Skel joints, layout score, geometric resolve
      skel_log.hpp           optional skeleton.log next to the exe
      vis.cpp|.hpp           .tri load, BVH, raycast
      math.hpp               Vec3/Vec2/Mat4x4, world_to_screen
      offsets.hpp            baked + mutable RVAs / schemas
      offset_update.cpp|.hpp CheatOffsets poller
  vendor/imgui-lianflow/     ImGui, FreeType, D3DX11, custom widgets
  release/CS2.exe            runtime copy
  wikis/                     this documentation
```

`CMakeLists.txt` compiles: `main.cpp`, `config.cpp`, `memory.cpp`, `game.cpp`, `vis.cpp`, `offset_update.cpp`, `esp.cpp`, `combat.cpp`, `weapon_icons.cpp`, plus ImGui / FreeType / DX11 backends. Links: `d3d11`, `d3dcompiler`, `dxgi`, `freetype`, `d3dx11`, `winmm`, `dwmapi`, `winhttp`. C++17, Unicode, `/utf-8 /W3 /MP`, x64 only.

## Layers

| Layer | Role |
| --- | --- |
| App | Overlay window, device, ImGui frame, keybinds, menu, hit-testing |
| Features | Draw-only modules. Keep new visuals here, not inside LianFlow headers |
| SDK | Process memory, CS2 reads, vis mesh, offsets |
| Vendor | Do not edit unless updating ImGui / LianFlow |

## Per-frame data flow

```
PeekMessage
find CS2 window (250 ms) → SetWindowPos overlay to client rect
Game::tick(read_bones)
  attach cs2.exe if needed (800 ms retry)
  read build, entity list, view matrix, fps, map, local pawn, players[1..64]
  if read_bones: RPM bone array → skel_resolve → Player.joints
OffsetUpdate::tick()          apply pending API dump on this thread
VisCheck::tick(map_name)      start worker load if map changed
Insert/F7/ESC/F8
set_passthrough(!menu_open)
ImGui NewFrame
draw_players(background)
combat_draw (FOV ring) + draw_watermark(background)
combat_tick(Game, VisCheck, dt, menu_open)   aimbot + triggerbot via SendInput
if menu anim > 0: LianFlow panel + collect g_hits
Present(0, 0)
```

ESP does **not** interpolate box positions. Smoothing that path caused visible lag; projection is immediate.

## Memory

`Memory::attach(L"cs2.exe")` snapshots processes, `OpenProcess`, then `module_base` via `TH32CS_SNAPMODULE`. `read<T>` / `read_raw` wrap `ReadProcessMemory`. Failure returns zeroed `T` / false.

## Game snapshot

`Game` holds one frame of:

- `attached_`, `build_number_`, `view_` (4×4)
- `local_team_`, `local_origin_`, `local_head_`, `local_name_`, `local_ping_`, `fps_`
- `map_name_`
- `players_` (`Player`: controller/pawn, origin/head/eye, AABB corners, `joints`/`joint_mask`, health, team, distance, speed, ducked, weapon def + icon glyph)
- aim/trigger inputs: `view_angles_`, `punch_angles_`, `sensitivity_`, `shots_fired_`, `weapon_def_`, `camera_fov_`, `scoped_`, `flash_alpha_`, `local_alive_`

Entity walk: controllers `1..64` through `CGameEntitySystem` (`kListOffset 0x10`, stride `0x70`, handle mask `0x7FFF`). Skip local, dead (`m_bPawnIsAlive` / `m_lifeState`), dormant.

Map detect: read 0x280 bytes at `dwGlobalVars`, try pointer slots `{0x180, 0x188, 0x190, 0x1B8, 0x1C0, 0x218, 0x228, 0x230, 0x238}`, then scan remaining qwords. `sanitize_map_name` produces `de_mirage`-style keys.

## Visibility

`VisCheck` owns a `shared_ptr<const VisMesh>`. Load/build on a worker; `visible()` is a const raycast. See [ESP](esp.md#visible-check).

## Overlay window

Created fullscreen on the primary monitor, then resized to the game **client** rect (`ClientToScreen` of `(0,0)`). `DwmExtendFrameIntoClientArea` with `-1` margins + `LWA_ALPHA` 255 for a transparent layered window. Clear color is fully transparent.

Hit-testing: [Menu](menu.md#window-and-input).

## Concurrency

| Thread | Work |
| --- | --- |
| Main | RPM, ImGui, apply offsets, vis queries |
| Offset worker | WinHTTP GET, parse JSON, write cache, set `pending_` |
| Vis worker | Read `.tri`, build BVH |

Offset apply and vis swap are mutex / generation guarded. Do not read `offsets::*` from the HTTP thread.

## Config

| File | Used? |
| --- | --- |
| `config/offsets_cache.json` | Yes, offset poller |
| `<config dir>/*.json` | Yes, `ConfigStore`: full `MenuState` + accent, version 2 |

Config dir resolves to `<exe>\configs` first, then the release / project fallbacks. Three in-memory presets (Legit, Legit with Aim, Semi Rage) apply without files. See [Configs](configs.md).

---

[Getting started](getting-started.md) · [Offsets](offsets.md) · [ESP](esp.md) · [Build](build.md)
