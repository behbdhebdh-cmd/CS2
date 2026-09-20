# Credits

[Wiki index](index.md)

This overlay is a private bot-match project. Names below are **sources in the tree**, not an endorsement and not a claim of authorship over upstream projects.

## UI and graphics

| Piece | Source | Notes |
| --- | --- | --- |
| Dear ImGui | `vendor/imgui-lianflow/` (MIT) | Core widgets, DX11/Win32 backends |
| ImGui LianFlow | Original archive `E:\ImGui-LianFlow.zip`, vendored copy | Custom widgets, blur, tabs, fonts. Base only — see [Menu](menu.md) |
| FreeType | `vendor/imgui-lianflow/freetype/` | Font raster; keep upstream license |
| D3DX11 | `vendor/imgui-lianflow/SDK/` | Headers/libs; x64 only |
| Poppins / icomoon | Embedded in LianFlow examples | Loaded from memory in `main.cpp` |

See [`vendor/imgui-lianflow/NOTICE.md`](../vendor/imgui-lianflow/NOTICE.md). Do not present LianFlow as an original UI kit in public materials.

Detours was **not** copied from the LianFlow zip on purpose.

## Game data

| Piece | Source |
| --- | --- |
| Offset dumps | [a2x/cs2-dumper](https://github.com/a2x/cs2-dumper) (baked header comment) |
| Hosted dumps + API | [cheatoffsets.com](https://www.cheatoffsets.com/) — CS2 page `/g/cs2`, API `/api/games/cs2/current` |
| Optional mirror | [sezzyaep/CS2-OFFSETS](https://github.com/sezzyaep/CS2-OFFSETS) |

Field meanings: [Offsets](offsets.md). Always verify the pinned build before trusting a paste.

## Visibility meshes

| Piece | Source |
| --- | --- |
| Extractor | `cphys-extractor/` (MIT README), .NET 9 |
| Parsing | [ValveResourceFormat](https://github.com/ValveResourceFormat/ValveResourceFormat), [ValvePak](https://github.com/SteamDatabase/ValvePak) |
| Mesh use | `world_physics.vmdl_c` → binary `.tri` (9 floats / triangle) |
| Ray/triangle | Möller–Trumbore in `VisMesh::ray_tri` |

CS2 map files stay in the Steam install. This repo only stores generated `.tri` locally; they are not a substitute for owning the game.

## Project

| | |
| --- | --- |
| Tree | `D:\CS2` |
| Runtime copy | `D:\CS2\release\CS2.exe` |
| GitHub (private) | `https://github.com/behbdhebdh-cmd/CS2.git` |

Older scaffold notes remain under [`docs/`](../docs/). Prefer this wiki when they disagree.

---

[Architecture](architecture.md) · [Build](build.md) · [Getting started](getting-started.md)
