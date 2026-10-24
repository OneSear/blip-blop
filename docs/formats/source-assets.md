# Source assets (`assets/source/`)

The editable form of the game data (migration.md §3.11, DEC-04, DEC-07):
PNG images, WAV sounds, Ogg Vorbis/MP3 music, UTF-8 JSON and text. The
tool `bbassets` converts them from and to the 2002 files and builds the
runtime pack ([runtime-pack.md](runtime-pack.md)):

```
bbassets extract <data-dir> <source-dir>   2002 files -> source assets
bbassets pack <source-dir> <pack>          source assets -> runtime pack
bbassets legacy <source-dir> <out-dir>     source assets -> 2002 files
bbassets verify <source-dir> <data-dir>    do the source assets rebuild data-dir?
bbassets pack-legacy <data-dir> <pack>     2002 files -> runtime pack
```

The build runs `bbassets pack` (target `assets_pack`, output
`<build>/blipblop.bbpack`) from `assets/source/`, or, when that directory
does not exist, from a tree it extracts into `<build>/assets-source/`.
Extraction is deterministic: extracting twice gives identical files.

Every JSON file starts with `"format"` and `"version"` (1). Asset names in
the game are the names of the 2002 files; the tree maps them as follows.

| Source | 2002 file / asset name |
|--------|------------------------|
| `sprites/<stem>/sprites.json` + PNG files | `<stem>.gfx` |
| `fonts/<stem>/font.json` + PNG files | `<stem>.lft` |
| `sounds/<stem>/sounds.json` + WAV files | `<stem>.sfx` |
| `music/<stem>.ogg` or `music/<stem>.mp3` | `<stem>.zik` |
| `music/<stem>.playlist.json` | `<stem>.mbk` |
| `levels/<stem>.json` | `<stem>.lvl` |
| `strings/<stem>.json` | `<stem>.dat` |
| `campaign/<stem>.json` | `<stem>.lst` |
| `scripts/<name>.cin`, `scripts/<name>.rpg` (UTF-8) | same name (Windows-1252) |

## Images

```json
{"format": "blipblop-sprites", "version": 1, "images": [
  {"file": "000.png", "hotspot": [3, 3], "layout": "rgb555"}, ...]}
```

`layout` is the 16-bit format the art was authored in: `rgb565` (LGX
version 0: decor screens, briefings, some large props) or `rgb555` (LGX
version 1). PNG files are 8-bit RGBA. Colours are the game's expansion of
the 16-bit value (`component * 255 / max`, rounded down); pixels of the
layout's key colour have alpha 0. Reading a PNG back rounds each opaque
colour to the nearest level of the layout and turns alpha 0 into the key
colour; partly transparent pixels are refused (the art has none). Banks the
game draws without transparency (briefings, credits) therefore show their
few key-coloured pixels as transparent in an image editor.

Fonts: `{"format": "blipblop-font", "version": 1, "lineHeight": 18,
"spaceWidth": 15, "glyphs": [{"char": "A", "file": "0041.png", "layout":
"rgb555"}, ...]}`. `char` is the character the glyph draws (one Unicode
character); the PNG is named after its code point. The 2002 fonts index
glyphs by Windows-1252 byte; the conversion maps bytes to code points
(`0xE9` is `é`, U+00E9).

## Sounds and music

`{"format": "blipblop-sounds", "version": 1, "sounds": [{"file": "00.wav",
"voices": 1}, ...]}`. The WAV files are the bytes of the 2002 banks,
unchanged (PCM mono, 8 or 16 bit, 8,050 to 44,100 Hz).

### Names

An image of a sprite manifest or a sound of a sound manifest can carry a
`"name"` (lower_case: letters, digits, single underscores), e.g.
`{"file": "008.png", ..., "name": "weapon_smg"}`. `bbassets ids
assets/source src/assets/include` writes one header per bank with names:
`bb/assets/sprites/<bank>.hpp` (namespace `bb::assets::sprites::<bank>`,
`constexpr int kWeaponSmg = 8;` and `kImageCount`) and
`bb/assets/sounds/<bank>.hpp` (`bb::assets::sounds::<bank>`, `kSoundCount`).
The headers are committed; `bbassets ids-check` (test `assets.ids`) fails
when they are out of date. Code names the first image of an animation and
adds the step (`kSmurfWalkRight + step`), and the first of a run of
alternative sounds and adds the random choice. `bbassets extract` keeps the
names of a manifest it overwrites when the bank has the same number of
images or sounds.

A playlist track can be named the same way
(`{"file": "data/boss.zik", "name": "boss"}`): `bb/assets/music/<bank>.hpp`
(`bb::assets::music::<bank>`, `kTrackCount`), kept by `bbassets extract`
when the playlist has the same number of tracks.

Music files are copied unchanged; the extension tells the format (20 Ogg
Vorbis files, and the two 2002 MP3 jingles `tambour` and `gameover`).
Playlists: `{"format": "blipblop-playlist", "version": 1, "tracks":
[{"file": "data/snuf.zik"}, ...]}`; a track whose `type` is not 1 (a
tracker module, which the game refuses) carries `"type"`.

## Levels

```json
{
  "format": "blipblop-level", "version": 1,
  "assets": {"decor": "snufdec1.gfx", "props": "snufpplan1.gfx", "enemies": "snufenn.gfx",
             "sounds": "smurf.sfx", "music": "snuf.mbk", "dialogue": "snuff.rpg", "portraits": ""},
  "screens": [0, 1, 2, 3, 4, 5, 6, 5, 6, 7, 8, 9, 9],
  "start": [[66, 454], [147, 454]],
  "victory": {"x": 7037, "flag1": 0, "value1": 7, "flag2": 0, "value2": 7},
  "platforms": [[[0, 103, 453], [103, 126, 454], ...], ... 8 layers],
  "solidWalls": [[row, x, length], ...],
  "bloodWalls": [[row, x, length], ...],
  "events": [
    {"type": "enemy", "activationX": 6874, "enemy": "smurf.farmer", "x": 6824, "y": 445, "facing": 1}, ...],
  "markers": {"smurf.wildMaxX": [5850]}
}
```

* `platforms`: per layer, runs `[x, length, height]` of pixel columns with
  the same platform height; columns outside every run have no platform.
  The 2002 editor painted platforms with a brush, so slopes are many short
  runs (at most 3,788 runs in a level).
* walls: runs `[row, x, length]` of 8×8 cells set to 1 (a fourth element
  gives another cell value; none in the shipped levels).
* `events`: in file order, which is descending `activationX` and must stay
  so (the game wakes events from the head of its list, docs/formats/lvl.md).
* Entities are named as the game's registry names them
  (`EntityRegistry`, `<world>.<entity>`: `smurf.farmer`, `video.rayman`):
  the `enemy` of `enemy` and `enemyGenerator` events, `prop`, `pickup` and
  `vehicle`. A number is accepted too (the 2002 id); `bbassets pack`
  refuses an unknown name, and the pack and the 2002 file store the
  number. `bbassets extract` writes the names.
* `markers` (optional): positions and settings of this level that its
  content reads by name (`World::marker()`), lists of integers, for
  example the Snork in Black's perches or where the stork drops its rider.
  The 2002 `.lvl` format has no place for them: the 2002 data source adds
  the 2002 values (`legacyLevelMarkers()`, `assets_level_markers.cpp`),
  and `bbassets extract` writes them. The test `content` checks that a
  level placing an entity has the markers it reads, and that the data
  directory and the pack agree.

Event fields by type, with the slot of the 2002 record they live in:

| `type` | Fields |
|--------|--------|
| `enemy` | `enemy` (name or id), `x`, `y`, `facing` (value) |
| `enemyGenerator` | `enemy`, `x`, `y`, `facing`, `capacity`, `period`, `temporary` (bool) |
| `animatedProp` | `prop` (name or id), `x`, `y` |
| `midgroundDecal`, `foreground` | `image` (id: the game's image), `x`, `y`, `editorImage` (capacity: the editor's list index) |
| `scrollLock` | `condition` (y), `flag` (capacity), `value` |
| `holdFire`, `setFlag` | `flag` (capacity), `value` |
| `forceScroll` | `speed` (y) |
| `dialogue` | `scene` (id), `condition`, `flag`, `value` |
| `bannerText` | `text` (bank), `condition`, `flag`, `value` |
| `sound` | `sound` (id), `soundBank` (bank), `flag`, `value` |
| `weather` | `kind` (id), `intensity` (y) |
| `overkill` | `enemy` (id), `duration` (capacity) |
| `dynamicLoad` | `id` |
| `music` | `track` (id), `play` (y) |
| `pickupGenerator` | `pickup` (id), `period` |
| `vehicle` | `vehicle` (name or id), `x`, `y`, `direction` (value) |
| `pickup` | `pickup` (name or id), `x`, `y` |

These are the values the game's loader (`Game::chargeNiveau`) or the level
editor read. The 2002 editor was a debug build: the fields it never set
hold the MSVC fill `0xCDCDCDCD`. A named field holding it is `null`; the
game still copies these values, so `null` is written back as the fill. A
record slot that no field names is written as `"_<slot>"` (`_id`, `_x`,
`_bank`, `_y`, `_capacity`, `_value`, `_period`, `_temporary`) when it is
not the fill, and so are a padding or file-name field that is not all
`0xCD` bytes (`_padding`, `_file`, hexadecimal). No shipped level has any
of these.

## Text

* `strings/<stem>.json`: `{"format": "blipblop-strings", "version": 1,
  "strings": [{"id": 1, "text": "..."}, ...]}`, UTF-8. The three
  Windows-1252 apostrophes `0x92` of `uk.dat` become U+2019.
* `campaign/<stem>.json`: `{"format": "blipblop-campaign", "version": 1,
  "entries": [{"briefing": "data/brief1.gfx"}, {"level": "data/snuf1.lvl",
  "kind": "first"}, {"cutscene": "..."}]}`; level kinds are `bonus`,
  `first`, `intermediate`, `last`, `complete`, `end` (docs/formats/lst.md).
* `scripts/`: the cutscene and dialogue scripts, converted to UTF-8 (one
  character changes: an `é` in a comment of `video.rpg`).

## Round trip

`bbassets verify` rebuilds every 2002 file from the source assets and
compares it with the shipped one. On the shipped data: 109 files are
identical byte for byte and 15 have the same content with other bytes, for
documented reasons:

* the 12 levels: the 2002 editor left stack garbage after the NUL of each
  20-byte asset name; it is not kept and written as zeros;
* 3 music lists (`inter.mbk`, `interl.mbk`, `snuf.mbk`) end with a newline,
  which the writer does not reproduce (the other 9 have none).

The `assets.verify` test runs this check on every build of the tests.
