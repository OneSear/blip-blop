# `.lft` — bitmap font (`fonte.cpp:52`, written by `FNT EditorDlg.cpp:527`)

| Field | Type | Meaning |
|-------|------|---------|
| `lineHeight` | int32 | vertical advance for multi-line text |
| `spaceWidth` | int32 | advance for `' '` |
| for `code = 1 … 255` | | glyph by single-byte code (Windows-1252/Latin-1) |
| `size` | int32 | 0 = no glyph |
| `lgx` | bytes | LGX image if `size > 0` |

Glyph advance = glyph width; hotspot (0,0); colour key (246,205,148). Code 0 has no slot.
