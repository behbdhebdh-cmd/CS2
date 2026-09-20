# Troubleshooting

[Wiki index](index.md) · [FAQ](faq.md)

## Overlay does not attach

Symptoms: watermark live dot is amber, `-- FPS` / `-- ms`, no boxes.

1. Confirm **Counter-Strike 2** is running (`cs2.exe`), not only Steam.
2. Window title must contain `Counter-Strike 2` (English client title). Other languages: TODO if the title string differs; `find_cs2_cb` is a substring match on that exact English phrase.
3. Start the overlay **after** the game window exists: `D:\CS2\release\CS2.exe`.
4. You launched the **overlay** binary, not a random `CS2.exe` elsewhere.
5. `OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION)` can fail under a higher-integrity game process. TODO: document whether this build must be run as Administrator on your setup; there is no elevation helper in code.

Attach retries every 800 ms. There is no on-screen “waiting for cs2” besides the watermark dot and empty ESP.

## Boxes missing or in the wrong place

1. Settings → Menu: offset status should be `live` or `cache`, not `error`.
2. Compare `Game::build_number()` (engine `dwBuildNumber`) with the CheatOffsets dump. A mismatch means the table is stale even if ESP “draws something”.
3. Press **Refresh offsets**. Check `config/offsets_cache.json` exists after a `200`.
4. You must be **in a match** with a local pawn. Menus / main menu have no useful view matrix / entity list for this reader.
5. **Visible only** + missing `.tri` does **not** hide players (filter is skipped). Visible only + a **wrong** mesh can hide almost everyone.
6. **Enemies only** hides teammates. **Max distance** culls beyond the slider (default 220 m).
7. Do not interpolate projected boxes. If a future change reintroduces position lerp, boxes will lag the pawn.

## Skeleton missing, inverted, or one limb gone

Boxes can be fine while bones look wrong: the AABB is collision, the skeleton is a **resolved** subset of the animation cache.

1. Skeleton checkbox on; body mode **Full skeleton** if you expect legs.
2. `D:\CS2\release\skeleton.log` — look for `resolve layout=` (`feet_origin`, `anim2026`, `geometric`, …) and `joint_mask`. `FAIL array=null` means `m_modelState`/`m_boneArray` did not yield a pointer.
3. A missing arm/leg with a solid torso is usually a **length gate** (world min/max or 420 px on screen), not a total resolve failure.
4. Torso glued to the floor: the resolver should reject “pelvis == origin”; if you still see it, the cache is empty/stale (dormant, wrong pawn).
5. Color (mint vs rose) follows the **player** vis-check, not per-bone walls.

See [ESP](esp.md#skeleton-esp).

## BUILD / dump confusion

`offsets.hpp` baked label and the live engine build are different numbers (dumper “update” vs `dwBuildNumber`). Trust:

- Poller `version` / `kDumpUpdate` after a successful apply
- Engine `dwBuildNumber` read from `engine2.dll`

There is **no** dedicated “BUILD MISMATCH” overlay string in this snapshot of `main.cpp`. TODO: if a banner is added, document it here.

## Visible check always on or always off

| Observation | Likely cause |
| --- | --- |
| Walls do not hide anyone | Mesh not ready (`VisCheck::ready()` false) or **Visible only** unchecked |
| Status would be `missing de_xxx.tri` | Dump maps into `D:\CS2\maps\tri\` ([Getting started](getting-started.md)) |
| `bad de_xxx.tri` | File size not a multiple of 36, or unreadable |
| Almost nobody draws on a known-open angle | Wrong map file, or vis rays to eye/chest both hit (smoke/props in the `.tri`) |
| Map never changes | `detect_map` failed; name scan is heuristic on the globalvars blob |

`g_vis.set_search_dir("D:\\CS2\\maps")` is the hint; `find_maps_root` also tries `<exe>\maps`, `<exe>\..\maps`, cwd.

## Color picker clicks fall through

`WM_NCHITTEST` must treat **popup / tooltip** ImGui windows as `HTCLIENT` via `g_hits`. If a change only hit-tests the 780×540 panel, the picker is unclickable. Use `custom::ColorEdit4` with `picker_flags`, not raw `ImGui::ColorEdit4`.

## Menu does not appear / cannot click

- Insert or F7 (edge trigger). Held Insert does not retrigger.
- Overlay may be on the wrong monitor if CS2 is not found; it then sits on the primary display at full screen.
- `WS_EX_NOACTIVATE` is intentional. Focus stays on the game; that is not a bug.

## Overlay eats game input

Menu is open. ESC or Insert/F7 to close. If input is eaten while closed, `set_passthrough` / `HTTRANSPARENT` is broken — check `WndProc` `WM_NCHITTEST`.

## F8 / Unload

F8 or Settings → Unload overlay stops the process (`done = true`). It does **not** touch `cs2.exe`. If you need to stop a stuck overlay, kill **by path** `D:\CS2\release\CS2.exe` or its PID, never by the name `cs2.exe`.

## Offset poller errors

| Status | What to do |
| --- | --- |
| `error · winhttp open failed` / `connect failed` / `request failed` | Network, TLS, or firewall to `www.cheatoffsets.com` |
| `error · HTTP 0` or other | Non-200/304; keep cache/baked |
| `error · no offsets_flat` | API schema change; check the live JSON |
| Stays `idle · baked` | `start()` never ran (should be after ImGui init) |

Offline is supported: baked header + `offsets_cache.json`.

## Build failures

- Generator `Visual Studio 18 2026` missing: the bat file falls back to Ninja. Install the VS C++ workload if `vswhere` finds nothing.
- Unresolved `d3dx11` / `freetype`: you are not x64, or link directories were edited.
- Added a `.cpp` but the linker misses it: it is not in `CMakeLists.txt`.

Wiki and README edits cannot break the compiler. Do not run `scripts\build.bat` for documentation-only changes.

## Settings reset every launch

Only if you never saved. Settings → Configs → Save writes the full `MenuState` to JSON; Load brings it back. Default just resets RAM.

## Aim slides off the target sideways

That was the shipped yaw-sign bug (`+delta.yaw` instead of `-delta.yaw` in `run_aim`), fixed in the combat pass. If it ever returns: enable Debug log, hold the key, check `configs\aim_debug.log`. Yaw delta and `mx` must oppose; pitch delta and `my` must agree.

## Config buttons clipped or unreachable

Fixed by splitting the tab into Actions / Saved columns with no nested child (the inner list used to swallow wheel events). If the menu runs at minimum height, each column scrolls on its own. Resizing the panel taller also helps; the corner grip is bottom-right.

---

[Getting started](getting-started.md) · [Offsets](offsets.md) · [ESP](esp.md) · [Menu](menu.md)
