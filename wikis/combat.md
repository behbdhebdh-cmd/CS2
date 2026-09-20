# Combat

[Wiki index](index.md) · [Features](features.md)

Aimbot and triggerbot live in [`src/features/combat.cpp`](../src/features/combat.cpp). Both are external: they move the mouse with `SendInput`, never write game memory. `combat_tick` runs every frame after ESP, but bails out when the menu is open, the game window is not focused, or the local player is dead.

## Aimbot

Hold the aim key (default **ALT**, `0x12`). Each frame with the key held:

1. `pick_target` scores every enemy by angular distance (`fov_degrees`) from the current view angles and takes the closest inside `prof.fov`. Sticky lock: the current target keeps a 0.72x score bonus and a 1.18x FOV hysteresis, with 90 ms lost-grace before the lock drops.
2. Bone world position comes from resolved `Skel` joints (`Player::joints`), same source as skeleton ESP. Bone order per target depends on the Bone combo (`Head`, `Neck`, `Chest`, `Upper chest`, `Head > chest`, `Head > neck > chest`).
3. `want = calc_angle(local_head, bone)`, minus punch compensation when RCS is on and shots were fired (`want -= punch * rcs`, clamp 0–2.5).
4. Humanize adds drift: lock offset wander, OU noise scaled by the Noise slider, occasional overshoot and miss offsets, plus a lognormal reaction delay (Reaction min/max) on first acquire.
5. `delta = want - view`, scaled by the smoothing factor `a = 1 - exp(-dt / tau)` with `tau = 0.035 + smooth * 0.20`. Smooth 0 skips scaling (raw delta).
6. Delta converts to mouse pixels with the Source mapping (`m_yaw == m_pitch == 0.022`):
   - `dx = -delta.yaw / (sens * 0.022)`
   - `dy = +delta.pitch / (sens * 0.022)`
   
   The minus on yaw is load-bearing: the engine turns yaw down when the mouse moves right, so positive yaw error needs negative mouse movement. An older revision shipped `+delta.yaw` here and the aim mirrored horizontally off the target; pitch was always correct. Sub-pixel remainders accumulate in `g_aim.acc_*`, output clamps to `28 + (1 - smooth) * 36` px per frame.

Sensitivity reads from `dwSensitivity` (+ `0x58`); below 0.05 it falls back to 1.0. Knives, grenades, and the bomb skip aiming entirely.

### Weapon profiles

One `CombatProfile` per class (`aim_rifle`, `aim_pistol`, `aim_sniper`), switched live by `classify_weapon` on the active `m_iItemDefinitionIndex`. The Aim tab edits the profile selected in the Weapon profile combo and shows the class currently in hand.

Defaults: rifle FOV 3.5 / smooth 0.65 / RCS 2.0, pistol 2.4 / 0.45 / 1.2, sniper 2.0 / 0.22 / RCS off. Bones default to Head everywhere.

### FOV circle

`combat_draw` rings the screen center with radius `tan(prof.fov) / tan(cam_fov / 2) * (h / 2)` using the live camera FOV (fallback 90). Toggle with Draw FOV. The ring matches the selection radius, so targets outside the ring never get picked.

### Debug log

Combat → Aim → Humanize → **Debug log**. While on, every 150 ms of aiming appends to `D:\CS2\configs\aim_debug.log` (plus `OutputDebugString`): bone name, eye and bone world positions, view vs want angles, delta before/after smoothing with the `a` factor, FOV, sensitivity, resulting mouse pixels, view-matrix sanity. The aim path never touches `world_to_screen`; the matrix line in the log exists to rule that out when something looks off.

## Triggerbot

Hold the trigger key (default **ALT**). While held, every enemy Hitbox bone projects to an angular size `atan(hitbox_radius / dist)` and fires when the crosshair angle falls inside `size * 1.08`:

| Hitbox setting | Bones tested | Radius |
| --- | --- | --- |
| Head | Head, Neck | 5.4 / 4.8 |
| Chest | SpineUpper, SpineLower | 8.5 / 9.5 |
| Body | all four | as above |

Timing: first shot waits `First delay + jitter`, release lasts 22–42 ms, then `Next delay + jitter` cooldown. Defaults 70 / 155 / 18 ms. Firing uses left mouse down/up through `SendInput`; an already-held physical left button blocks the trigger so the two never fight.

Filters, all default on: Visible only (same BVH vis-check as ESP), Enemies only, scope check for snipers (AWP / SSG 08 / G3SG1 / SCAR-20), anti-flash above 80 alpha, ignore knife / grenades / empty hands.

## Related types

```text
CombatProfile   fov, smooth, bone, rcs_yaw, rcs_pitch
AimBone         Head, Neck, Chest, UpperChest, HeadThenChest, Priority
TriggerHitbox   Head, Chest, Body
WpnClass        Rifle, Pistol, Sniper, Utility
```

---

[Configs](configs.md) · [Menu](menu.md) · [Troubleshooting](troubleshooting.md)
