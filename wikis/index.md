# CS2 overlay wiki

Private **bot-match only** external overlay for Counter-Strike 2. Runtime binary used on this machine: `D:\CS2\release\CS2.exe`. Source and CMake tree: `D:\CS2`.

This wiki is the developer documentation. Project files are English. Do not treat the older notes under [`docs/`](../docs/) as current unless a wiki page still points at them.

**Scope:** educational overlay for a private server where the host allows it. Do not use it on official matchmaking, Faceit, or any server that forbids overlays.

## Pages

| Page | Contents |
| --- | --- |
| [Getting started](getting-started.md) | Install, build, start, private bot match |
| [Features](features.md) | Implemented vs UI-only vs not started |
| [ESP](esp.md) | Box, health, head, distance, **skeleton**, vis check, filters, colors |
| [Menu](menu.md) | LianFlow UI, open/close animation, keybinds, color pickers |
| [Watermark](watermark.md) | Status bar: FPS, ping, session time, region, name |
| [Offsets](offsets.md) | CheatOffsets poller, ETag, cache, apply path |
| [Architecture](architecture.md) | Tree, modules, per-frame data flow |
| [Build](build.md) | Toolchain, CMake, `scripts/build.bat`, release copy |
| [Troubleshooting](troubleshooting.md) | Attach, vis meshes, offsets, click-through, color pickers |
| [FAQ](faq.md) | Common questions |
| [Roadmap](roadmap.md) | Aim, trigger, chams, config |
| [Credits](credits.md) | Vendors, dumpers, extractors |

## Current snapshot

| Area | State in this tree |
| --- | --- |
| DX11 overlay | Layered, topmost, click-through when the menu is closed |
| Menu | Insert / F7 toggle, ESC hide, F8 unload |
| ESP | Corner / 3D / filled boxes, glass health, head marker, distance, skeleton |
| Skeleton | Logical `Skel` joints; layout score + geometric resolve; vis-colored lines |
| Visibility | BVH raycast against `maps/tri/{map}.tri` |
| Offsets | Auto-poll CheatOffsets API every 10 minutes (ETag) |
| Aim / trigger | Menu controls only, not wired to game input |
| Config JSON | `config/settings.json` is a placeholder; Save/Load is not wired |

## Quick links (source)

- Overlay entry: [`src/app/main.cpp`](../src/app/main.cpp)
- Menu state: [`src/app/settings.hpp`](../src/app/settings.hpp)
- ESP + watermark: [`src/features/esp.cpp`](../src/features/esp.cpp)
- Skeleton resolve: [`src/sdk/skeleton.hpp`](../src/sdk/skeleton.hpp)
- Game read: [`src/sdk/game.cpp`](../src/sdk/game.cpp)
- Offset poller: [`src/sdk/offset_update/README.md`](../src/sdk/offset_update/README.md)
- Map meshes: [`maps/README.md`](../maps/README.md)

---

[Getting started](getting-started.md) · [Architecture](architecture.md) · [Build](build.md)
