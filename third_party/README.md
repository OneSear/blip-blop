# Third-party code

Vendored unmodified (single headers, and the files of Dear ImGui the editor
builds). Allowed by migration.md DEC-11.
The game links only the two music decoders of its mixer (`stb_vorbis.c`
and `dr_mp3.h`, in the `bb_core_codecs` library, Phase 9): doctest is used
by the tests, stb_image, stb_image_write and nlohmann/json by the asset
tools in `tools/` only, Dear ImGui by the level editor (`tools/editor`)
only.

| Directory | Library | Version | Source | Licence |
|-----------|---------|---------|--------|---------|
| `doctest/` | doctest | 2.4.11 | https://github.com/doctest/doctest | MIT |
| `stb/` | stb_image 2.30, stb_image_write 1.16, stb_vorbis 1.22 | commit `2c980bb59875b0d32144a71867fbdebb2f77cd20` (`stb_vorbis.c` SHA-256 `4c7cb2ff1f7011e9d67950446b7eb9ca044f2e464d76bfbb0b84dd2e23e65636`) | https://github.com/nothings/stb | MIT or public domain (`stb/LICENSE`) |
| `dr_libs/` | dr_mp3 0.7.4 (`dr_mp3.h` only) | commit `51e61d308dde6b437fce0c5fabb32cd86b40f4d7` (31 August 2026), SHA-256 `997b7ee18de6e6b81e2a83f1ea9fc62aef25c62b28d48db95635f49e65de0a2f` | https://github.com/mackron/dr_libs | public domain or MIT No Attribution (end of `dr_mp3.h`) |
| `imgui/` | Dear ImGui: the core files and the SDL 2 platform and SDL_Renderer backends (`backends/imgui_impl_sdl2.*`, `imgui_impl_sdlrenderer2.*`) | 1.92.9b (`https://github.com/ocornut/imgui/archive/refs/tags/v1.92.9b.tar.gz`, SHA-256 `21d8a0a565e85dce943e375db00812c2f3f0ab21f3f0f7964e364a63422d7f99`) | https://github.com/ocornut/imgui | MIT (`imgui/LICENSE.txt`) |
| `nlohmann/` | nlohmann/json | 3.11.3 (`single_include/nlohmann/json.hpp`, SHA-256 `9bea4c8066ef4a1c206b2be5a36302f8926f7fdc6087af5d20b417d0cf103ea6`) | https://github.com/nlohmann/json | MIT |
