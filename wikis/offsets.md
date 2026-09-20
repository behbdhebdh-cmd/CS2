# Offsets

[Wiki index](index.md) · [Architecture](architecture.md)

Runtime table: [`src/sdk/offsets.hpp`](../src/sdk/offsets.hpp). Fields are `inline std::ptrdiff_t`, not `constexpr`, so the poller can overwrite them.

Baked fallback in this tree: **a2x/cs2-dumper 2026-09-10 12:36 UTC**, dump label **14181**. Live values come from CheatOffsets. Module details also live in [`src/sdk/offset_update/README.md`](../src/sdk/offset_update/README.md).

## Why they exist

`dw*` values are **RVAs**: add to the loaded module base (`client.dll`, `engine2.dll`). Schema fields (`m_iHealth`, …) are offsets from a class instance.

Wrong numbers look “fine” (boxes in the wrong place, empty player list, garbage build). Always compare `engine2.dll` `dwBuildNumber` (read into `Game::build_number()`) with the dump you think you have.

## API

| | |
| --- | --- |
| Base | `https://www.cheatoffsets.com` |
| Endpoint | `GET /api/games/cs2/current` |
| Auth | none |
| Client | WinHTTP, user agent `CS2OffsetPoll/1.0` |
| Timeouts | 8 / 8 / 10 / 10 s |
| Docs | https://www.cheatoffsets.com/api-docs |
| Human page | https://www.cheatoffsets.com/g/cs2 |

JSON includes `version`, `updated_at`, nested `offsets`, and **`offsets_flat`**: name → hex string (`"0x2577BE0"`). This build applies `offsets_flat`. If that object is missing it flattens `offsets` with dotted keys (`client_dll.dwEntityList`).

Hex is parsed with or without a `0x` prefix. Unicode `\uXXXX` in strings is stored as `?` (not needed for offset values).

## ETag polling

Interval: **10 minutes** (`600000` ms). Suggested range from the API docs is 5–15 minutes.

Every successful `200` stores the `ETag` header. The next request sends:

```
If-None-Match: <etag>
Accept: application/json
```

| Status | Meaning | Action |
| --- | --- | --- |
| 200 | New dump | Parse, queue apply, write cache |
| 304 | Unchanged | Keep table, empty body |
| other | Network / API error | Keep baked or last-good; status `error · …` |

**Refresh offsets** in Settings → Menu sets `force_poll_` and signals the worker wait event so a poll runs immediately.

## Poll loop

```
start()                         // after overlay init
  load config/offsets_cache.json if present → apply on this thread
  spawn worker

worker
  GET /api/games/cs2/current  (+ If-None-Match)
  if 200 → pending_ map + version + updated_at
  if 304 → status only
  Wait 10 minutes  (or wake on Refresh / shutdown)

tick()                          // each overlay frame, main thread
  if pending → copy into offsets::client / engine2 / schema

stop()                          // F8 / quit
```

Apply is **main-thread only** so `game.cpp` never races a half-updated table. Missing keys keep the previous / baked value.

## Key matching

Flat keys are compared case-insensitively. Each field tries:

1. Qualified dumper name (`client_dll.dwEntityList`, `C_BaseEntity.m_iHealth`)
2. Bare name (`dwEntityList`, `m_iHealth`)
3. Any key whose dotted suffix matches

That covers both `offsets_flat` layouts CheatOffsets has used.

## Cache

Path (first existing **directory** wins):

- `D:\CS2\config\offsets_cache.json`
- `<exe>\..\config\offsets_cache.json`
- `<exe>\config\offsets_cache.json`
- `config\offsets_cache.json`

Written on `200`. Contains `etag`, `version`, `updated_at`, `offsets_flat`. Loaded in `start()` before the first HTTP call so a launch without network still uses the last good dump.

`offsets_cache.json` is a runtime file. Do not commit secrets into it; the API is unauthenticated, but the file can be large.

## Integration

[`src/app/main.cpp`](../src/app/main.cpp):

```
OffsetUpdate::instance().start();   // after D3D/ImGui init
OffsetUpdate::instance().tick();    // each frame, next to Game::tick
OffsetUpdate::instance().stop();    // on unload
```

Status strings (Settings → Menu):

| Example | Meaning |
| --- | --- |
| `idle · baked` | Worker not running / initial |
| `cache · <version>` | Loaded from disk |
| `polling · cheatoffsets.com` | Request in flight |
| `fetched · applying` | Body parsed, waiting for `tick()` |
| `live · <version> · N fields` | Applied `N` matching keys |
| `etag 304 · <version> unchanged` | Not modified |
| `error · …` | WinHTTP, HTTP status, or missing `offsets_flat` |

Dump meta on the same page: `offsets::kDumpUtc`, `kDumpUpdate`, `kDumpSource` (`baked` or `api`).

## Manual fallback

If the API is down:

1. Open https://www.cheatoffsets.com/g/cs2
2. Confirm the pinned build number
3. Paste into `src/sdk/offsets.hpp` (fallbacks only)
4. Rebuild

Mirrors (still verify build number): [a2x/cs2-dumper](https://github.com/a2x/cs2-dumper), [sezzyaep/CS2-OFFSETS](https://github.com/sezzyaep/CS2-OFFSETS)

Constants that the poller does **not** overwrite (compile-time): `entity_system` stride `0x70`, handle mask `0x7FFF`, `life::kAlive`, `flags::FL_DUCKING`, `globalvars` frame fields.

## What `Game` actually reads

Used today: entity list, view matrix, global vars (FPS + map scan), local pawn/controller, scene node origin/dormant, health/team/flags/velocity, collision mins/maxs, view offset, player name, ping, pawn handle / alive.

Present in the header but **not** referenced by `game.cpp` in this snapshot: `dwViewAngles`, `dwCSGOInput`, `dwPlantedC4`, `dwGlowManager`, `dwGameRules`, buttons, `dwSensitivity`, several engine2 net-client fields. TODO: document them here when a feature starts using them.

---

[Build](build.md) · [Troubleshooting](troubleshooting.md) · [FAQ](faq.md)
