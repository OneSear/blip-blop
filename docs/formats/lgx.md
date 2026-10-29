# LGX image (`LGXpacker::loadLGX`, encoders in `winLGX.cpp` of the GFX and FNT editors)

Header, 10 bytes: `'L' 'G' 'X'`, `version` (uint8: 0 or 1), `width` (uint16), `height` (uint16), `depth` (uint16, always 16).

- **v0**: `width × height` uint16 pixels in RGB565 (5-6-5), row-major, no padding.
- **v1**: RLE stream of uint16 words over the pixel sequence (runs may cross row boundaries): a word with bit 15 set is a run marker — `count = word & 0x7FFF` (2..32767) followed by one colour word; a word with bit 15 clear is a literal colour. Colours are RGB555 (5-5-5, bit 15 unused).

The encoders sample pixels through GDI `GetPixel`, hence the "colours are BGR" remark in the comments. All shipped fonts are v1. Re-encoding the decoded pixels with the
editors' greedy rule (longest run of one colour, at most 32,767, across rows;
runs of one are literals) gives the shipped bytes for all 3,564 v1 images
(`encodeLgx`, checked by `tests/formats/legacy_formats_test.cpp`). Of the 3,755 images in the 56 banks, 3,564 are v1 and 191 are v0; v0 is used for the 640×480 decor screens, the briefing images and a few large props, where run-length coding would not pay off.
