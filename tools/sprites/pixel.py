"""Dependency-free pixel helpers for the sprite generators (Python 3 stdlib only).

Images are lists of rows of (r, g, b, a) tuples: image[y][x].
"""
import struct
import zlib

# Isometric terrain tile: a 2:1 diamond (see src/scene/world/core/isometric.hpp).
TILE_WIDTH, TILE_HEIGHT = 64, 32
# How far (in pixels) a tile sprite reaches past its diamond, so neighbours overlap and no
# seam shows between tiles.
TILE_BLEED = 1.5

TRANSPARENT = (0, 0, 0, 0)


def blank(width, height, color=TRANSPARENT):
    return [[color] * width for _ in range(height)]


def write_png(path, image):
    height, width = len(image), len(image[0])
    raw = b"".join(b"\0" + bytes(c for pixel in row for c in pixel) for row in image)

    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data
                + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF))

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)  # 8-bit RGBA
    with open(path, "wb") as file:
        file.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header)
                   + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b""))


def read_png(path):
    """Reads an 8-bit RGBA PNG written by write_png() (filter 0 only)."""
    with open(path, "rb") as file:
        data = file.read()
    pos, idat = 8, b""
    while pos < len(data):
        (length,) = struct.unpack(">I", data[pos:pos + 4])
        kind, body = data[pos + 4:pos + 8], data[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            width, height = struct.unpack(">II", body[:8])
        elif kind == b"IDAT":
            idat += body
        pos += 12 + length
    raw, stride = zlib.decompress(idat), width * 4 + 1
    return [[tuple(raw[y * stride + 1 + x * 4:y * stride + 5 + x * 4]) for x in range(width)]
            for y in range(height)]


def in_diamond(x, y, width=TILE_WIDTH, height=TILE_HEIGHT, bleed=TILE_BLEED):
    """Whether pixel (x, y) is inside the tile diamond, grown by `bleed` pixels."""
    half_w, half_h = width / 2, height / 2
    return abs(x + 0.5 - half_w) / half_w + abs(y + 0.5 - half_h) / half_h <= 1.0 + bleed / half_w


def on_diamond_rim(x, y):
    """Inside the plain diamond but next to its edge: keep details off it."""
    return in_diamond(x, y, bleed=0) and not all(
        in_diamond(x + dx, y + dy, bleed=0) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))


def diamond_tile(color_at):
    """A tile image: color_at(x, y) -> (r, g, b) inside the diamond, transparent outside."""
    return [[(*color_at(x, y), 255) if in_diamond(x, y) else TRANSPARENT
             for x in range(TILE_WIDTH)] for y in range(TILE_HEIGHT)]


def mix(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def shade(color, factor):
    return tuple(max(0, min(255, int(v * factor))) for v in color[:3])


def sheet(frames, columns):
    """Lays equal frames out in a grid, left to right then top to bottom (SpriteSheet order)."""
    frame_h, frame_w = len(frames[0]), len(frames[0][0])
    rows = (len(frames) + columns - 1) // columns
    image = blank(frame_w * columns, frame_h * rows)
    for index, frame in enumerate(frames):
        ox, oy = (index % columns) * frame_w, (index // columns) * frame_h
        for y in range(frame_h):
            image[oy + y][ox:ox + frame_w] = frame[y]
    return image
