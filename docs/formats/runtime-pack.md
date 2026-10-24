# Runtime pack (`.bbpack`), version 2

The file the game loads its assets from when started with `--pack <file>`
(migration.md Phase 2, DEC-03). Built by `bbassets pack` from the source
assets ([source-assets.md](source-assets.md)), or by `bbassets pack-legacy`
from a 2002 `data/` directory. Reader and writer:
`src/core/include/bb/core/assets/pack.hpp`.

One pack holds every asset of the game. Each asset keeps the name of the
2002 file it replaces, lower-case (`snufenn.gfx`, `uk.dat`, `snuf.zik`), so
the game asks for the same names whatever the storage. An index lets the
game read one asset, or one image of a sprite bank, without reading the
rest: decor screens can be loaded one at a time on small machines.

All integers are little-endian. Every payload starts on a 4-byte boundary.
Nothing is compressed with a general-purpose codec: sprites use the span
encoding below, sounds and music keep their WAV, Ogg Vorbis or MP3 bytes.

## Layout

| Offset | Type | Field |
|-------:|------|-------|
| 0 | char[4] | magic `BBPK` |
| 4 | u32 | version, 2 (2: level markers) |
| 8 | u32 | asset count A |
| 12 | u32 | chunk count C |
| 16 | u32 | offset of the asset table (A records of 32 bytes) |
| 20 | u32 | offset of the chunk table (C records of 32 bytes) |
| 24 | u32 | offset of the name blob |
| 28 | u32 | size of the name blob |

The writer places the payloads first, then the names, then the two tables;
a reader must use the offsets.

**Asset record** (32 bytes), sorted by name hash, then name:

| Type | Field |
|------|-------|
| u32 | FNV-1a (32-bit) hash of the name |
| u32 | name offset in the name blob |
| u32 | name length (bytes, no terminator) |
| u32 | type (below) |
| u32 | first chunk |
| u32 | chunk count |
| i32 | parameter 0 |
| i32 | parameter 1 |

**Chunk record** (32 bytes): u32 payload offset, u32 payload size, then six
i32 values whose meaning depends on the asset type.

The reader rejects a file whose index points outside the file, whose names
do not match their hashes or are not sorted, or whose version is not 2.

## Asset types

| Type | Asset | Parameters | Chunks | Chunk values |
|-----:|-------|------------|--------|--------------|
| 1 | sprite bank (`.gfx`) | — | one per image, in bank order | width, height, hotspot x, hotspot y, layout (0 RGB565, 1 RGB555), 0 |
| 2 | font (`.lft`) | line height, space width | one per glyph, by code point | width, height, 0, 0, layout, Unicode code point |
| 3 | sound bank (`.sfx`) | — | one per sound: a RIFF/WAVE file | voices (always 1 in the shipped data), 0… |
| 4 | music list (`.mbk`) | — | one: list (below) | — |
| 5 | music (`.zik`) | — | one: the Ogg Vorbis or MP3 file | — |
| 6 | level (`.lvl`) | — | one: level (below) | — |
| 7 | campaign (`.lst`) | — | one: list (below) | — |
| 8 | string table (`.dat`) | — | one: table (below) | — |
| 9 | script (`.cin`, `.rpg`) | — | one: UTF-8 text | — |

A *string* inside a payload is a u32 byte length, the UTF-8 bytes, and
zero padding to a multiple of 4 bytes.

### Images (sprites and glyphs): spans

The pixels keep their 16-bit values in the image's layout. The payload is
a table of `height` u32 row offsets (from the start of the payload), then
each row as a list of operations that cover the row's width exactly:

| Operation (u16) | Meaning |
|-----------------|---------|
| `00` + length (bits 13–0) | skip: pixels of the layout's key colour |
| `01` + length, then `length` u16 pixels | copy |
| `10` + length, then one u16 pixel | fill |

The top two bits are the kind, the low 14 bits the length (1–16383). The
key colours are RGB565 (30, 52, 18) and RGB555 (30, 25, 18). Decoding
writes the key colour into skipped pixels, so the 16-bit image comes back
exactly; whether the key is transparent is the caller's choice, as with the
2002 banks (briefings are loaded opaque). The writer emits a fill for runs
of 3 or more equal pixels and copies shorter runs.

Measured on the 3,755 shipped images: 68.3 MB as spans, against 91.3 MB as
LGX and 118.6 MB uncompressed. Spans also let a renderer skip transparent
pixels without testing them (migration.md §3.14).

### Level payload

| Field | Type |
|-------|------|
| 7 asset names: decor, props, enemies, sounds, music, dialogue, portraits | string ×7 |
| screen count, then the decor image of each screen | u32, i32 × count |
| start 1 x, y, start 2 x, y, victory x, flag 1, value 1, flag 2, value 2 | i32 × 9 |
| 8 platform layers: run count, then runs (x, length, height) | u32, (i32 ×3) × count |
| solid walls, then blood walls: run count, then runs (row, value, x, length) | u32, (u16 ×4) × count |
| event count, then events | u32, 40 bytes × count |
| marker count, then markers by name: name, value count, values | u32, (string, u32, i32 × count) × count |

Platform columns not covered by a run have no platform (`0xFFFF` in the
`.lvl` file). Wall cells not covered are 0. An event is the first 40 bytes of
the 2002 record: type, activation x, id, x, the four union slots (bank, y,
capacity, value), period (i32 ×9), the `temporary` byte, and 3 padding
bytes written as zeros. The record's 20-byte file name and the content of
its padding are not stored: no code reads them. Values the 2002 editor never set are stored as they
are (`0xCDCDCDCD`), because the game copies some of them into its state.
Events keep the file's order (descending activation x).

### Lists

* music list: u32 count, then per track i32 type (1 = streamed file) and
  the path string (`data/snuf.zik`);
* campaign: u32 count, then per entry i32 kind (0 cutscene, 1 level,
  2 briefing), i32 level kind, path string;
* string table: u32 count, then per string i32 id and the text string,
  in id order.

## Checks

* `tests/formats/pack_test.cpp` writes every shipped asset to a pack and
  compares what the pack gives back with the 2002 files.
* The `replay.*.pack` tests replay the three regression replays with
  `--pack`: the fingerprints must equal those of the runs from `data/`.
