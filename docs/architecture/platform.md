# Adding a platform backend

The game runs on Windows, macOS and Linux through one backend, SDL 2
(`src/platform/sdl2`). Another backend (SDL 3, a console SDK) would
implement the same interfaces; nothing outside `src/platform` and
`src/app/main.cpp` would change.

## What a backend provides

All in `src/platform/include/bb/platform/`:

| Interface | Responsibility |
|-----------|----------------|
| `Platform` | Creates the `Display` and gives the other services. |
| `Display` | Shows a finished 640x480 frame (`present(const core::gfx::Pixmap&)`), scaled to the window or the screen; fullscreen; the refresh rate. A headless display (tests) presents nothing. |
| `InputSource` | The keyboard, pad and window events (`poll()`), with key codes of `bb::core::input` (`codes.hpp`); the pads' slots and names. |
| `Audio` | Sounds from complete WAV files (32 voices, looping or not) and one streamed music at a time (Ogg Vorbis or MP3); `available()` false when there is no device. |
| `Clock` | Microseconds since start, and sleeping. |
| `System` | Message boxes for errors and warnings; the game's directory and the user's preferences directory (settings, hi-scores, log). |

And the process entry (`entry.hpp`): the backend owns `main()` (or what the
system calls instead) and calls `bbMain(argc, argv)`.

## Steps

1. Create `src/platform/<name>/` with an implementation of the interfaces
   and a `create<Name>Platform()` factory, as `sdl2_platform.cpp` does.
2. Add a library target in `src/platform/CMakeLists.txt` linking
   `bb::platform` (the interfaces) and the system's libraries; the only
   files that may include them are in that directory.
3. Give the backend its entry point (`sdl2_main.cpp` is the model) and let
   `src/app/CMakeLists.txt` link it instead of SDL 2's.
4. In `src/app/main.cpp`, create the new platform instead of
   `bb::platform::sdl2::createPlatform()`.
5. Check it: the conformance tests (`tests/conformance`) compare a
   backend's display and audio with the SDL behaviour the replays were
   recorded with; the replays must keep their fingerprints (they do not
   depend on the backend: frames are fingerprinted before they are
   presented).

## Notes

* The pixel layout is the one of `core::gfx::Pixel` (bytes R, G, B, A).
* Input codes: keys below 255, `0x40000000 | (key & 0xFFF)` for the other
  keys, `((pad + 1) << 10) | button` for pads (100 to 103 for the
  directions); `bb::app::DeviceInput` turns the events into these. A pad
  the backend recognises as a game controller (`padIsController()`)
  reports the button numbers of `codes.hpp` (`kPadButtonA`...), its D-pad
  as a hat and its left stick as axes 0 and 1, whatever the device; the
  SDL 2 backend uses SDL's game controller database for this. The default
  pad layout of each player (`Bindings::defaultPadCode()`) relies on it.
* A deterministic run never reads the clock or the input of the backend
  directly: the session gives virtual time and the recorded input.
