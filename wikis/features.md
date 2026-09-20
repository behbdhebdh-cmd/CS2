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
| Overlay attach | auto | `cs2.exe` + window title `Counter-Strike 2` |
| Offset auto-update | auto | [Offsets](offsets.md) |
| Map vis mesh | auto | Load `maps/tri/{map}.tri` on map name change |

## Menu-only (not wired)

These draw in Combat tabs and write `g_menu`, but no code path aims, shoots, or sends input.

| Control | Fields |
| --- | --- |
| Aim enable / visible / recoil | `aim_enable`, `aim_visible`, `aim_recoil` |
| Aim FOV / smooth / bone | `aim_fov`, `aim_smooth`, `aim_bone` (`Head`, `Neck`, `Chest`, `Pelvis`) |
| Aim key | `aim_key`, `aim_key_mode` |
| Trigger enable / delay | `trigger_enable`, `trigger_delay_ms` |
| Trigger key | `trigger_key`, `trigger_key_mode` |

The Combat pages show the text `Not wired yet — visuals first.`

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

- Aimbot / triggerbot runtime
- Chams / glow writing
- Radar, world ESP, bomb ESP, spectator list
- Config Save/Load (`config/settings.json` is a placeholder)
- Sound ESP, third-person, exploits

See [Roadmap](roadmap.md).

---

[ESP](esp.md) · [Menu](menu.md) · [Watermark](watermark.md) · [Architecture](architecture.md)
