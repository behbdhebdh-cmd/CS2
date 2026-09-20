# CS2

**Wiki:** [wikis/index.md](wikis/index.md) — getting started, ESP, menu, offsets, architecture, build, troubleshooting.


## Status

| Area | State |
| --- | --- |
| DX11 overlay | click-through when menu is closed |
| LianFlow menu | Insert / F7 toggle, ESC hide, F8 unload |
| ESP | Corner / 3D / Filled + glass health |
| Colors | RGB sliders in the Visuals panel |
| Vis check | BVH raycast against `.tri` meshes in `maps/tri/` |
| Offsets | Auto-poll from CheatOffsets API (ETag, 10 min) |
| Aim / trigger | UI only, not wired |

**Insert** opens the menu. While it is closed the overlay does not eat mouse or
keyboard input. While it is open, only the panel itself is interactive; the rest
of the game stays clickable.

## Automatic offset updates

Offsets are no longer a hand-pasted header you have to refresh after every CS2
patch. On startup the overlay:

1. Loads the last good dump from `config/offsets_cache.json` (if present).
2. Polls `GET https://www.cheatoffsets.com/api/games/cs2/current`.
3. Applies `offsets_flat` (name → hex) to the live offset table.
4. Stores the response `ETag` and sends `If-None-Match` on the next poll.
5. Repeats every **10 minutes**. HTTP **304** means “nothing changed” — the
   current table stays as-is, no download.

Baked values in `src/sdk/offsets.hpp` remain the offline fallback. Settings →
Menu shows poll status and a **Refresh offsets** button.

Module details: [`src/sdk/offset_update/README.md`](src/sdk/offset_update/README.md)

## Map meshes

Generate once, locally (not in git):

```bat
cd cphys-extractor
dotnet run -c Release -- --official --tri --out D:\CS2\maps --nopause
```

## UI base: ImGui LianFlow

The menu is **ImGui LianFlow** (`vendor/imgui-lianflow`). Treat that kit as a
**base**, not a finished product.

Current tabs:

```
Visuals    Players
Combat     Aim, Trigger
Settings   Menu
```

Details: [`docs/UI.md`](docs/UI.md)

## Offsets

Source of truth: **https://www.cheatoffsets.com/api**  
CS2 current dump: **https://www.cheatoffsets.com/api/games/cs2/current**  
Human page: **https://www.cheatoffsets.com/g/cs2**

Field meanings: [`docs/OFFSETS.md`](docs/OFFSETS.md)

## Layout

```
CS2/
  README.md
  wikis/                    developer wiki (start at wikis/index.md)
  CMakeLists.txt
  config/
    settings.json
    offsets_cache.json      written at runtime (ETag + offsets_flat)
  docs/
  scripts/build.bat
  src/
    app/main.cpp
    sdk/
      offsets.hpp           baked fallback, mutated at runtime
      offset_update.hpp/.cpp
      offset_update/README.md
    features/
  vendor/imgui-lianflow/
```

## Build

Windows x64, Visual Studio 2022/18 with the C++ workload.

```bat
scripts\build.bat
```

Release binary: `build/Release/CS2.exe`. Copy used by this machine:
`D:\CS2\release\CS2.exe`. See [`docs/BUILD.md`](docs/BUILD.md).

## Scope

Educational overlay sandbox for a private server where the host allows it.
Do not use it on official matchmaking, Faceit, or any server that forbids
overlays / game modification.
