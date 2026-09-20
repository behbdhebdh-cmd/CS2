# FAQ

[Wiki index](index.md) · [Troubleshooting](troubleshooting.md)

## Is this an injected cheat?

No. It is a second process with a transparent DX11 window. It reads `cs2.exe` via `ReadProcessMemory`. There is no Detours vendor copy and no write into the game in this tree.

## Where do I run it?

Private bot matches / a personal server where the host allows overlays. The root README forbids official matchmaking, Faceit, and any server that forbids overlays.

## Overlay vs game executable?

| | Overlay | Game |
| --- | --- | --- |
| Process | `CS2.exe` | `cs2.exe` |
| This machine | `D:\CS2\release\CS2.exe` | Steam `game\bin\win64\cs2.exe` |

Always identify the overlay by full path or PID.

## Why Insert and F7?

Both call the same toggle. Insert is the documented default; F7 is a second edge. ESC only hides. F8 unloads. See [Menu](menu.md#keybinds).

## Why is Aim in the menu if it does nothing?

LianFlow pages were kept as a layout placeholder. Checkboxes write `g_menu` only. Combat code is not implemented. See [Roadmap](roadmap.md).

## Does “Visible only” use the GPU / CS2 visleafs?

No. It raycasts a CPU BVH built from extracted `world_physics` triangles. Smokes, players, and some dynamic props are not in that mesh. If the `.tri` is missing, the filter is skipped (everyone can draw).

## Why meters with `* 0.0254`?

Source engine units are inches. ESP distance labels convert to meters for the slider (`Max distance (m)`).

## Do offsets survive a CS2 patch without rebuilding?

Usually yes, if CheatOffsets has a new dump: the worker polls every 10 minutes (or immediately on **Refresh offsets**) and `tick()` patches `offsets::*` in memory. Baked `offsets.hpp` is only the offline fallback. You still need a rebuild if **new fields** must be added to the apply list in `offset_update.cpp`.

## Is `settings.json` my config?

Not yet. The file is a placeholder (`stage: setup-only`). Menu state is RAM-only.

## Why do color pickers need special hit-testing?

The overlay is click-through except for listed rects. ImGui picker windows sit outside the main 780×540 panel. Those windows are appended to `g_hits` as popups/tooltips. See [Menu](menu.md#color-pickers).

## Can I run the overlay before CS2?

Yes. It will retry attach every 800 ms and sit on the primary monitor until a `Counter-Strike 2` window is found, then snap to that client rect.

## Does the watermark show my real city?

It maps Windows timezone / ISO country to a small table (Frankfurt, Berlin, …). It is not a geolocation service and not the CS2 datacenter.

## Where is skeleton ESP?

Implemented. Visuals → Players → **Skeleton**. It does **not** assume cache index 6 is the head. Raw bones are mapped to logical `Skel` joints (`skeleton.hpp`) by scoring several CS2 layouts and a geometric solver. Details: [ESP](esp.md#skeleton-esp).

## Will editing `wikis/` break the build?

No. CMake does not compile Markdown. Do not run a Release build for wiki-only diffs.

---

[Features](features.md) · [Offsets](offsets.md) · [Build](build.md)
