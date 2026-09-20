# Offset update module

Runtime poller that keeps `offsets::*` in sync with
[CheatOffsets](https://www.cheatoffsets.com/api-docs) without rebuilding.

Files:

- `../offset_update.hpp` — singleton API (`start` / `tick` / `stop` / `request_poll`)
- `../offset_update.cpp` — WinHTTP client, JSON `offsets_flat` parser, apply
- `../offsets.hpp` — baked fallback table; fields are `inline std::ptrdiff_t`, not `constexpr`

## API

| | |
| --- | --- |
| Base | `https://www.cheatoffsets.com` |
| Game slug | `cs2` |
| Endpoint | `GET /api/games/cs2/current` |
| Auth | none |
| Docs | https://www.cheatoffsets.com/api-docs |

The JSON body includes `version`, `updated_at`, nested `offsets`, and
**`offsets_flat`**: a single-level map of offset names to hex strings
(`"0x2577BE0"`). This module applies `offsets_flat`. If that object is missing
it flattens `offsets` with dotted keys (`client_dll.dwEntityList`).

Hex strings are parsed with or without the `0x` prefix.

## ETag polling

Every successful `200` stores the `ETag` header (CheatOffsets derives it from
build id + last-modified). The next request sends:

```
If-None-Match: W/"b42-…"
```

| Status | Meaning | Action |
| --- | --- | --- |
| 200 | New dump | Parse `offsets_flat`, apply on the render thread, save cache |
| 304 | Unchanged | Keep the current table, empty body, no apply |
| other | Network / CF / API error | Keep baked or last-good values, surface status |

Suggested interval from the API docs is 5–15 minutes. This build uses **10
minutes** (`600000` ms). A Settings button forces a poll immediately.

## Poll loop

```
start()
  load config/offsets_cache.json if present → apply
  spawn worker

worker
  GET /api/games/cs2/current  (+ If-None-Match when we have an ETag)
  if 200 → queue pending map
  if 304 → status only
  Wait 10 minutes  (or wake on Refresh / shutdown)

tick()  // each overlay frame, main thread
  if pending → copy values into offsets::client / engine2 / schema
```

Apply happens on the **main thread** so `game.cpp` never races a half-updated
table. Missing keys are left at the previous / baked value.

## Integration

`src/app/main.cpp`:

```
OffsetUpdate::instance().start();   // after overlay init
OffsetUpdate::instance().tick();    // each frame, next to Game::tick
OffsetUpdate::instance().stop();    // on F8 / quit
```

`game.cpp` keeps reading `offsets::client::dwEntityList` etc. Those symbols are
now mutable and pick up API updates automatically.

Cache path (first existing directory wins):

- `D:\CS2\config\offsets_cache.json`
- `<exe>\..\config\offsets_cache.json`

Status line lives in Settings → Menu (`live · <version> · N fields`,
`etag 304 · … unchanged`, or `error · …`).

## Key matching

Flat keys are compared case-insensitively. Each runtime field tries:

1. Qualified dumper name (`client_dll.dwEntityList`, `C_BaseEntity.m_iHealth`)
2. Bare name (`dwEntityList`, `m_iHealth`)
3. Any key whose dotted suffix matches

That covers both `offsets_flat` layouts CheatOffsets has used (module prefix
vs. class prefix).
