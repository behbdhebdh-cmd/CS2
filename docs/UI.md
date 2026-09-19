# LianFlow UI base

This project uses **ImGui LianFlow** as the visual base.

Original archive: `E:\ImGui-LianFlow.zip`  
Vendored copy: `vendor/imgui-lianflow/`

## What to keep

- Custom widgets (`custom::Checkbox`, `SliderInt`, `Combo`, `Keybind`, `Child`, …)
- FreeType fonts + icomoon icons
- DX11 blur (`directx_blur.h` / `pshader.hpp`)
- Tab chrome (`c_tabs` in `main.h`)
- ESP *preview* painter in `esp.h` (layout mock, not a game reader)

## What was already removed from the stock example

The zip is a generic cheat-menu demo. A lot of it is noise for CS2.

| Removed | Why |
| --- | --- |
| Discord avatar + WinINet / json loader | unrelated |
| AuthGuards comments | unrelated |
| Duplicate "Backup Aim" page | demo clutter |
| Close Aim / Weapon Config demo pages | unused until you need them |
| World ESP: vehicles, chests, ammo boxes, floor loot | Fortnite leftovers |
| Exploits: no bloom, vehicle speed, first person | not CS2 |
| Microsoft Detours | not needed for an external overlay setup |

## What is left (edit in `src/app/main.cpp`)

```
Combat     Aim, Trigger
Visuals    Players, Radar
Misc       General
Settings   Menu, Config
```

Every checkbox is local `MenuState`. Nothing is bound to CS2.

## How to strip more

1. Open `src/app/main.cpp`.
2. Delete a name from the `tabs_info` vector.
3. Delete the matching `p_tabs.IsTabActive(n)` block. Tab indices are sequential
   across all sub-tabs (Aim=0, Trigger=1, Players=2, …).
4. Delete the unused field from `MenuState`.
5. Rebuild.

Do **not** restyle from scratch. LianFlow already is the look. Only add a
control when you are about to implement it.

## Branding

| Piece | Where |
| --- | --- |
| Window class / title | `src/app/main.cpp` (`CS2Setup` / `CS2 Overlay Setup`) |
| Header wordmark | `"CS2"` next to `ICON_FIRE_FILL` |
| Accent | `c::main_color` in `imgui_settings.h`, also editable in Menu tab |
| Menu size | `c::bg::size` (850×596) in `imgui_settings.h` |
