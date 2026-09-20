# Menu

[Wiki index](index.md) · [Features](features.md)

The panel is **ImGui LianFlow** (`vendor/imgui-lianflow`), trimmed in [`src/app/main.cpp`](../src/app/main.cpp). Treat the kit as a visual base, not a second product. Feature logic stays in `src/features` and `src/sdk`.

## Window and input

| Item | Value |
| --- | --- |
| Overlay class / title | `CS2Overlay` / `CS2` |
| Style | `WS_POPUP` + `WS_EX_TOPMOST | LAYERED | TOOLWINDOW | NOACTIVATE | TRANSPARENT` |
| Size | Synced to the CS2 client rect every 250 ms; else primary monitor |
| Menu size | `c::bg::size` = **780×540**, rounding 18 (`imgui_settings.h`) |
| imgui.ini | Disabled (`io.IniFilename = nullptr`) |
| Keyboard nav | Off |

While the menu is **closed**, `WS_EX_TRANSPARENT` is set and `WM_NCHITTEST` returns `HTTRANSPARENT`. The game keeps mouse and keyboard.

While it is **open**, passthrough is cleared, but hits are still limited: only `g_hits[]` rectangles return `HTCLIENT`. That list is the main panel plus active ImGui **popup / tooltip** windows (color pickers). Clicks outside those rects go through to the game.

`WM_MOUSEACTIVATE` returns `MA_NOACTIVATE` so opening the menu does not steal focus from CS2.

## Open / close animation

There is no separate splash / startup cinematic. Motion is the panel show/hide:

| | Open | Close |
| --- | --- | --- |
| Duration | ~0.36 s | ~0.24 s |
| Curve | Smootherstep (`t^3 (t (6t − 15) + 10)`) | same |
| Scale | 0.965 → 1.0 about the panel center | reverse |
| Offset | 16 px downward while hidden | reverse |
| Alpha | style `Alpha` + window shadow 8→28 | reverse |

Implemented with `g_menu_anim`, `scale_menu_windows("CS2", pivot, scale)` on the ImGui window named `CS2`. Background blur uses LianFlow `draw_background_blur`.

## Tabs

```
Visuals    Players     (tab index 0)
Combat     Aim         (1)
           Trigger     (2)
Settings   Menu        (3)
           Configs     (4)
Misc       Team        (5)
           Hitlog      (6)
```

Tab indices are sequential across all sub-tabs (`c_tabs`). Adding or removing a name in `tabs_info` shifts later `IsTabActive(n)` checks.

### Visuals → Players

Left child **Box**: ESP toggles and sliders, including **Skeleton**, body mode (`Head only` / `Upper body` / `Full skeleton`), style (`Lines` / `Points` / `Lines + points`), thickness.

Right child **Color**: box / health / head / **Skeleton visible** / **Skeleton hidden**.

See [ESP](esp.md#skeleton-esp).

### Combat → Aim / Trigger

Wired since the combat pass. Left child holds the enable/key/visibility/profile sliders, right child the humanize + RCS block and the debug-log toggle. Full math: [Combat](combat.md).

### Settings → Menu

- Accent color (`c::main_color`)
- Watermark checkbox → [Watermark](watermark.md)
- Hotkeys panel checkbox (`misc_hotkeys`, persisted in configs): floating ESP / Aimbot / Triggerbot status with live key pills, drawn under the watermark on the background draw list
- Offset dump meta + poller status
- **Refresh offsets** → `OffsetUpdate::request_poll()`
- **Unload overlay** → `g_want_quit`

### Settings → Configs

Two columns, same halves as every other tab. Left (**Actions**): name field, vertical preset stack, Save, Refresh + Default, save preview. Right (**Saved (n)**): one Load / Delete row per file, scrolls on its own. Details: [Configs](configs.md).

### Misc → Team / Hitlog

Left children hold the ESP-side toggles (**Team names**, **Team distance**) and the **Hitmarker** block (enable, size, opacity, fade, both colors). Right children hold the two floating panels: **Spectators** and **Damage log**, each with enable, corner anchor, X/Y nudge, plus zone colors and kill-icon toggle for the log. Details: [Misc](misc.md).

## Keybinds

Hard-coded overlay keys (not the LianFlow `Keybind` widgets):

| Key | Action |
| --- | --- |
| Insert or F7 | Toggle `g_menu_open` |
| ESC | Hide menu if open (does not unload) |
| F8 | Unload overlay |

Footer text on the panel repeats: `INSERT / F7 hide · ESC close · F8 unload`.

`custom::Keybind` on Aim / Trigger writes `aim_key` / `trigger_key` and mode, and `combat_tick` reads them every frame through `GetAsyncKeyState`. Defaults are both ALT (`0x12`). `menu_key` exists on `MenuState` but is unused; overlay toggle is Insert/F7 only. TODO: bind menu toggle to `g_menu.menu_key` if that is intended.

## Color pickers

Use **`custom::ColorEdit4`**, not raw `ImGui::ColorEdit4`.

Flags (`picker_flags` in LianFlow `main.h`):

```
ImGuiColorEditFlags_NoSidePreview
ImGuiColorEditFlags_AlphaBar
ImGuiColorEditFlags_NoInputs
ImGuiColorEditFlags_AlphaPreview
```

Popup hit-testing is required. If `WM_NCHITTEST` only tests the main 780×540 rect, picker windows are `HTTRANSPARENT` and clicks fall through. `g_hits` / `g_hitn` collect popup and tooltip `ImGuiWindow` rects after the menu is drawn (cap 24).

## Branding

| Piece | Where |
| --- | --- |
| Header wordmark | `"CS2"` + `ICON_FIRE_FILL` |
| Accent | `c::main_color` (also Settings → Accent) |
| Fonts | Poppins (Regular/Medium/SemiBold/Bold) + icomoon, FreeType |
| Present | `Present(0, 0)` — no vsync |

## State

`g_menu` (`MenuState`) lives in RAM and persists through `ConfigStore` JSON files plus the three built-in presets. See [Configs](configs.md).

---

[ESP](esp.md) · [Watermark](watermark.md) · [Offsets](offsets.md) · [Troubleshooting](troubleshooting.md)
