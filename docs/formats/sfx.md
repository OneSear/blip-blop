# `.sfx` — sound bank (`sound_bank.cpp:84`, written by `SFX EditorDlg.cpp:688`)

| Field | Type | Meaning |
|-------|------|---------|
| `count` | int32 | |
| repeated: `voices` | int32 | intended simultaneous instances (always 1 in shipped data; ignored by the SDL port) |
| `size` | int32 | |
| `wav` | bytes | a complete RIFF/WAVE file; shipped data is PCM mono: 142 × 22,050 Hz 16-bit, 18 × 22,050 Hz 8-bit, 12 × 44,100 Hz 16-bit, 3 odd ones (8,050 / 10,989 / 44,100 Hz 8-bit) |
