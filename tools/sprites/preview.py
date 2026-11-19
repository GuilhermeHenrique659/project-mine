"""Previews sprites, enlarged, so they can be checked before running the game.

  python tools/sprites/preview.py strip <out.png> <sprite.png>...
      every sprite side by side (a sheet shows all its frames)
  python tools/sprites/preview.py tiled <out.png> <tile.png>...
      a 6x6 isometric patch laid out like the game, tiles picked at random
      (first frame of a sheet): shows seams between tiles
"""
import random
import sys

from pixel import TILE_HEIGHT, TILE_WIDTH, blank, read_png, write_png

SCALE = 3
BACKGROUND = (30, 30, 30, 255)


def scale(image, factor):
    return [[p for p in row for _ in range(factor)] for row in image for _ in range(factor)]


def paste(dest, image, ox, oy):
    for y, row in enumerate(image):
        for x, pixel in enumerate(row):
            if pixel[3] and 0 <= oy + y < len(dest) and 0 <= ox + x < len(dest[0]):
                dest[oy + y][ox + x] = pixel


def strip(paths):
    images = [read_png(p) for p in paths]
    width = sum(len(i[0]) for i in images) + 4 * (len(images) - 1)
    out, x = blank(width, max(len(i) for i in images), BACKGROUND), 0
    for image in images:
        paste(out, image, x, 0)
        x += len(image[0]) + 4
    return out


def tiled(paths, size=6):
    tiles = [read_png(p) for p in paths]
    rng = random.Random(1)
    out = blank(TILE_WIDTH * size, TILE_HEIGHT * (size + 1), BACKGROUND)
    for ty in range(size):
        for tx in range(size):
            tile = rng.choice(tiles)
            paste(out, [row[:TILE_WIDTH] for row in tile],
                  (tx - ty) * TILE_WIDTH // 2 + (size - 1) * TILE_WIDTH // 2,
                  (tx + ty) * TILE_HEIGHT // 2)
    return out


def main():
    if len(sys.argv) < 4 or sys.argv[1] not in ("strip", "tiled"):
        sys.exit(__doc__)
    mode, out, paths = sys.argv[1], sys.argv[2], sys.argv[3:]
    write_png(out, scale(strip(paths) if mode == "strip" else tiled(paths), SCALE))
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
