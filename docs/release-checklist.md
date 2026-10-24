# Release checklist

What the automated checks cannot see, tried by hand on each platform
(Windows, macOS, Linux) with the package that will be released, before the
release is tagged. The automated part comes first:

* CI is green on the three platforms (build, unit tests, the replays,
  the sanitizer job), and the packages are its artifacts.
* Locally, optionally: `tools/docker/linux-check.sh` and
  `tools/docker/windows-cross-check.sh` (MinGW build, every test and replay
  under Wine).

Write down the platform, the system version, the package and the result of
each line; a line that fails blocks the release or becomes a known issue in
the release notes.

## Install and start

- [ ] The package installs (Windows installer or zip; macOS: drag the app
      from the disk image; Linux: unpack the archive, SDL 2 and SDL2_mixer
      installed from the distribution) and the game starts from the
      system's usual place (Start menu, Applications, the folder).
- [ ] The log (below) says `Game data: ...blipblop.bbpack`, next to the
      executable (macOS: `Contents/Resources`).
- [ ] The intro plays, then the attract mode and the main menu.

## Display

- [ ] The window opens at 640x480 points; on a high-DPI screen the log says
      it is drawn at the screen's scale (for example 1280x960 pixels at 2x)
      and the pixels look sharp.
- [ ] Resizing the window keeps the 4:3 picture, with black bars.
- [ ] F10 switches to the full screen and back; so does the system's own
      control (macOS green button). After a restart the game opens as it
      was left.
- [ ] The VSYNC menu option switches; either way the game runs at its
      normal speed (the log's exit line: about 60 frames presented per
      second) and the character choice does not scroll faster on a 120 Hz
      or 144 Hz screen.

## Audio

- [ ] Menu music, level music and sounds play; the music changes between
      levels.
- [ ] `/quiet` starts without music.
- [ ] With no audio device (disabled or unplugged), the game warns once
      ("Cannot open the audio device") and runs silent.

## Controls

- [ ] The default keys work (arrows, Left Ctrl fire, Left Alt jump, Space
      cow bomb, Escape pause).
- [ ] A game controller plugged in before the start, and one plugged in
      while the game runs, play with the default layout (README: D-pad or
      left stick, A jump, X fire, B cow bomb; A confirms in the menus,
      Start pauses) without any setting; the log names them "(game
      controller)".
- [ ] A second controller plays the second player.
- [ ] A controller can be assigned in the controls menu, and then plays
      with the chosen buttons only; unplugging one during a level does not
      crash.
- [ ] Redefined keys survive a restart.

## Focus and quitting

- [ ] Switching to another application during a level opens the pause
      menu; coming back resumes from it.
- [ ] Closing the window (or the system's quit) saves the settings and the
      hi-scores, like EXIT in the main menu.

## User files

In the user's preferences directory: `%APPDATA%\LOADED Studio\Blip and Blop\`
(Windows), `~/Library/Application Support/LOADED Studio/Blip and Blop/`
(macOS), `~/.local/share/LOADED Studio/Blip and Blop/` (Linux).

- [ ] After a first run: `settings.cfg`, `hiscores.scr` and `BlipBlop.log`
      are there, and nothing was written next to the executable or in the
      installation directory.
- [ ] A score entered at the end of a game is in the hi-score table after
      a restart.
- [ ] Deleting the directory gives the default settings and hi-scores on
      the next start.
