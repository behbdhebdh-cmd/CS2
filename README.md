# CS2

Private overlay **setup** for a personal CS2 HVH server. This repository is
intentionally incomplete: folder layout, ImGui, a trimmed LianFlow menu, and
offset documentation. **No gameplay features are implemented.**

Use it as a vibe-coding base. Keep the LianFlow look, delete widgets you do not
need, then fill `src/features/` later.

## Status

| Area | State |
| --- | --- |
| Project layout | done |
| ImGui + FreeType + DX11 | vendored |
| LianFlow UI (trimmed) | done — visual shell only |
| Offset docs | done — [CheatOffsets CS2](https://www.cheatoffsets.com/g/cs2) |
| Process / memory / features | **not started** |

## UI base: ImGui LianFlow

The menu is **ImGui LianFlow** (`E:\ImGui-LianFlow.zip` unpacked into
`vendor/imgui-lianflow`). Treat that kit as a **base**, not a finished product.

Rules for continuing work:

1. Keep the LianFlow widgets, fonts, blur, and tab chrome.
2. **Remove unused buttons and pages.** The stock example shipped Fortnite-style
   extras (chests, vehicles, floor loot), a Discord avatar loader, duplicate
   aim pages, and an Exploits tab. Those are already stripped in `src/app/main.cpp`.
3. Do not add a widget unless you will actually wire it.
4. Branding, accent color, and tab names live in `src/app/main.cpp`.
5. Toggle the menu with **Insert**.

Current tabs:

```
Combat     Aim, Trigger
Visuals    Players, Radar
Misc       General
Settings   Menu, Config
```

Details: [`docs/UI.md`](docs/UI.md)

## Offsets

Source of truth: **https://www.cheatoffsets.com/**  
CS2 dump: **https://www.cheatoffsets.com/g/cs2**

Offsets move every CS2 patch. Copy a fresh dump into `src/sdk/offsets.hpp`
before any runtime work. Snapshot + field meanings:
[`docs/OFFSETS.md`](docs/OFFSETS.md)

## Layout

```
CS2/
  README.md
  CMakeLists.txt
  config/settings.json          placeholder
  docs/
    OFFSETS.md                  CheatOffsets reference
    UI.md                       LianFlow trim rules
    ARCHITECTURE.md             planned layers
    BUILD.md                    compile notes
  scripts/build.bat
  src/
    app/main.cpp                trimmed LianFlow window
    sdk/offsets.hpp             dated snapshot + TODOs
    features/                   empty — setup only
  vendor/imgui-lianflow/        ImGui + LianFlow + FreeType + D3DX11
```

## Build

Windows x64, Visual Studio 2022/18 with the C++ workload.

```bat
scripts\build.bat
```

Release binary lands in `build/Release/CS2.exe` (VS generator) or `build/CS2.exe`
(Ninja). See [`docs/BUILD.md`](docs/BUILD.md).

## Scope

This tree is an educational overlay sandbox for a private server where the host
allows it. Do not use it on official matchmaking, Faceit, or any server that
forbids overlays / game modification.

Third-party UI: LianFlow / Dear ImGui. Detours from the original zip were **not**
copied on purpose.
