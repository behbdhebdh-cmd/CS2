# Architecture (planned)

Nothing below the UI is implemented. This is the map for later vibe-coding.

```
+----------------------+     +------------------+     +-------------------+
|  Win32 + DX11 window | --> |  ImGui LianFlow  | --> |  MenuState (UI)   |
+----------------------+     +------------------+     +-------------------+
                                      |
                                      v  (not implemented)
                         +---------------------------+
                         |  src/features/*           |
                         |  src/sdk/offsets.hpp      |
                         +---------------------------+
```

## Layers

1. **App** (`src/app/main.cpp`) — window, device, menu.
2. **SDK** (`src/sdk/`) — offset constants only, for now.
3. **Features** (`src/features/`) — empty. Add modules here, not inside `main.cpp`.
4. **Vendor** (`vendor/imgui-lianflow/`) — do not edit unless you are updating ImGui.

## Later (not in this commit)

- Config JSON load/save for `config/settings.json`
- Overlay attach to the game window (this build is a fullscreen sandbox)
- Anything that talks to `cs2.exe` / `client.dll`

Keep feature code out of LianFlow headers so a UI drop-in stays possible.
