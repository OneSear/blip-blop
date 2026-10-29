# `bb.cfg` (binary, 61 bytes, `config.cpp`)

`vsync` (bool, 1 byte), `language` (int32), then 14 × int32 bindings: P1 up/down/left/right/fire/jump/special, P2 same. Values are raw SDL2 key codes (`0x4000xxxx` for non-printable keys) or joystick codes `((slot+1) << 10) | button`, with 100–103 for hat directions. Not portable; import once.
