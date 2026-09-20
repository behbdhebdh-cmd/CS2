# ESP

[Wiki index](index.md) · [Features](features.md)

Player ESP is `draw_players` in [`src/features/esp.cpp`](../src/features/esp.cpp). It reads the current `Game` snapshot and `VisCheck`, then draws on ImGui’s background draw list. Boxes are **not** position-lerped; they project the pawn AABB from the live view matrix each frame.

Skeleton ESP is implemented. Boxes, health, head marker, and bones share the same player filter. Drawing uses **logical joints** (`Skel`), not raw CS2 cache slots.

## Pipeline

1. `Game::tick()` fills `players_` (alive, non-dormant, team 2 or 3, not local). See [Architecture](architecture.md).
2. `draw_players` skips the player when:
   - `g_menu.vis_enable` is false
   - `vis_team_check` and `p.team == local_team`
   - distance in meters `>= vis_max_distance`
   - visible-only is on **and** the mesh is ready **and** both vis rays miss
   - fewer than 4 AABB corners project, the 2D box is tiny, or it is fully off-screen
3. Remaining players get a box, optional health bar, distance text, optional head marker, optional skeleton.

Distance for ESP is `p.distance * 0.0254f` (Source units to meters). Opacity uses a smoothstep fade from 72% of max distance to the max, plus a small speed and pulse term.

## Box styles

`BoxStyle` in `settings.hpp`, combo **Style** on Visuals → Players.

| Value | Name | Draw |
| --- | --- | --- |
| 0 | Corner Box | Four framed L-shaped corners (`frame_corner`) |
| 1 | 3D Box | 12 edges of the world AABB if all 8 corners projected; otherwise falls back to corners |
| 2 | Filled Box | Vertical gradient fill **plus** corner box |

Thickness (`vis_thickness`, 8–28, stored in 0.1 px units) and glow (`vis_glow`, 10–90) feed line width and shadow. Corner length (`vis_corner`, 16–42) is a percent of the shorter box side, clamped.

2D bounds come from projecting `Player::corners[8]` (origin ± half extents, z 0 and height). A few pixels of padding are added after projection.

## Health bar

Enabled with **Health bar**. Vertical glass bar beside the 2D box.

| Control | Field | Behavior |
| --- | --- | --- |
| Health position | `vis_health_position` | Left or Right (default Right) |
| Health width | `vis_health_width` | 2–8 px |
| Health gradient | `vis_health_gradient` | Lerp `health_low` → `health_high` by HP fraction; off uses high color only |
| Health value | `vis_health_value` | Integer percent next to the bar |

Fill height is `hp * box_height`. A light inner shimmer uses `sin(time)`. HP comes from `Player::health / max_health` (pawn `m_iHealth`, fallback controller `m_iPawnHealth`).

## Head marker

Enabled with **Head marker**. Projects `p.eye + (p.head - p.eye) * 0.45f`.

| Style | Draw |
| --- | --- |
| Circle (default) | Filled circle + ring + shadow |
| Dot | Smaller filled disc |
| Box | Rounded square |

**Head size** (`vis_head_size`, 5–20) scales radius from the 2D box width. Colors: `head_enemy` / `head_team`.

`Player::head` is origin + collision height. `Player::eye` is origin + view-height estimate (46 ducked / 64 standing) when `m_vecViewOffset` is not used on the remote pawn. Local eye uses `m_vecViewOffset` when its length is in `(8, 90)`.

## Distance

**Distance** draws `{N}m` centered under the box, or above it if the label would leave the display. Uses the same alpha as the box.

## Visible check

**Visible only** (`vis_visible_only`) is applied only when `VisCheck::ready()` is true. If the mesh is missing or still loading, the filter is **off** (players are not hidden).

For each player, two segments are tested from `Game::local_head()`:

- to head: resolved `Skel::Head` if present, else `p.eye`
- to chest: resolved `Skel::SpineUpper` if present, else `p.origin + (0, 0, ducked ? 32 : 48)`

The player is skipped only if **both** return not visible.

Implementation: [`src/sdk/vis.cpp`](../src/sdk/vis.cpp)

| Piece | Detail |
| --- | --- |
| Mesh file | `maps/tri/{sanitized_map}.tri` |
| Triangle | 9 floats (3× `Vec3`), 36 bytes, no header |
| BVH | Longest-axis split, `std::nth_element` on vertex-sum, leaf size 8, max depth 28 |
| Ray/tri | Möller–Trumbore |
| AABB | Slab test, 0.08 unit pad |
| Near skip | Hits closer than 1.6 units ignored |
| Far pad | Trace length is segment length minus 5 units |

Map name is scanned from the `dwGlobalVars` blob (preferred pointer offsets, then a wider scan) and sanitized (`de_mirage`, strip `maps/`, `.vpk`, etc.). Load happens on a worker thread.

Generate meshes: [Getting started](getting-started.md#map-meshes-visibility). Extractor: `cphys-extractor` (ValveResourceFormat + ValvePak) from `world_physics.vmdl_c`.

## Filters

| Filter | Default | Rule |
| --- | --- | --- |
| Enable | on | Master ESP switch |
| Enemies only | on | Drop teammates |
| Visible only | on | Drop if both vis rays hit world geometry (mesh ready) |
| Max distance (m) | 220 | Cull at limit; fade from 72% of limit |

Entity-side filters (always on, not in the menu): dead, dormant, local pawn/controller, team not 2/3, invalid collision.

## Colors

Visuals → Color, `custom::ColorEdit4` with `picker_flags` (`NoSidePreview | AlphaBar | NoInputs | AlphaPreview`).

| Label | Field | Default (approx.) |
| --- | --- | --- |
| Enemy | `box_enemy` | cool grey, 0.92 alpha |
| Team | `box_team` | light blue, 0.75 alpha |
| Health low | `health_low` | red |
| Health high | `health_high` | green |
| Head enemy | `head_enemy` | orange |
| Head team | `head_team` | light blue |
| Skeleton visible | `skeleton_visible` | mint |
| Skeleton hidden | `skeleton_hidden` | rose |

Accent for the **menu** is separate (`c::main_color`). See [Menu](menu.md).

## Skeleton ESP

On by default (`vis_skeleton`). Visuals → Players: **Skeleton**, body mode, style, thickness. Colors: **Skeleton visible** / **Skeleton hidden** (same vis-check as the box, not per-bone walls).

### What changed vs the first bone pass

The first version treated CS2 cache indices as if they were CS:GO-style (`Head = 6`, `Chest = 2`). That is wrong for current player models: helper, twist, and weapon bones sit in the array, **slot 8 is not “left shoulder”**, and `SpineLower`/`Chest` both used index `2` so the torso collapsed.

The fix does **not** draw raw slots. It:

1. Reads up to 28 cache entries (`stride 0x20`) from `CSkeletonInstance` + `CModelState::m_boneArray` (`0x80`), via the scene node, then the body-component skeleton if needed (`m_modelState` baked `0x140`).
2. Maps those points onto 17 **logical joints** (`enum class Skel` in [`src/sdk/skeleton.hpp`](../src/sdk/skeleton.hpp)).
3. Picks a mapping by scoring several known layouts **plus** a geometric solver, then drops segments whose world length is impossible.

`Player` stores `joints[kSkelCount]` and `joint_mask`, not a 28-slot bone array.

### Logical joints

```text
Head, Neck, SpineUpper, SpineLower, Pelvis
LShoulder, LElbow, LHand
RShoulder, RElbow, RHand
LHip, LKnee, LFoot
RHip, RKnee, RFoot
```

Chains (also the draw graph):

```text
Pelvis → SpineLower → SpineUpper → Neck → Head
Neck → LShoulder → LElbow → LHand
Neck → RShoulder → RElbow → RHand
Pelvis → LHip → LKnee → LFoot
Pelvis → RHip → RKnee → RFoot
```

### Resolve (`skel_resolve`)

Each layout is a `SkelLayout`: cache index per logical joint. Candidates, in order of consideration (best score wins):

| Name | Idea |
| --- | --- |
| `feet_origin` | CS2 pawn origin at the feet; pelvis is **not** slot 0 |
| `anim2026` | Newer animgraph packing |
| `anim_clav` | Clavicle slots before the arm chain |
| `classic28` | Old 28-bone CS-style packing (Head=6, arms 8–15, legs 22–27) |
| `legs18` | Legs starting at 18 |
| `geometric` | No index map: highest bone near origin = head, pelvis band at origin.z+20…48, then nearest unused points along the spine / out to arms and down to legs |

Scoring (`skel_score_layout`):

- Reward filled joints and segments whose length is inside `kSkelSegs` (e.g. neck–head 2–28, hip–knee 4–65).
- Reward head.z > neck.z > spine > pelvis (standing).
- **Penalize** a “pelvis” sitting on the origin (that is the root/foot bone, not the hips).
- Penalize knees/feet above the hip (inverted legs).
- Reward a sane shoulder span (~6–55 units).

If the best score is `< 20`, no joints are published (`joint_mask = 0` → nothing drawn). After the winner is chosen, any child whose parent segment length is still illegal is cleared from the mask.

Debug: `skel_log` (`src/sdk/skel_log.hpp`) writes `D:\CS2\release\skeleton.log` (layout name, mask, raw deltas). Not shown in the overlay.

### Draw (`draw_skeleton`)

Same player already passed box filters. Per segment:

1. Skip if body mode is too low (`Head only` / `Upper body` / `Full skeleton`).
2. Skip if either joint missing.
3. Skip if world length outside the same min/max as resolve.
4. `world_to_screen` both ends (`w < 0.001` → behind camera → skip that segment only).
5. Skip if the **screen** length is > 420 px (stops a single bad W2S from spanning the HUD).
6. Outline + colored line. Optional joint dots; head dot is skipped when the head marker is on.

Color is `skeleton_visible` or `skeleton_hidden` from the **player** vis-check (eye/chest rays), not a ray per bone.

RPM for bones runs only when `vis_enable && vis_skeleton` (`Game::tick(true)` from `main.cpp`).

### Why it can still look wrong

Documented so a “broken arm” is not treated as a missing feature:

| Symptom | Typical cause |
| --- | --- |
| No skeleton, boxes OK | Resolve score `< 20`, dormant, or bone pointer null (`m_modelState` / `m_boneArray` stale) |
| Torso on the floor | Layout still picked the root bone as pelvis (should be heavily penalized now) |
| Missing limb | That child failed the length gate or W2S; the rest of the chain still draws |
| Rubber-band while strafing | Cache is the game’s bone array, not a separately interpolated pose |
| Green through a crate | Color is player vis, not per-bone vis |

This is closer to a `BoneJointList` + cached positions pipeline than to “index 6 = head forever”. The extra work vs a fixed 30-bone dump is the **layout contest + length gates**, which is what stopped the CS2 helper-bone packing from drawing garbage.

## Related types

```text
Player          origin, head, eye, mins/maxs, corners[8],
                joints[17], joint_mask,
                health, max_health, team, distance, speed, ducked
Skel            Head … RFoot (logical; see skeleton.hpp)
BoxStyle        Corner, Box3D, Filled
HealthBarPosition  Left, Right
HeadMarkerStyle Circle, Dot, Box
SkeletonBodyMode   Head, Upper, Full
SkeletonStyle      Lines, Points, LinesAndPoints
```

---

[Menu](menu.md) · [Watermark](watermark.md) · [Architecture](architecture.md) · [Troubleshooting](troubleshooting.md)
