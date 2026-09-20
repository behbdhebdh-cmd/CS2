# Roadmap

[Wiki index](index.md) · [Features](features.md)

Only items that already have UI, types, or an explicit gap in this tree. Anything else is TODO without a design.

## Done since the last wiki snapshot

| Item | Where |
| --- | --- |
| Skeleton ESP | [ESP](esp.md#skeleton-esp) — logical joints, layout scoring, geometric fallback |
| Aimbot | [Combat](combat.md) — hold-key, FOV + sticky pick, per-weapon profiles, smoothing, humanize, RCS, FOV ring, debug log |
| Aim yaw-sign fix | `combat.cpp`: `dx` uses `-delta.yaw`; the old `+delta.yaw` mirrored horizontally off target |
| Triggerbot | [Combat](combat.md) — hitbox angular test, first/next/jitter timing, vis + scope + flash + weapon filters |
| Weapon icons | ESP draws the live weapon glyph per player |
| Config Save/Load + presets | [Configs](configs.md) — JSON v2, Legit / Legit with Aim / Semi Rage |

Aim and trigger stay external (`SendInput` mouse). Reads remain read-only RPM; nothing writes game memory.

## Not started (named because they are commonly next)

| Item | Status |
| --- | --- |
| Chams | No material/hook/write path. TODO |
| Glow | `dwGlowManager` is in the offset table only. TODO |
| Radar | Removed from the current tab list (older `docs/UI.md` still mentions it). TODO |
| Menu keybind widget | `menu_key` on `MenuState` unused; toggle is Insert/F7. TODO |
| Offset vs engine build banner | `build_number()` is read but not drawn. TODO |
| World / bomb / weapon ESP | `dwPlantedC4` unused. TODO |
| Spectator list / name ESP | Controller name is read only for local watermark. TODO |

## Explicit non-goals (this tree)

- Official matchmaking / Faceit
- Microsoft Detours / injection
- Fortnite leftovers that were stripped from LianFlow (vehicles, chests, floor loot)

## Suggested order (not a schedule)

1. Surface offset/engine build on Settings or watermark.
2. Round out ESP (bomb, world, names) before new input tricks.
3. Only then consider anything that writes game memory; external mouse covers aim + trigger today.

---

[ESP](esp.md) · [Menu](menu.md) · [Architecture](architecture.md)
