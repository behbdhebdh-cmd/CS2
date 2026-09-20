# Misc: Team, Spectators, Hitmarker, Damage-Log

[Wiki index](index.md) · [Features](features.md)

Second-wave modules under the **Misc** tab (`Team`, `Hitlog`). Same rules as everything else: external only, `SendInput` for input, read-only RPM, watermark-style panels.

## Team ESP names

Visuals pipeline, no new snapshot data. When **Team names** is on, teammates draw even with "Enemies only" active: box in `box_team`, name above the box in team color, same font size, gap, and centering as enemy names. Optional **Team distance** prints the meters below the box, dimmed to 75% alpha.

Deliberately quiet: no health bar, no head marker, no weapon icon on teammates. Skeleton still draws (it already color-codes vis state). Everything persists in configs like the rest of `MenuState`.

## Spectators

`Game::tick` resolves observer targets for dead controllers: pawn → `m_pObserverServices` → `m_hObserverTarget` → pawn, kept when it equals the local pawn (max 10 names). Offsets live in `offsets.hpp` (`C_BasePlayerPawn::m_pObserverServices`, `CPlayer_ObserverServices::m_hObserverTarget`) with baked fallbacks, and the poller applies API values by suffix match, so a CS2 patch that moves them heals itself on the next dump.

Panel (`src/features/spec.cpp`): hotkeys-lookalike, eye glyph + "Spectators" header, green dot per name. Empty list keeps the panel with a dimmed "No spectators" hint instead of vanishing. Position is anchor (corners) plus X/Y nudge, all in Misc → Team.

If the observer offsets go stale after a patch, the list just stays empty; ESP, aim, and trigger are unaffected since they never touch that path.

## Hitmarker

Center-screen X of four short lines, fired only for outgoing damage (see attribution below). Dark outline pass under the color pass so it reads on any background. Size slider (6–20, default 14), Thickness slider (1–5, default 3), Opacity (default 1.0), Fade (150–500 ms, default 300). Envelope is 40 ms fade-in then linear fade. Headshots keep their own color (default gold, picker) plus a short scale punch (~1.5x decaying over ~70 ms).

## Damage log

`hitlog_tick` polls per-pawn health each frame (`src/features/hitlog.cpp`), but a row only lands in the feed when the damage is attributable to the local player:

- Victim must be an enemy. Teammates, the local pawn, incoming hits, and environment damage never create entries.
- Health polling cannot see the shooter, so attribution uses two windows: 600 ms after any local trigger pull (`m_iShotsFired` edge) or 800 ms of aim lock on that pawn. No window, no entry.
- Kills (pawn vanishes within 2 s of its damage) need the same proof: a shot within 900 ms or a lock on that pawn within 1000 ms. The killing blow already fired the marker, so kills only add the feed row with the current weapon glyph.

Cap: newest N rows (`Max entries` 1–8), each living `Lifetime` seconds (2–8) plus a 0.8 s fade. Rows slide up 10 px on arrival, so fast successive hits stay readable.

Zones are a heuristic, not server hitgroups: damage on the recently locked pawn takes the locked bone (Head/Neck → head, SpineUpper → chest, else body); everything else counts as body. Dot colors come from three pickers. Crossfire while dead never lands in the feed; the tracker re-baselines instead. Knife and grenade damage skips the shot window (no `m_iShotsFired` edge) and only logs while that pawn is aim-locked.

Panel anchors like spectators (default bottom-right), plus X/Y nudge and a kill-icon toggle.

---

[Combat](combat.md) · [Configs](configs.md) · [Menu](menu.md)
