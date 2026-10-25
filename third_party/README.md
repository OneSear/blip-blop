# Third-party code

Vendored unmodified (single headers, and the files of Dear ImGui the editor
builds). Allowed by migration.md DEC-11.
None of them is linked into the game: doctest is used by the tests, stb
and nlohmann/json by the asset tools in `tools/` only, Dear ImGui by the
level editor (`tools/editor`) only.

| Directory | Library | Version | Source | Licence |
|-----------|---------|---------|--------|---------|
| `doctest/` | doctest | 2.4.11 | https://github.com/doctest/doctest | MIT |
| `stb/` | stb_image 2.30, stb_image_write 1.16 | commit `2c980bb59875b0d32144a71867fbdebb2f77cd20` | https://github.com/nothings/stb | MIT or public domain (`stb/LICENSE`) |
| `imgui/` | Dear ImGui: the core files and the SDL 2 platform and SDL_Renderer backends (`backends/imgui_impl_sdl2.*`, `imgui_impl_sdlrenderer2.*`) | 1.92.9b (`https://github.com/ocornut/imgui/archive/refs/tags/v1.92.9b.tar.gz`, SHA-256 `21d8a0a565e85dce943e375db00812c2f3f0ab21f3f0f7964e364a63422d7f99`) | https://github.com/ocornut/imgui | MIT (`imgui/LICENSE.txt`) |
| `nlohmann/` | nlohmann/json | 3.11.3 (`single_include/nlohmann/json.hpp`, SHA-256 `9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6`) | https://github.com/nlohmann/json | MIT |
