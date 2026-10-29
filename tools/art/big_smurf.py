#!/usr/bin/env python3
"""Generates the Big Smurf's images (Papa Smurf: red hat and trousers, a
white beard) from the basic Smurf's images of snufenn, which has no art for him (the 2002 class borrowed the basic
Smurf's images). Deterministic: the same input gives the same PNG bytes.

Usage: big_smurf.py <snufenn source dir> <output dir> [preview.png]
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pngrgba  # noqa: E402

# The hat and trousers: greys of the basic Smurf, and their red.
REDS = {
    (246, 246, 246): (230, 41, 32),
    (238, 238, 238): (213, 32, 24),
    (222, 222, 222): (189, 24, 16),
    (172, 172, 172): (139, 16, 8),
}
BEARD = (246, 246, 246)
BEARD_SHADE = (205, 205, 205)

# Source images: (first, count, facing, beard) - facing 1 = right.
SOURCES = [
    (0, 5, 1, True),     # walk right
    (5, 5, 0, True),     # walk left
    (10, 1, 1, True),    # jump right (arms up: also his throw)
    (11, 1, 0, True),    # jump left
    (12, 11, 1, False),  # die right (head moves: no beard)
    (23, 11, 0, False),  # die left
]


def is_blue(px):
    r, g, b, a = px
    return a and b > r + 40 and b > g


def face_box(rows):
    """The face: the first 9 rows that hold blue, between the leftmost and
    rightmost blue pixels of those rows; None when there is no blue."""
    height, width = len(rows), len(rows[0])
    blue_rows = [y for y in range(height) if any(is_blue(px) for px in rows[y])]
    if not blue_rows:
        return None
    top = blue_rows[0]
    xs = [x for y in range(top, min(top + 9, height)) for x in range(width) if is_blue(rows[y][x])]
    return top, min(xs), max(xs)


def grey_regions(rows):
    """The 4-connected regions of hat greys, as lists of (x, y)."""
    height, width = len(rows), len(rows[0])
    seen, regions = set(), []
    for y in range(height):
        for x in range(width):
            if (x, y) in seen or not rows[y][x][3] or rows[y][x][:3] not in REDS:
                continue
            region, todo = [], [(x, y)]
            seen.add((x, y))
            while todo:
                cx, cy = todo.pop()
                region.append((cx, cy))
                for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                    if 0 <= nx < width and 0 <= ny < height and (nx, ny) not in seen:
                        px = rows[ny][nx]
                        if px[3] and px[:3] in REDS:
                            seen.add((nx, ny))
                            todo.append((nx, ny))
            regions.append(region)
    return regions


def recolour(rows, facing, beard):
    height = len(rows)
    out = [list(row) for row in rows]
    # Hat and trousers are large grey regions; eyes and teeth are small.
    for region in grey_regions(rows):
        if len(region) > 20:
            for x, y in region:
                out[y][x] = REDS[rows[y][x][:3]] + (rows[y][x][3],)
    box = face_box(rows) if beard else None
    if box is None:
        return out
    # The beard: the blue of the chin and the upper chest (rows 5 to 11
    # from the top of the face), on the side the Smurf faces.
    top, left, right = box
    middle = (left + right) // 2
    for y in range(top + 5, min(top + 12, height)):
        for x in range(left, right + 1):
            front = x >= middle - 2 if facing == 1 else x <= middle + 2
            if front and is_blue(rows[y][x]):
                out[y][x] = (BEARD if (x + 2 * y) % 4 else BEARD_SHADE) + (255,)
    return out


def main():
    source, output = sys.argv[1], sys.argv[2]
    os.makedirs(output, exist_ok=True)
    frames = []
    for first, count, facing, beard in SOURCES:
        for i in range(first, first + count):
            w, h, rows = pngrgba.read(os.path.join(source, '%03d.png' % i))
            new = recolour(rows, facing, beard)
            name = 'big_%03d.png' % i
            pngrgba.write(os.path.join(output, name), w, h, new)
            frames.append((i, w, h, rows, new))
    if len(sys.argv) > 3:
        # Preview: original above, Big Smurf below, 3x, on grey.
        scale, gap = 3, 4
        width = sum(w for _, w, _, _, _ in frames) * scale + gap * len(frames)
        height = max(h for _, _, h, _, _ in frames) * 2 * scale + gap * 3
        sheet = [[(128, 128, 140, 255)] * width for _ in range(height)]
        x0 = 0
        for _, w, h, old, new in frames:
            for band, img in enumerate((old, new)):
                y0 = gap + band * (h * scale + gap)
                for y in range(h * scale):
                    for x in range(w * scale):
                        px = img[y // scale][x // scale]
                        if px[3]:
                            sheet[y0 + y][x0 + x] = px
            x0 += w * scale + gap
        pngrgba.write(sys.argv[3], width, height, sheet)


if __name__ == '__main__':
    main()
