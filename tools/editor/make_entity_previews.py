#!/usr/bin/env python3
"""Finds the game image each entity of the 2002 level editor showed.

The 2002 editor drew enemies and animated props with BMP icons listed in
the bbe_ennemis.ini and bbe_fondsanimes.ini files of each level-editor
folder ("<id> <name>*<file.bmp>"). The icons are copies of images of the
levels' sprite banks; this script finds, for every icon, the bank image it
copies (the same size, at most 5 % of the opaque pixels differing by more
than the 16-bit rounding, 12 levels per channel), so that the new editor (bbedit) draws each
entity with its real image and hotspot. Measured on the shipped data:
64 of the 89 enemy and prop types match, every one at 0 % (0.1 % for
lemmings.water); the others have no icon of a bank image's size or differ
by 9 % or more (other images), and bbedit draws them as labelled boxes.

usage: make_entity_previews.py <level-editor-dir> <source-dir> <entities.txt> <out.json>

<entities.txt> is the output of `bbassets entities`. Only the Python
standard library is used (PNG and BMP are decoded here).
"""

import glob
import json
import os
import re
import struct
import sys
import zlib


def read_png(path):
    """8-bit RGBA PNG (what bbassets writes) -> (width, height, pixels)."""
    data = open(path, 'rb').read()
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError(path + ': not a PNG file')
    pos = 8
    idat = b''
    width = height = 0
    while pos < len(data):
        (length,) = struct.unpack('>I', data[pos:pos + 4])
        kind = data[pos + 4:pos + 8]
        chunk = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b'IHDR':
            width, height, depth, colour = struct.unpack('>IIBB', chunk[:10])
            if depth != 8 or colour != 6:
                raise ValueError(path + ': not 8-bit RGBA')
        elif kind == b'IDAT':
            idat += chunk
    raw = zlib.decompress(idat)
    stride = width * 4
    out = bytearray()
    previous = bytearray(stride)
    pos = 0
    for _ in range(height):
        kind = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        for x in range(stride):
            a = line[x - 4] if x >= 4 else 0
            b = previous[x]
            c = previous[x - 4] if x >= 4 else 0
            if kind == 1:
                line[x] = (line[x] + a) & 255
            elif kind == 2:
                line[x] = (line[x] + b) & 255
            elif kind == 3:
                line[x] = (line[x] + (a + b) // 2) & 255
            elif kind == 4:
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        out += line
        previous = line
    return width, height, [tuple(out[i:i + 4]) for i in range(0, len(out), 4)]


def png_size(path):
    with open(path, 'rb') as f:
        head = f.read(24)
    return struct.unpack('>II', head[16:24])


def read_bmp(path):
    """8- or 24-bit uncompressed BMP -> (width, height, RGB pixels)."""
    data = open(path, 'rb').read()
    (offset,) = struct.unpack('<I', data[10:14])
    (header,) = struct.unpack('<I', data[14:18])
    width, height = struct.unpack('<ii', data[18:26])
    (bits,) = struct.unpack('<H', data[28:30])
    (compression,) = struct.unpack('<I', data[30:34])
    if compression != 0 or bits not in (8, 24):
        raise ValueError('%s: %d-bit BMP, compression %d' % (path, bits, compression))
    palette = []
    if bits == 8:
        (used,) = struct.unpack('<I', data[46:50])
        base = 14 + header
        for i in range(used or 256):
            b, g, r = data[base + i * 4:base + i * 4 + 3]
            palette.append((r, g, b))
    rows = abs(height)
    stride = (width * bits // 8 + 3) & ~3
    pixels = [None] * (width * rows)
    for row in range(rows):
        y = rows - 1 - row if height > 0 else row
        line = data[offset + row * stride:offset + row * stride + stride]
        for x in range(width):
            if bits == 24:
                b, g, r = line[x * 3:x * 3 + 3]
                pixels[y * width + x] = (r, g, b)
            else:
                pixels[y * width + x] = palette[line[x]]
    return width, rows, pixels


def difference(icon, image):
    """Share of the image's opaque pixels the icon does not copy."""
    opaque = 0
    differ = 0
    for p, q in zip(icon, image):
        if q[3] == 0:
            continue
        opaque += 1
        if max(abs(p[0] - q[0]), abs(p[1] - q[1]), abs(p[2] - q[2])) > 12:
            differ += 1
    return differ / opaque if opaque else 1.0


def main(editor_dir, source_dir, entities_file, out_file):
    names = {}
    for line in open(entities_file, encoding='utf-8'):
        category, legacy_id, name = line.split()
        names[(category, int(legacy_id))] = name

    # Icons by (category, id), from every folder (a type can appear in several).
    icons = {}
    for folder in sorted(glob.glob(os.path.join(editor_dir, '*', ''))):
        for ini, category in (('bbe_ennemis.ini', 'enemy'), ('bbe_fondsanimes.ini', 'prop')):
            path = os.path.join(folder, ini)
            if not os.path.exists(path):
                continue
            for line in open(path, encoding='latin-1').read().splitlines()[1:]:
                m = re.match(r'\s*(\d+)\s+(.*?)\*(\S+)', line)
                if m:
                    icons.setdefault((category, int(m.group(1))), []).append(os.path.join(folder, m.group(3)))

    # The banks levels draw enemies and props from.
    banks = {'enemy': set(), 'prop': set()}
    for level in glob.glob(os.path.join(source_dir, 'levels', '*.json')):
        assets = json.load(open(level, encoding='utf-8'))['assets']
        for category, key in (('enemy', 'enemies'), ('prop', 'props')):
            if assets[key]:
                banks[category].add(assets[key])
    images = {}  # (width, height) -> [(bank, index, path)]
    for category in banks:
        for bank in sorted(banks[category]):
            stem = bank.rsplit('.', 1)[0]
            manifest = json.load(open(os.path.join(source_dir, 'sprites', stem, 'sprites.json'), encoding='utf-8'))
            for index, item in enumerate(manifest['images']):
                path = os.path.join(source_dir, 'sprites', stem, item['file'])
                images.setdefault(png_size(path), []).append((category, bank, index, path))

    decoded = {}
    previews = []
    unmatched = []
    for key in sorted(names, key=lambda k: (('enemy', 'prop', 'pickup', 'vehicle').index(k[0]), k[1])):
        category, legacy_id = key
        if category not in ('enemy', 'prop'):
            continue
        best = None
        for icon_path in icons.get(key, []):
            width, height, icon = read_bmp(icon_path)
            for image_category, bank, index, path in images.get((width, height), []):
                if image_category != category:
                    continue
                if path not in decoded:
                    decoded[path] = read_png(path)[2]
                score = difference(icon, decoded[path])
                if best is None or score < best[0]:
                    best = (score, bank, index, icon_path)
        if best is None or best[0] > 0.05:
            unmatched.append((names[key], best))
            continue
        previews.append({'entity': names[key], 'sprites': best[1], 'image': best[2]})

    out = {'format': 'blipblop-editor-entities', 'version': 1, 'previews': previews}
    with open(out_file, 'w', encoding='utf-8', newline='\n') as f:
        f.write('{\n  "format": "blipblop-editor-entities",\n  "version": 1,\n  "previews": [\n')
        f.write(',\n'.join('    ' + json.dumps(p) for p in previews))
        f.write('\n  ]\n}\n')
    print('%d previews written to %s' % (len(previews), out_file))
    for name, best in unmatched:
        if best is None:
            print('no icon of the same size as a bank image: %s' % name)
        else:
            print('no match for %s (best: %s image %d, %.0f %% different, %s)' %
                  (name, best[1], best[2], best[0] * 100, best[3]))
    return 0


if __name__ == '__main__':
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    sys.exit(main(*sys.argv[1:]))
