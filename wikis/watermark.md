# Watermark

[Wiki index](index.md) · [Features](features.md)

Status bar drawn by `draw_watermark` in [`src/features/esp.cpp`](../src/features/esp.cpp) on the background draw list. Toggle: Settings → Menu → **Watermark** (`g_menu.misc_watermark`, default on).

Position: top-left of the overlay client (`16, 14`), height 38 px, rounded 11, semi-transparent fill plus a light shadow. It is **not** click-tested (`g_hits` only covers the menu and popups), so it never captures input.

## Segments (left to right)

| Segment | Source | Display |
| --- | --- | --- |
| FPS | `Game::fps()` | `{n} FPS` or `-- FPS` |
| Ping | `Game::local_ping()` | `{n} ms` or `-- ms` |
| Session time | `ImGui::GetTime()` from first watermark draw | `mm:ss` or `h:mm:ss` |
| Region | Windows geo + timezone map | e.g. `Frankfurt am Main, DE` |
| User | `Game::local_name()`, else `GetUserNameA` | in-game name or Windows user |
| Live dot | `Game::attached()` | green if attached, amber if not |

Icons are small filled shapes drawn next to each label (bars, signal, clock, pin). Separators are 1 px vertical lines.

## FPS

`Game::tick()` samples `CGlobalVarsBase`:

- Instant: `1 / m_flAbsoluteFrameTime` when the value is in `(0.0008, 0.2)`
- Fallback: `m_nFrameCount` delta over ≥ 180 ms wall time, used when the instant sample is unusable
- Display: exponential smooth (`0.82 / 0.18`) as an integer

Color: green at ≥ 120, default text at ≥ 60, yellow below 60. `-- FPS` when not attached or no valid sample.

This is the **game** frame estimate, not the overlay present rate.

## Ping

`CCSPlayerController::m_iPing` on the local controller. Accepted range `(0, 1000)`. Otherwise `-- ms`.

Color: green `< 45`, yellow `< 80`, red at 80+, muted grey when unknown.

## Region (“Ort”)

`status_region()` (once per process):

1. `GetDynamicTimeZoneInformation` → `TimeZoneKeyName`
2. Match a small table (W. Europe → Frankfurt, Central Europe → Berlin, GMT → London, …)
3. Else `GetUserGeoID` + `GEO_ISO2` (two-letter country)

This is the **Windows** locale, not a CS2 server region and not GPS. TODO: if a real server/region string is added later, replace this table.

## Username

Prefer `CBasePlayerController::m_iszPlayerName` on the local controller. If empty, dereference `CCSPlayerController::m_sSanitizedPlayerName`. Trailing control characters stripped. If still empty, `GetUserNameA` (fallback `"player"`).

## Live dot

Right edge of the bar. `Game::attached()` is true after a successful `Memory::attach(L"cs2.exe")` plus non-zero `client.dll` and `engine2.dll` bases. Attach is retried at most every 800 ms while detached.

## Related

- Overlay attach and entity read: [Architecture](architecture.md)
- Menu checkbox: [Menu](menu.md)
- Offset / build metadata is **not** on the watermark; it lives in Settings → Menu ([Offsets](offsets.md))

---

[ESP](esp.md) · [Menu](menu.md) · [Troubleshooting](troubleshooting.md)
