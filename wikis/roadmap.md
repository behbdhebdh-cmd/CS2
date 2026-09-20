# Roadmap

[Wiki index](index.md) · [Features](features.md)

Only items that already have UI, types, or an explicit gap in this tree. Anything else is TODO without a design.

## Done since the last wiki snapshot

| Item | Where |
| --- | --- |
| Skeleton ESP | [ESP](esp.md#skeleton-esp) — logical joints, layout scoring, geometric fallback |

## UI exists, runtime does not

Combat tab widgets write `g_menu` and show `Not wired yet — visuals first.`

| Item | Fields today | Missing |
| --- | --- | --- |
| Aimbot | `aim_enable`, `aim_visible`, `aim_recoil`, `aim_fov`, `aim_smooth`, `aim_bone`, `aim_key`, `aim_key_mode` | Angle write or mouse move; use resolved `Skel` (already on `Player`); FOV check; recoil source |
| Triggerbot | `trigger_enable`, `trigger_delay_ms`, `trigger_key`, `trigger_key_mode` | Crosshair / vis test; `attack` button or mouse; delay timer |

`offsets.hpp` already has `dwViewAngles` and `dwCSGOInput`; `game.cpp` does not read them yet.

This overlay is **read-only** RPM today. Any aim/trigger design must state whether it stays external (mouse) or starts writing memory. TODO: choose that before implementing.

## Not started (named because they are commonly next)

| Item | Status |
| --- | --- |
| Chams | No material/hook/write path. TODO |
| Glow | `dwGlowManager` is in the offset table only. TODO |
| Radar | Removed from the current tab list (older `docs/UI.md` still mentions it). TODO |
| Config Save/Load | `config/settings.json` placeholder; no code. TODO |
| Menu keybind widget | `menu_key` on `MenuState` unused; toggle is Insert/F7. TODO |
| Offset vs engine build banner | `build_number()` is read but not drawn. TODO |
| World / bomb / weapon ESP | `dwPlantedC4` unused. TODO |
| Spectator list / name ESP | Controller name is read only for local watermark. TODO |

## Explicit non-goals (this tree)

- Official matchmaking / Faceit
- Microsoft Detours / injection
- Fortnite leftovers that were stripped from LianFlow (vehicles, chests, floor loot)

## Suggested order (not a schedule)

1. Persist `g_menu` + accent to JSON.
2. Surface offset/engine build on Settings or watermark.
3. Only then Combat, with a written input method. Aim can reuse `Player::joints` / `Skel::Head`.

---

[ESP](esp.md) · [Menu](menu.md) · [Architecture](architecture.md)
