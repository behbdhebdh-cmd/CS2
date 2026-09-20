# Getting started

[Wiki index](index.md)

External overlay: a separate Win32 + DirectX 11 process (`CS2.exe`) that reads `cs2.exe` with `ReadProcessMemory` and draws on a transparent topmost window aligned to the game client.

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 or 18 (Community is fine) with the Desktop C++ workload
- CMake bundled with Visual Studio
- Counter-Strike 2 installed and launchable
- For visibility checks: .NET 9 SDK (to run `cphys-extractor`) and a one-time map dump

No extra DirectX SDK install. LianFlow already vendors D3DX11 headers/libs under `vendor/imgui-lianflow/SDK/`.

## Clone / tree

Project root on this machine: `D:\CS2`.

```
D:\CS2\
  CMakeLists.txt
  scripts\build.bat
  src\
  vendor\imgui-lianflow\
  config\
  maps\tri\          generated locally, not a substitute for the game
  release\CS2.exe    copy you actually run
  wikis\
```

Private GitHub origin (do not put tokens in the remote URL): `https://github.com/behbdhebdh-cmd/CS2.git`

## Build

From the project root:

```bat
scripts\build.bat
```

That configures with generator `Visual Studio 18 2026` (`-A x64`) and builds **Release**. If that generator is missing, the script falls back to Ninja.

Output: `D:\CS2\build\Release\CS2.exe`

Copy used at runtime on this machine:

```
D:\CS2\build\Release\CS2.exe  ->  D:\CS2\release\CS2.exe
```

Details: [Build](build.md).

## Map meshes (visibility)

ESP “Visible only” needs collision triangles next to the overlay, not inside the Steam install.

```bat
cd D:\CS2\cphys-extractor
dotnet run -c Release -- --official --tri --out D:\CS2\maps --nopause
```

Expected files: `D:\CS2\maps\tri\de_mirage.tri`, `de_dust2.tri`, … (9 floats per triangle, 36 bytes each).

The overlay searches `D:\CS2\maps`, `<exe>\..\maps`, and a few relatives. See [ESP — visible check](esp.md#visible-check).

## Start a private bot match

1. Launch **Counter-Strike 2** (`cs2.exe`). The overlay looks for a visible window whose title contains `Counter-Strike 2`.
2. Create a **private lobby / practice with bots**. Do not use official matchmaking or third-party anti-cheat clients with this overlay.
3. Join the map. Wait until you are in-game (alive pawn, view matrix valid).
4. Start the overlay **after** the game window exists:

   ```
   D:\CS2\release\CS2.exe
   ```

   Working directory should be the project root if you want relative `config\` paths to resolve; absolute fallbacks (`D:\CS2\config`, `D:\CS2\maps`) also exist.

5. Press **Insert** or **F7** to open the menu. **ESC** hides it. **F8** unloads the overlay.

The overlay process name is `CS2.exe`. The game process is `cs2.exe` under the Steam library (on this machine: `E:\SteamLibrary\steamapps\common\Counter-Strike Global Offensive\game\bin\win64\cs2.exe`). Never kill the Steam game process when you only meant to stop the overlay.

## First-run checks

| Check | Where |
| --- | --- |
| Overlay attached | Watermark live dot is green when `Game::attached()` is true ([Watermark](watermark.md)) |
| Offsets | Settings → Menu status line (`live · …`, `etag 304 · …`, or `error · …`) |
| Map mesh | Visible-only ESP only filters after the `.tri` BVH is ready |
| Click-through | With the menu closed, mouse and keyboard go to the game |

## What is not set up yet

- `config/settings.json` is **not** loaded or saved at runtime. Menu values live in `g_menu` for the process lifetime only. TODO: persist settings.
- Aim and Trigger tabs do not write input. TODO: see [Roadmap](roadmap.md).

---

[Features](features.md) · [Menu](menu.md) · [Troubleshooting](troubleshooting.md) · [FAQ](faq.md)
