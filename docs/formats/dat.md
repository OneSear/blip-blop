# `uk.dat` — strings (text, `txt_data.cpp:29`)

Reader and writer: `readDat` / `writeDat` in `bb/core/assets/legacy_formats.hpp`;
the game now keeps the strings in UTF-8. Source form: `strings/uk.json`
([source-assets.md](source-assets.md)).

Lines `id^text`, Windows-1252, no newline after the last line. The 2002
reader kept up to 1000 ids and 399 bytes per line (a longer line made its
`getline` loop forever). Shipped in 2002: 136 strings; used ids: 1–4, 10–19, 200–213, 300–308, 400–414, 500–507, 520–526, 600–615, 700–708 (dialogue and banner texts per world), 100–108 (bonus-level banners), 800–834 (menus; 828 = "TIME"). Ids 508–519 do not exist although `ennemi_princesse.cpp` requests banners 517 and 519 (D-63).

Phase 2 added the text `game.cpp` printed from literals (data change in
`tools/data/fix_legacy_data.py`): ids 840–849 are the level summary labels
(`LEVEL COMPLETE!`, `PERFECT`, `Casualties`…), ids 1000–1098 the 99 lines of
the credits in display order. 245 strings in all; the game's table now has
2000 entries.
