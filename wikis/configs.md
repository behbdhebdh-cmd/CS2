# Configs

[Wiki index](index.md) · [Combat](combat.md)

`ConfigStore` (`src/app/config.cpp`) persists the whole `MenuState` as version-2 JSON. Settings → Configs tab, two columns: **Actions** on the left (name field, presets, Save, Refresh, Default), **Saved (n)** on the right (Load / Delete per file, scrolls when the list grows).

## Files

Directory resolves at startup, first hit wins: `<exe>\configs`, `D:\CS2\release\configs`, `D:\CS2\configs`. Status line and `config.log` next to the exe confirm which one got used. One file per config: `<name>.json`.

Names sanitize on input: spaces become `_`, anything outside `A–Z a–z 0–9 _ -` drops out, max 48 chars. Empty after sanitize means the Save button asks for a real name instead of writing.

`aim_debug` persists with the rest, so a config can carry its own logging preference. Unknown keys in old files just keep defaults; `version` is informational.

## Presets

One click applies a full combat setup. Visuals, colors, and both keybinds stay untouched; only aim / trigger / humanize / RCS change.

| | Legit | Legit with Aim | Semi Rage |
| --- | --- | --- | --- |
| Aim / Trigger | on / off | on / on | on / on |
| Rifle FOV / smooth | 2.0 / 0.82 | 4.0 / 0.55 | 10.0 / 0.05 |
| Pistol FOV / smooth | 1.6 / 0.82 | 3.0 / 0.50 | 8.0 / 0.05 |
| Sniper FOV / smooth | 1.4 / 0.78 | 2.5 / 0.35 | 7.0 / 0.05 |
| Bones | Head | Head | Head |
| Humanize | on, 90–180 ms, noise 0.22, overshoot 0.14, miss 0.04 | on, 60–130 ms, noise 0.16, overshoot 0.10, miss 0.02 | off, 0–25 ms, noise 0.05 |
| RCS | off (0.0) | on (rifle 2.0, pistol 1.5, sniper 1.0) | aggressive (2.5 everywhere) |
| Trigger hitbox / delays | Head, 80 / 170 / 20 (idle, trigger off) | Head, 70 / 150 / 18, all filters on | Body, 12 / 45 / 6, scope + flash + weapon filter off |
| Vis-check | on everywhere | on everywhere | on everywhere (prioritzed, other filters relaxed) |

Presets never touch your keys. Apply one, tweak a slider, hit Save under your own name and it becomes a regular file like any other.

## Layout note

The tab used to be a single panel with the file list nested inside a second child. Nested scroll containers trap the wheel: hovering the inner list scrolled nothing (empty list has nowhere to go) while the outer panel with the buttons stayed out of reach, so Save / Refresh / Default ended up half-clipped and unclickable. Splitting into two top-level children with no nesting fixed it; each column scrolls on its own and the preset buttons stack vertically so long labels like "Legit with Aim" never clip at the panel edge.

---

[Combat](combat.md) · [Menu](menu.md) · [FAQ](faq.md)
