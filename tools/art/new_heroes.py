#!/usr/bin/env python3
"""The art of the third and fourth players (four-player mode): Bleach, a
pink Blip, and Blitz, a green Blop, recoloured from the 2002 banks.

  bleach.gfx  from blip.gfx: the blue body and hands pink, the red
              bandana purple;
  blitz.gfx   from blop.gfx: the red body and hands green, the green
              helmet khaki;
  scorep.lft, scoreg.lft  from scorei.lft (Blip's score digits) and
              scoreo.lft (Blop's, red to yellow): pink and greens.

The grey metal stays as it is. The muzzle flash images (74 to 103) also
hold the hero's hand: there only the colours of the hero's other images
change, and never a pure red (the flames' reds; Blip's bandana shares
them). The colour key (docs/formats/gfx.md) is never produced. The
images are decoded and encoded again as the 2002 editors wrote LGX
(docs/formats/lgx.md: v1 greedy run-length, v0 raw RGB565).
Deterministic: the same bytes give the same bytes.

Used by tools/data/fix_legacy_data.py. Run alone, it writes preview sheets:

  new_heroes.py <data dir> <output dir>
"""

import colorsys
import os
import struct
import sys

FLASHES = range(74, 104)  # smg_flash: flames, left as they are

KEY555 = (30 << 10) | (25 << 5) | 18
KEY565 = (30 << 11) | (52 << 5) | 18


def to8(c, bits):
    return (c << (8 - bits)) | (c >> (2 * bits - 8))


def hsv(r, g, b):
    return colorsys.rgb_to_hsv(r / 255, g / 255, b / 255)


def rgb(h, s, v):
    r, g, b = colorsys.hsv_to_rgb(h % 1.0, max(0.0, min(1.0, s)), max(0.0, min(1.0, v)))
    return round(r * 255), round(g * 255), round(b * 255)


def bleach(r, g, b):
    h, s, v = hsv(r, g, b)
    deg = h * 360
    if s > 0.25 and 190 <= deg <= 250:   # the blue body and hands: pink
        return rgb(325 / 360, s * 0.75, min(1.0, v * 1.08 + 0.06))
    if s > 0.45 and (deg >= 345 or deg <= 12):  # the red bandana: purple
        return rgb(280 / 360, s * 0.85, v)
    return r, g, b


def blitz(r, g, b):
    h, s, v = hsv(r, g, b)
    deg = h * 360
    if s > 0.25 and (deg >= 340 or deg <= 22):  # the red body and hands: green
        return rgb(115 / 360, s * 0.9, v * 0.92)
    if s > 0.25 and 80 <= deg <= 160:   # the green helmet: khaki
        return rgb(40 / 360, s * 0.7, v * 0.8)
    return r, g, b


def blitz_digits(r, g, b):
    """Blop's score digits run from red to yellow: greens, in the same
    order."""
    h, s, v = hsv(r, g, b)
    deg = h * 360
    if s > 0.25 and (deg >= 330 or deg <= 65):
        return rgb((100 + (deg if deg <= 65 else deg - 360) * 0.5) / 360, s, v)
    return r, g, b


# --- LGX ---------------------------------------------------------------

def decode_lgx(blob):
    """(version, width, height, 16-bit words of the pixels)."""
    assert blob[:3] == b"LGX", "not an LGX image"
    version = blob[3]
    w, h, depth = struct.unpack_from("<HHH", blob, 4)
    assert depth == 16
    words = struct.unpack_from("<%dH" % ((len(blob) - 10) // 2), blob, 10)
    if version == 0:
        return version, w, h, list(words[:w * h])
    pixels = []
    i = 0
    while len(pixels) < w * h:
        word = words[i]
        if word & 0x8000:
            pixels.extend([words[i + 1]] * (word & 0x7FFF))
            i += 2
        else:
            pixels.append(word)
            i += 1
    return version, w, h, pixels


def encode_lgx(version, w, h, pixels):
    out = bytearray(b"LGX")
    out += struct.pack("<BHHH", version, w, h, 16)
    if version == 0:
        out += struct.pack("<%dH" % len(pixels), *pixels)
        return bytes(out)
    i = 0
    n = len(pixels)
    while i < n:
        j = i + 1
        while j < n and pixels[j] == pixels[i] and j - i < 0x7FFF:
            j += 1
        if j - i >= 2:
            out += struct.pack("<HH", 0x8000 | (j - i), pixels[i])
        else:
            out += struct.pack("<H", pixels[i])
        i = j
    return bytes(out)


def recolour_word(word, version, colour):
    if version == 0:  # RGB565
        if word == KEY565:
            return word
        r, g, b = to8(word >> 11, 5), to8((word >> 5) & 63, 6), to8(word & 31, 5)
        nr, ng, nb = colour(r, g, b)
        out = ((nr >> 3) << 11) | ((ng >> 2) << 5) | (nb >> 3)
        return out ^ 1 if out == KEY565 else out
    if word == KEY555:
        return word
    r, g, b = to8((word >> 10) & 31, 5), to8((word >> 5) & 31, 5), to8(word & 31, 5)
    nr, ng, nb = colour(r, g, b)
    out = ((nr >> 3) << 10) | ((ng >> 3) << 5) | (nb >> 3)
    return out ^ 1 if out == KEY555 else out


def recolour_image(blob, colour, only=None):
    """The image with its colours changed; with `only`, just the 16-bit
    colours in that set."""
    version, w, h, pixels = decode_lgx(blob)
    cache = {}
    out = []
    for p in pixels:
        if p not in cache:
            cache[p] = p if only is not None and p not in only else recolour_word(p, version, colour)
        out.append(cache[p])
    return encode_lgx(version, w, h, out)


def is_pure_red(word):
    return (word >> 5) & 31 == 0 and word & 31 == 0


def recolour_bank(gfx, colour, flashes=()):
    """A .gfx bank recoloured; in the images of `flashes`, only the colours
    of the other images that are not pure reds."""
    count = struct.unpack_from("<i", gfx, 0)[0]
    images = []
    at = 4
    for _ in range(count):
        hx, hy, size = struct.unpack_from("<iii", gfx, at)
        images.append((hx, hy, gfx[at + 12:at + 12 + size]))
        at += 12 + size
    assert at == len(gfx), "trailing bytes in the bank"

    hero = set()
    for i, (_, _, blob) in enumerate(images):
        if i not in flashes:
            hero.update(decode_lgx(blob)[3])
    hand = {p for p in hero if not is_pure_red(p)}

    out = bytearray(gfx[:4])
    for i, (hx, hy, blob) in enumerate(images):
        blob = recolour_image(blob, colour, hand if i in flashes else None)
        out += struct.pack("<iii", hx, hy, len(blob)) + blob
    return bytes(out)


def recolour_font(lft, colour):
    """A .lft font with every glyph recoloured."""
    out = bytearray(lft[:8])
    at = 8
    for _ in range(1, 256):
        size = struct.unpack_from("<i", lft, at)[0]
        blob = lft[at + 4:at + 4 + size]
        if size > 0:
            blob = recolour_image(blob, colour)
        out += struct.pack("<i", len(blob)) + blob
        at += 4 + size
    assert at == len(lft), "trailing bytes in the font"
    return bytes(out)


# The new files, from the 2002 ones.
def new_files(read):
    """{name: bytes} of the new files; read(name) gives a 2002 file."""
    return {
        "data/bleach.gfx": recolour_bank(read("data/blip.gfx"), bleach, FLASHES),
        "data/blitz.gfx": recolour_bank(read("data/blop.gfx"), blitz, FLASHES),
        "data/scorep.lft": recolour_font(read("data/scorei.lft"), bleach),
        "data/scoreg.lft": recolour_font(read("data/scoreo.lft"), blitz_digits),
    }


# --- preview -------------------------------------------------------------

def bank_images(gfx):
    count = struct.unpack_from("<i", gfx, 0)[0]
    at = 4
    for _ in range(count):
        size = struct.unpack_from("<i", gfx, at + 8)[0]
        yield decode_lgx(gfx[at + 12:at + 12 + size])
        at += 12 + size


def sheet(banks, path, scale=2, width=1200):
    """A BMP of every image of the banks, one bank per block."""
    tiles = []
    for gfx in banks:
        x = y = rowh = 0
        for version, w, h, pixels in bank_images(gfx):
            if x + w * scale > width:
                x, y, rowh = 0, y + rowh + 4, 0
            tiles.append((x, y, version, w, h, pixels))
            x += w * scale + 4
            rowh = max(rowh, h * scale)
        tiles.append(None)
        tiles.append(y + rowh + 12)
    height = 0
    blocks = []
    current = []
    for t in tiles:
        if t is None:
            continue
        if isinstance(t, int):
            blocks.append((height, current))
            height += t
            current = []
        else:
            current.append(t)
    canvas = [[(40, 40, 40)] * width for _ in range(height)]
    for top, items in blocks:
        for x, y, version, w, h, pixels in items:
            for j in range(h):
                for i in range(w):
                    p = pixels[j * w + i]
                    if version == 0:
                        c = (to8(p >> 11, 5), to8((p >> 5) & 63, 6), to8(p & 31, 5))
                    else:
                        c = (to8((p >> 10) & 31, 5), to8((p >> 5) & 31, 5), to8(p & 31, 5))
                    for dy in range(scale):
                        for dx in range(scale):
                            canvas[top + y + j * scale + dy][x + i * scale + dx] = c
    row = width * 3
    out = bytearray(b"BM") + struct.pack("<IHHI", 54 + row * height, 0, 0, 54)
    out += struct.pack("<IiiHHIIiiII", 40, width, height, 1, 24, 0, row * height, 0, 0, 0, 0)
    for j in range(height - 1, -1, -1):
        for r, g, b in canvas[j]:
            out += bytes((b, g, r))
    with open(path, "wb") as f:
        f.write(out)


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    data, out = sys.argv[1], sys.argv[2]

    def read(name):
        with open(os.path.join(data, os.path.basename(name)), "rb") as f:
            return f.read()

    files = new_files(read)
    sheet([read("data/blip.gfx"), files["data/bleach.gfx"]], os.path.join(out, "bleach.bmp"))
    sheet([read("data/blop.gfx"), files["data/blitz.gfx"]], os.path.join(out, "blitz.bmp"))
    print("wrote", os.path.join(out, "bleach.bmp"), "and", os.path.join(out, "blitz.bmp"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
