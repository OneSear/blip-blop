# `.lvl` — level (`game.cpp:567`, written by `BB3_level_editorDlg.cpp:2435`)

| Field | Type | Meaning |
|-------|------|---------|
| 7 × `name[20]` | char[20] each | asset file names, NUL-terminated, **garbage after the NUL** (editor writes an uninitialised 100-byte stack buffer; bytes `0xCC` observed): decor bank, level/prop bank, enemy bank, sound bank, music bank, dialogue script, dialogue portrait bank. Empty string = none. Game prefixes `data/`. |
| `screenCount` | int32 | level width in 640-px screens |
| `screenIndex[screenCount]` | int32 | which decor image to show for each screen (tiles reused) |
| `start1X, start1Y, start2X, start2Y` | int32 ×4 | player spawn points |
| `victoryX, flag1, value1, flag2, value2` | int32 ×5 | victory when camera X ≥ `victoryX` and `flag[flag1] == value1` and `flag[flag2] == value2` |
| `platformY[8][screenCount × 640]` | int32 | per-pixel platform height per layer; `0xFFFF` (65535) = none; layers 6–7 are slippery (`couille.cpp:642`) |
| `solidWall[60][width / 8]` | uint8 (bool) | 8×8-pixel cells blocking movement |
| `bloodWall[60][width / 8]` | uint8 (bool) | cells that accept blood decals |
| `eventCount` | int32 | |
| `events[eventCount]` | 60 bytes each | `FICEVENT` records sorted by `activationX` in **descending** order (verified: no ascending adjacent pair in any of the 12 levels) |

`FICEVENT` (`fic_events.h`), 60 bytes as laid out by MSVC x86: `eventId`, `activationX`, `id`, `x`, union{`pictureBank`,`soundBank`,`textId`}, union{`y`,`condition`,`speed`,`intensity`,`play`}, union{`capacity`,`flag`,`duration`}, union{`value`,`facing`,`direction`}, `period` (9 × int32), `temporary` (bool, 1 byte + 3 padding), `file[20]`. Events wake when `activationX <= cameraX`. The loader prepends each record to the sleeping list (`SuperListe::ajoute`), so the descending file order becomes ascending in memory, and `Game::updateEvents()` (called from `updateAll`) wakes events from the head only until the first one that is not ready — the order is load-bearing and any converter must preserve it. The editor derives `activationX` from the object's X with per-type offsets (`BB3_level_editorDlg.cpp:2573–2946`): enemies −750 when facing left, +50 when facing right; generators −800 / +50 likewise; props, foregrounds, vehicles and pickups −740; midground decals −660; locks, hold-fire, scroll speed, dialogue, flags, texts, sounds, weather, music and pickup generators use X unchanged.

Event IDs: 0 enemy, 1 enemy generator, 2 animated prop, 3 midground decal, 4 foreground, 5 scroll lock, 6 hold fire, 7 force scroll speed, 8 dialogue, 9 set flag, 10 banner text, 11 sound, 12 weather, 13 overkill (unused), 14 dynamic load (unused), 15 music, 16 pickup generator, 17 vehicle, 18 pickup. Scroll-lock release conditions: 0 no enemies, 1 no generators, 2 flag == value, 3 flag ≥ value.

Facts measured on the 12 shipped levels (Phase 2): the editor was an MSVC
debug build, so every field it never set holds the heap fill `0xCDCDCDCD`
(bytes `0xCD`): the union slots an event type does not use, `period` outside
generators, `temporary` outside generators (`0xCD`; generators have 0 or 1),
the 3 padding bytes and the 20-byte `file` of every record. Some of these
values are copied by the loader into the game's state (for example the flag
and value of a scroll lock whose condition is 0 or 1), so converters keep
them. The bytes after each asset name's NUL are `0xCC` (stack fill) mixed
with the end of earlier names; no code reads them. Midground decals and
foregrounds store the editor's own image index in the `capacity` slot (the
editor reads it back; the game reads `id`). Platforms were painted with a
brush (`BB3_level_editorDlg.cpp`, `TOOL_PLATEFORME`): heights are arbitrary
per pixel column, and runs of equal height number 9 to 3,788 per level.

The level editor reads `.lvl` files back (`BB3_level_editorDlg.cpp:3040`):
the format is also its project format.

Flags: 0–3 user script registers (meaning differs per level), 4 bubbles (underwater), 5 pickup mode (selects a drop table in `make_bonus.cpp`), 6 timer, 7 generators off, 8 kill count, 9 enemy count, 10 generator count.
