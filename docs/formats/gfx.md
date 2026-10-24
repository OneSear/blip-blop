# `.gfx` — sprite bank (`picture_bank.cpp:61`, written by `GFX_EditorDlg.cpp:1094`)

| Field | Type | Meaning |
|-------|------|---------|
| `count` | int32 | number of images |
| repeated `count` × | | |
| `hotspotX`, `hotspotY` | int32 ×2 | drawing origin relative to the image's top-left |
| `size` | int32 | byte size of the following LGX blob |
| `lgx` | bytes | one LGX image (A.2) |

Images are addressed by index only; the bank has no names and no offset table (sequential load, no streaming). Transparency is a colour key applied after decoding: for LGX v1 banks RGB (246,205,148), for v0 banks RGB (246,210,148) — these are the *decoded* values of the authoring key (250,206,152)/(250,214,152) after RGB555/RGB565 quantisation, i.e. the key is really "RGB555 (30,25,18)" / "RGB565 (30,52,18)". Banks loaded with `trans = false` (briefings, credits, "the end") have no key.
