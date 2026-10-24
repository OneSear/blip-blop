# Third-party code

Vendored as single headers, unmodified. Allowed by migration.md DEC-11.
None of them is linked into the game: doctest is used by the tests, stb
and nlohmann/json by the asset tools in `tools/` only.

| Directory | Library | Version | Source | Licence |
|-----------|---------|---------|--------|---------|
| `doctest/` | doctest | 2.4.11 | https://github.com/doctest/doctest | MIT |
| `stb/` | stb_image 2.30, stb_image_write 1.16 | commit `2c980bb59875b0d32144a71867fbdebb2f77cd20` | https://github.com/nothings/stb | MIT or public domain (`stb/LICENSE`) |
| `nlohmann/` | nlohmann/json | 3.11.3 (`single_include/nlohmann/json.hpp`, SHA-256 `9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6`) | https://github.com/nlohmann/json | MIT |
