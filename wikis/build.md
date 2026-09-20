# Build

[Wiki index](index.md) · [Getting started](getting-started.md)

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 or **18** (this machine: Community at `C:\Program Files\Microsoft Visual Studio\18\Community`) with Desktop C++ 
- Windows SDK (comes with VS)
- CMake bundled with VS (`Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`)

x64 only: FreeType is `vendor/imgui-lianflow/freetype/win64`, D3DX libs are `SDK/Lib/x64`. There is no x86 target.

No extra DirectX SDK. LianFlow vendors D3DX11.

Optional:

- .NET 9 SDK — only to rebuild/run `cphys-extractor`
- Network — only for live offset polls (baked + cache work offline)

## One shot

```bat
D:\CS2\scripts\build.bat
```

The script:

1. `cd` to the repo root (`scripts\..`)
2. `vswhere` → latest VS with the VC tools
3. `vcvarsall.bat x64`
4. `cmake -S . -B build -G "Visual Studio 18 2026" -A x64`
5. On generator failure: Ninja + `CMAKE_BUILD_TYPE=Release`
6. `cmake --build build --config Release --parallel`

Output: `D:\CS2\build\Release\CS2.exe`

## Manual

```bat
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cmake -S D:\CS2 -B D:\CS2\build -G "Visual Studio 18 2026" -A x64
cmake --build D:\CS2\build --config Release --parallel
```

Debugger working directory is set to `${CMAKE_SOURCE_DIR}` (`D:\CS2`) so relative `config\` / `maps\` lookups work from VS.

## Runtime copy

This machine runs:

```
D:\CS2\release\CS2.exe
```

After a successful Release build, copy `build\Release\CS2.exe` there. The overlay does not self-copy. `maps\` and `config\` stay in the **project root**, not next to the exe (the code also searches `<exe>\..\maps` and `<exe>\..\config`).

Do not confuse:

| Binary | Path |
| --- | --- |
| Overlay | `D:\CS2\release\CS2.exe` (process name `CS2.exe`) |
| Game | Steam `...\game\bin\win64\cs2.exe` |

## CMake sources and libs

Sources: `src/app/main.cpp`, `src/sdk/{memory,game,vis,offset_update}.cpp`, `src/features/esp.cpp`, ImGui core + FreeType + `imgui_impl_dx11` + `imgui_impl_win32` + `custom_widgets.cpp`.

Include paths: `src/`, LianFlow root, backends, examples, `example_win32_directx11`, FreeType headers, `SDK/Include`.

Defines: `UNICODE`, `_UNICODE`. MSVC runtime: MultiThreaded DLL (`/MD`).

Adding a `.cpp` requires a `CMakeLists.txt` edit and a reconfigure. Do not add feature code only as a header that `main.cpp` includes unless it is already in the target.

## Map extractor

```bat
cd D:\CS2\cphys-extractor
dotnet run -c Release -- --official --tri --out D:\CS2\maps --nopause
```

| Flag | Effect |
| --- | --- |
| `--official` / `--workshop` / `--all` | Which maps |
| `--tri` / `--vphys` / `--both` | Output format |
| `--out <dir>` | Base dir (`tri\` created under it) |
| `--map <name>` | Single map filter |
| `--nopause` | No “press any key” |

Steam/CS2 install is discovered from the registry. On this machine CS2 maps live under `E:\SteamLibrary\...`, not `D:\Steam`.

## What not to do

- Do not vendor Microsoft Detours (intentionally omitted).
- Do not run a Release build just because wiki files changed. Markdown is not compiled.
- Do not kill `cs2.exe` when stopping the overlay; kill `D:\CS2\release\CS2.exe` by path/PID.

---

[Architecture](architecture.md) · [Troubleshooting](troubleshooting.md) · [Offsets](offsets.md)
