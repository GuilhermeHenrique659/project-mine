"""Generates the terrain sprites in resource/sprite/environment/.

Run from the project root:  python tools/sprites/environment.py [output_dir]
Each sprite is a sheet plus its .sprite file (pixel.write_sprite); the game finds it as
environment/<tile type> (graphics::tile_sprite_name()), so nothing in the C++ needs editing.
Grass variants are the rows of one sheet; the game picks a row per tile.
"""
import math
import os
import random
import sys

from pixel import TILE_HEIGHT as H, TILE_WIDTH as W, diamond_tile, mix, on_diamond_rim, \
    shade, write_sprite

GRASS_VARIANTS = 4
WATER_FRAMES = 8
WATER_FPS = 8


def grass(seed):
    rng = random.Random(seed)
    base = (78, 140, 62)
    noise = {(x, y): rng.random() for x in range(W) for y in range(H)}
    blades = {(rng.randrange(W), rng.randrange(H)) for _ in range(90)}
    flowers = {(rng.randrange(W), rng.randrange(H)):
               rng.choice([(235, 225, 120), (230, 230, 235), (220, 140, 170)]) for _ in range(3)}

    def color(x, y):
        # No edge shading: tiles must join without a visible grid.
        col = shade(base, 0.9 + noise[(x, y)] * 0.2)
        if (x, y) in blades:
            col = (104, 170, 78)
        if (x, y - 1) in blades:
            col = (60, 112, 50)
        if (x, y) in flowers and not on_diamond_rim(x, y):
            col = flowers[(x, y)]
        return col
    return diamond_tile(color)


def ore(seed, rock, fleck):
    """Grass with rock chunks, kept off the rim so a neighbour never cuts them."""
    rng = random.Random(seed)
    image = grass(seed + 100)
    for _ in range(9):
        cx, cy = rng.uniform(12, 52), rng.uniform(7, 25)
        rx, ry = rng.uniform(3, 6), rng.uniform(2, 3.5)
        for y in range(H):
            for x in range(W):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 > 1 or not image[y][x][3] \
                        or on_diamond_rim(x, y):
                    continue
                if (y - cy) < -ry * 0.3 and (x - cx) < 0:
                    light = 1.15  # lit from the top left
                elif (y - cy) > ry * 0.4:
                    light = 0.75
                else:
                    light = 1.0
                col = shade(rock, light * (0.92 + rng.random() * 0.16))
                if rng.random() < 0.12:
                    col = fleck
                image[y][x] = (*col, 255)
    return image


def water(frame):
    """One frame of a seamless looping wave pattern (the waves follow the iso grid)."""
    t = frame / WATER_FRAMES * 2 * math.pi
    deep, light, foam = (40, 92, 160), (70, 140, 205), (180, 220, 245)

    def color(x, y):
        u, v = x / W + y / H, y / H - x / W
        wave = (math.sin(2 * math.pi * 2 * u + t) * 0.5 + math.sin(2 * math.pi * 3 * v - t) * 0.3
                + math.sin(2 * math.pi * (u + v) * 2 + t) * 0.2)
        return foam if wave > 0.78 else mix(deep, light, (wave + 1) / 2)
    return diamond_tile(color)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join("resource", "sprite", "environment")
    os.makedirs(out, exist_ok=True)
    write_sprite(out, "grass", [grass(i) for i in range(GRASS_VARIANTS)], 1)
    write_sprite(out, "iron_ore", [ore(11, (120, 104, 96), (190, 120, 90))], 1)
    write_sprite(out, "copper_ore", [ore(12, (150, 96, 60), (230, 150, 80))], 1)
    write_sprite(out, "coal", [ore(13, (46, 44, 48), (90, 90, 100))], 1)
    write_sprite(out, "crude_oil", [ore(14, (24, 20, 28), (70, 50, 90))], 1)
    write_sprite(out, "water", [water(f) for f in range(WATER_FRAMES)], WATER_FRAMES,
                 clips={"idle": (0, WATER_FRAMES, WATER_FPS, True)})
    print(f"wrote environment sprites to {out}")


if __name__ == "__main__":
    main()
