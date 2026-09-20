# Features

[Wiki index](index.md)

All feature flags live in `MenuState` (`src/app/settings.hpp`) as `g_menu`. Visuals are consumed by `src/features/esp.cpp`. Combat flags are stored and drawn in the menu only.

## Implemented

| Feature | Default | Notes |
| --- | --- | --- |
| Player ESP | on | [ESP](esp.md) |
| Box styles | Corner | Corner / 3D / Filled |
| Health bar | on, right | Glass bar, optional gradient and numeric value |
| Head marker | on, circle | Circle / Dot / Box |
| Skeleton | on, full, lines+points | Logical joints, vis-colored; [ESP](esp.md#skeleton-esp) |
| Distance label | on | Meters under (or above) the box |
| Enemies only | on | Skips local team when enabled |
| Visible only | on | Needs a ready `.tri` mesh; otherwise the filter is skipped |
| Max distance | 220 m | Fade then cull |
| Thickness / glow / corner length | 11 / 28 / 24 | Thickness is stored in 0.1 px units |
| Box / health / head / skeleton colors | see settings.hpp | `custom::ColorEdit4` in Visuals → Color |
| Watermark | on | [Watermark](watermark.md) |
| Hotkeys panel | on | Floating status panel under the watermark: per-feature switch plus live key pill (ESP, Aimbot, Triggerbot), toggle in Settings → Menu |
| Overlay attach | auto | `cs2.exe` + window title `Counter-Strike 2` |
| Offset auto-update | auto | [Offsets](offsets.md) |
| Map vis mesh | auto | Load `maps/tri/{map}.tri` on map name change |
| Aimbot | hold-key, external mouse | [Combat](combat.md): FOV pick + sticky lock, per-weapon profiles, exponential smoothing, humanize (reaction, noise, overshoot, miss), RCS, FOV ring, debug log |
| Triggerbot | hold-key, external mouse | [Combat](combat.md): Head / Chest / Body hitboxes, first/next/jitter delays, vis + scope + flash + weapon filters |
| Weapon icons | on | Active-weapon glyph under the box, `weapon_icons::resolve` from designer name + item definition |
| Configs | JSON v2 + presets | [Configs](configs.md): Save / Load / Delete / Default, one-click Legit, Legit with Aim, Semi Rage |
| Team names | on | Teammate name in team color over the box, optional dimmed distance below |
| Spectators | panel | Observer-target resolve, eye-header panel with hint when empty |
| Hitmarker | center X | White normal / gold headshot, size + opacity + fade sliders |
| Damage log | killfeed | Health-diff rows with zone dots, kill rows with weapon glyph, max + lifetime + anchor |

## Menu-only (not wired)

Only leftovers without runtime behind them:

| Control | Fields |
| --- | --- |
| Menu key | `menu_key`, `menu_key_mode` (overlay toggle stays Insert/F7) |

## Overlay chrome

| Item | Implementation |
| --- | --- |
| Menu toggle | Insert or F7 (`key_edge`) |
| Hide menu | ESC while open, or the in-panel close button |
| Unload | F8 or Settings → Unload overlay (`g_want_quit`) |
| Click-through | `WS_EX_TRANSPARENT` when the menu is closed; `WM_NCHITTEST` when open |
| Accent color | `c::main_color`, Settings → Menu |
| Open/close motion | Smootherstep scale + fade, ~360 ms open / ~240 ms close |

Details: [Menu](menu.md).

## Not in this tree

Documented as planned only. Do not assume they exist in `src/`.

- Chams / glow writing
- Radar, world ESP, bomb ESP, spectator list
- Sound ESP, third-person, exploits

See [Roadmap](roadmap.md).

---

[ESP](esp.md) · [Menu](menu.md) · [Watermark](watermark.md) · [Architecture](architecture.md)
