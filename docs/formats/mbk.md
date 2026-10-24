# `.mbk` — music bank (text, `music_bank.cpp:30`)

Line 1: `count`. Then `count` lines `type path`, `type` 0 = tracker module (unused by any shipped bank), 1 = streamed music. Paths are relative to the working directory and **must use `/`** (9 shipped banks still use `\`). `.zik` files are Ogg Vorbis (formerly renamed MP3).
