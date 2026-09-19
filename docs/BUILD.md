# Build

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 or 18, Desktop C++ workload
- Windows SDK (comes with VS)
- CMake (bundled with VS is enough)

No extra DXSDK install: LianFlow's D3DX11 headers/libs are under
`vendor/imgui-lianflow/SDK/`.

## One shot

```bat
D:\CS2\scripts\build.bat
```

The script runs `vcvarsall x64`, then CMake (`Visual Studio 18 2026` generator,
falls back to Ninja).

## Manual

```bat
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cmake -S D:\CS2 -B D:\CS2\build -G "Visual Studio 18 2026" -A x64
cmake --build D:\CS2\build --config Release --parallel
```

Output: `D:\CS2\build\Release\CS2.exe`

## Run

`CS2.exe` opens a fullscreen DX11 sandbox with the LianFlow menu. It does not
attach to CS2. Insert toggles the menu. Alt+F4 / close the process to exit.

## Notes

- x64 only (`freetype\win64` + `SDK\Lib\x64`).
- Detours was left out of vendor on purpose.
- `imgui.ini` is disabled (`io.IniFilename = nullptr`).
