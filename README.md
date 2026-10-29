# Blip & Blop

*Blip'n Blop* ("Balls of Steel") is a 2002 run-and-gun game by LOADED Studio:
two ball-shaped heroes fight their way through parodies of the Smurfs, the
Snorks, Pokémon, the Care Bears, Lemmings and a gallery of video-game icons.
The game was released as open source in 2013 and ported to SDL by volunteers.

This repository is migrating that code to modern C++17, SDL2 and CMake for
Windows, macOS and Linux, with a portable core (no platform code outside a thin
SDL2 layer). The plan, the reverse-engineered file formats and the
list of known defects are in [migration.md](migration.md); progress is tracked
in [docs/progress.md](docs/progress.md).

## Building

Requirements: CMake 3.21 or later (presets), a C++17 compiler and SDL2. The
game mixes its sound itself (Ogg Vorbis and MP3 music through the vendored
stb_vorbis and dr_mp3), so SDL2_mixer is not needed.

| Platform | Dependencies |
|----------|--------------|
| macOS (Homebrew) | `brew install cmake sdl2` |
| Debian / Ubuntu | `sudo apt install cmake g++ libsdl2-dev` |
| Windows (vcpkg) | `vcpkg install sdl2`, then pass `-DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake` |

```sh
cmake --preset default          # Debug build in build/default
cmake --build --preset default
ctest --preset default          # unit tests and regression replays
```

Other presets: `release` (optimised) and `asan` (AddressSanitizer and
UndefinedBehaviorSanitizer). In-source builds are refused.

The build extracts the game data (`vc-projects/Blip_n_Blop_3/data.zip`) into
the build directory, next to the executable, and builds the runtime pack
`blipblop.bbpack` from the source assets in `assets/source/` (PNG, JSON, WAV,
Ogg Vorbis and UTF-8 text; see [docs/formats/source-assets.md](docs/formats/source-assets.md)).
When `assets/source/` is absent, the build extracts it from the game data
first.

### Packages

```sh
cmake --preset release && cmake --build --preset release
cd build/release && cpack
```

CPack makes a disk image with `Blip and Blop.app` on macOS (the SDL
libraries bundled), an NSIS installer and a zip on Windows, and a `.tar.gz`
on Linux (SDL from the system). Each carries the executable, the runtime
pack, `LICENSE` and this README (see `cmake/BBPackaging.cmake`).

Checks in containers (Docker): `tools/docker/linux-check.sh` (GCC, every
test) and `tools/docker/windows-cross-check.sh` (a MinGW build for Windows,
every test and replay under Wine). Before a release, the manual checks of
[docs/release-checklist.md](docs/release-checklist.md).

Releases: pushing a tag `v*` (for example `git tag v1.0.0 && git push
origin v1.0.0`), or creating a release with a new tag in GitHub, runs
`.github/workflows/release.yml`. It builds the packages on Windows (64 and
32 bits: `win64` and `win32` files), macOS and Linux with the tag's version, tests each (unit tests, then the
campaign and smoke replays played from the package) and attaches them to
the tag's release. A release it has to create starts as a draft with
generated notes and is published once the three platforms are attached.
For an existing tag: Actions > Release > Run workflow. The macOS disk
image holds a universal app (Apple silicon and Intel, macOS 11 or later):
the workflows build SDL 2 so with `tools/macos/sdl2-universal.sh` instead
of using Homebrew's.

### Assets

The asset tool `bbassets` (built in `build/<preset>/tools/`) converts between
the 2002 files, the source assets and the runtime pack:

```sh
bbassets extract data ../../assets/source     # 2002 files -> source assets
bbassets pack ../../assets/source game.bbpack  # source assets -> runtime pack
bbassets verify ../../assets/source data       # do the sources rebuild the 2002 files?
bbassets legacy ../../assets/source out        # source assets -> 2002 files (for the editors)
bbassets ids ../../assets/source ../../src/assets/include   # sprite-name constants
```

The level editor `bbedit` (same directory) edits the levels and the sprite
hotspots and names of `assets/source`, and test-plays a level with F5:
see [docs/editor.md](docs/editor.md).

## Running

```sh
cd build/default
./blipblop            # add /quiet to disable music, /cheat for cheat keys
./blipblop --pack blipblop.bbpack   # read the assets from the runtime pack
```

Without `--data` or `--pack`, the game reads the `data` directory of the
working directory; when there is none, the runtime pack next to the
executable (in the app bundle, `Contents/Resources`), then a `data`
directory next to the executable.

Keyboard defaults: arrows to move and aim, Left Ctrl to fire, Left Alt to
jump, Space for the cow bomb, Escape for the pause menu. Cheat keys (`/cheat`):
F1 ammo, F2 cow bomb, F3 extra life, F9 win the level, F12 FPS counter.
F10 switches between the window and the full screen.

Game controllers: the first pad plays the first player, the second pad the
second, with the D-pad or the left stick to move, A to jump, X to fire and
B for the cow bomb; A also confirms in the menus, Start or Back pauses and
goes back, Start ends the name entry. A player who binds any action to a
pad in the controls menu gets only the bindings chosen there. The layout is
by button position (A is the bottom button, cross on a PlayStation pad) for
the pads SDL recognises; other pads keep their own button numbers.

The options menu (in the main menu and the pause menu) sets the vsync, the
scaling (FIT fills the window; INTEGER scales by whole factors, with a black
border), the filter (SHARP pixels or SMOOTH), SMOOTH MOTION (draws between
the game's 84.7 simulation ticks a second, so that movement is even on a
60 Hz screen, one tick late) and the language (English or French), and
opens the players' controls; F10 switches the full screen.

START offers games of 1 to 4 players. Player 1 chooses Blip or Blop and
player 2 is the other; players 3 and 4 are Bleach (a pink Blip: SMG and
laser) and Blitz (a green Blop: shotgun and flamethrower). Players 3 and 4
have no keys by default and play with game controllers 3 and 4 (the same
layout as the others); the controls menu can bind them to anything. Their
scores show in a second row under players 1 and 2; the level summary
becomes a table; only Blip and Blop speak in the dialogues.

The game keeps its settings (`settings.cfg`) and its log (`BlipBlop.log`) in
the user's preferences directory, for example
`~/Library/Application Support/LOADED Studio/Blip and Blop/` on macOS or
`~/.local/share/LOADED Studio/Blip and Blop/` on Linux; `--settings <file>` and
`--log <file>` choose other files. The hi-scores are kept there too
(`hiscores.scr`). The first run imports the 2002 files of the data directory
when there are any: `data/bb.cfg` and `data/bb.scr` (the one in `data.zip`
holds two scores played in 2014, "KID" and " DDD", above the default table).

### Deterministic runs, replays and regression tests

```sh
./blipblop --record session.bbreplay        # play and record the input
./blipblop --replay session.bbreplay --hash out.bbhash
./blipblop --headless --replay ../../tests/replays/campaign.bbreplay
./blipblop --help
```

Recording and replaying use virtual time and a fixed random seed, so a replay
reproduces the same frames on every run and every platform. `--hash` writes
fingerprints of the presented frames and of the world state; the regression
tests compare them with the expected values in `tests/replays/`. See
[docs/replays.md](docs/replays.md).

## Repository layout

```
CMakeLists.txt, CMakePresets.json, cmake/   the build
src/core/                 new platform-independent code (C++17 standard library only)
src/platform/             platform interfaces, and their SDL 2 backend (the only SDL user)
src/game/                 the game: world, entities of every world, screens, campaign
src/app/                  the game executable: entry point, session (record/replay), input, settings
tests/                    unit, conformance and regression tests; benchmarks in tests/bench
tools/                    developer tools: bbassets, the level editor bbedit, replay scripts, ...
vc-projects/Blip_n_Blop_3 data.zip, the 2002 game data (the 2002 code is in the git history)
vc-projects/*Editor*      the original MFC level, sprite, font and sound editors (Windows; replaced by bbedit)
level-editor/, assets-gfx/ source art for the editors
docs/                     file formats, replay format, progress notes
```

## Licence and credits

The code is distributed under the MIT licence (see [LICENSE](LICENSE)): the
Google Code project through which LOADED Studio published the game in 2013
recorded the licence as MIT.

Original game (2002): Benjamin Karaban and Sylvain Bugat (code); Gerard Kelly
(music and sound design); Rik Ede and El Mobo (additional music); Laurent
Schneider, Jérémie Comarmond, Didier Colin, Jérôme Karaban and Sylvain Bugat
(artwork); Eric Khodja and Benjamin Karaban (storyline). SDL port (2013-2014):
Roberto Domenella, kidanger and Benjamin Karaban. Modernisation work in the
upstream repository (2019): Guillaume "Vermeille" Sanchez —
<https://github.com/benkaraban/blip-blop>.

The game parodies characters that belong to their respective owners (the
Smurfs, the Snorks, Pokémon, the Care Bears, Lemmings, Mario, Sonic, Pac-Man,
Lara Croft, Rayman and others). Those characters are not covered by the
licence above, and builds of this game must not be distributed commercially.
