#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render the Wraith Launcher ghost icon to PNG/ICO/ICNS without third-party dependencies.

The shapes mirror program_info/io.github.wraithlauncher.WraithLauncher.svg (256x256 design units).
Usage: python wraith/tools/gen_icons.py   (run from the repository root)
"""
import math
import os
import struct
import zlib

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
PI = os.path.join(ROOT, "program_info")

STROKE = 4.0  # half of the SVG stroke width
TOP, BOTTOM = (0xF2, 0xFD, 0xFF), (0xA7, 0xE9, 0xF7)
OUTLINE = (0x2B, 0x2F, 0x44)
FEATURE = (0x1E, 0x20, 0x30)
GLOW = (0x8B, 0x7C, 0xF6)
BUMPS = (72.0, 128.0, 184.0)


def in_ghost(x, y, grow=0.0):
    """Ghost silhouette, optionally grown outward by `grow` units (used for the outline)."""
    if y <= 112 and (x - 128) ** 2 + (y - 112) ** 2 <= (84 + grow) ** 2:
        return True
    if 44 - grow <= x <= 212 + grow and 112 <= y <= 200:
        return True
    if y >= 200:
        for cx in BUMPS:
            if (x - cx) ** 2 + (y - 200) ** 2 <= (28 + grow) ** 2:
                return True
    return False


def in_ellipse(x, y, cx, cy, rx, ry):
    return ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0


def sample(x, y):
    """Return (r, g, b, a) in 0..1 floats for one design-space point."""
    if in_ghost(x, y, -STROKE):
        if in_ellipse(x, y, 100, 118, 13, 18) or in_ellipse(x, y, 156, 118, 13, 18) or in_ellipse(x, y, 128, 160, 10, 7):
            return FEATURE + (1.0,)
        t = min(max((y - 28) / 200.0, 0.0), 1.0)
        return tuple(TOP[i] + (BOTTOM[i] - TOP[i]) * t for i in range(3)) + (1.0,)
    if in_ghost(x, y, STROKE):
        return OUTLINE + (1.0,)
    if in_ellipse(x, y, 128, 240, 64, 9):
        return GLOW + (0.35,)
    return (0, 0, 0, 0.0)


def render(size):
    ss = 4 if size <= 64 else 2
    scale = 256.0 / size
    rows = []
    for py in range(size):
        row = bytearray([0])  # PNG filter: none
        for px in range(size):
            r = g = b = a = 0.0
            for sy in range(ss):
                for sx in range(ss):
                    cr, cg, cb, ca = sample((px + (sx + 0.5) / ss) * scale, (py + (sy + 0.5) / ss) * scale)
                    r += cr * ca
                    g += cg * ca
                    b += cb * ca
                    a += ca
            n = ss * ss
            if a > 0:
                row += bytes((round(r / a), round(g / a), round(b / a), round(255 * a / n)))
            else:
                row += b"\0\0\0\0"
        rows.append(bytes(row))
    return png(size, b"".join(rows))


def png(size, raw):
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")


def ico(images):
    """images: list of (size, png_bytes). PNG-compressed ICO entries are valid since Windows Vista."""
    header = struct.pack("<HHH", 0, 1, len(images))
    offset = 6 + 16 * len(images)
    entries, data = b"", b""
    for size, blob in images:
        dim = 0 if size >= 256 else size
        entries += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(blob), offset + len(data))
        data += blob
    return header + entries + data


def icns(images):
    types = {16: b"icp4", 32: b"icp5", 64: b"icp6", 128: b"ic07", 256: b"ic08", 512: b"ic09", 1024: b"ic10"}
    body = b"".join(types[s] + struct.pack(">I", len(blob) + 8) + blob for s, blob in images if s in types)
    return b"icns" + struct.pack(">I", len(body) + 8) + body


def main():
    sizes = [16, 24, 32, 48, 64, 128, 256, 512]
    rendered = {s: render(s) for s in sizes}
    with open(os.path.join(PI, "wraithlauncher.ico"), "wb") as f:
        f.write(ico([(s, rendered[s]) for s in (16, 24, 32, 48, 64, 128, 256)]))
    with open(os.path.join(PI, "io.github.wraithlauncher.WraithLauncher_256.png"), "wb") as f:
        f.write(rendered[256])
    with open(os.path.join(PI, "wraithlauncher.icns"), "wb") as f:
        f.write(icns([(s, rendered[s]) for s in (16, 32, 64, 128, 256, 512)]))
    print("wrote wraithlauncher.ico, io.github.wraithlauncher.WraithLauncher_256.png, wraithlauncher.icns")


if __name__ == "__main__":
    main()
