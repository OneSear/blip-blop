# File formats

All integers are 32-bit little-endian unless stated; strings are single-byte Windows-1252 (Latin-1 plus the `0x92` apostrophe); "the editors" are the MFC tools in `vc-projects/`. Reader locations name the 2002 sources (`vc-projects/Blip_n_Blop_3/` at commit `44cab36`, in the git history) so the specs can be checked against them; the readers in use are in `src/core/src/assets_*.cpp`.

These pages were extracted from Appendix A of [migration.md](../../migration.md) and are maintained here from now on.

- [`.gfx` — sprite bank (`picture_bank.cpp:61`, written by `GFX_EditorDlg.cpp:1094`)](gfx.md)
- [LGX image (`LGXpacker::loadLGX`, encoders in `winLGX.cpp` of the GFX and FNT editors)](lgx.md)
- [`.lft` — bitmap font (`fonte.cpp:52`, written by `FNT EditorDlg.cpp:527`)](lft.md)
- [`.sfx` — sound bank (`sound_bank.cpp:84`, written by `SFX EditorDlg.cpp:688`)](sfx.md)
- [`.mbk` — music bank (text, `music_bank.cpp:30`)](mbk.md)
- [`.lvl` — level (`game.cpp:567`, written by `BB3_level_editorDlg.cpp:2435`)](lvl.md)
- [`bb.lst` — campaign (text, `game.cpp:3449`)](lst.md)
- [`uk.dat` — strings (text, `txt_data.cpp:29`)](dat.md)
- [`.cin` — cutscene script (text, `cine_player.cpp`)](cin.md)
- [`.rpg` — dialogue script (text, `rpg_player.cpp`)](rpg.md)
- [`bb.cfg` (binary, 61 bytes, `config.cpp`)](cfg.md)
- [`bb.scr` (binary, 244 bytes, `hi_scores.cpp`)](scr.md)
- [Editor-side formats](editor-formats.md)
- [Runtime pack (`.bbpack`, Phase 2)](runtime-pack.md)
- [Source assets (`assets/source/`, Phase 2)](source-assets.md)

Phase 2 put a reader (and a writer where the tools need one) for every
format in `src/core/include/bb/core/assets/legacy_formats.hpp`; the game
reads all its assets through them. Each page states the facts measured on
the shipped files.
