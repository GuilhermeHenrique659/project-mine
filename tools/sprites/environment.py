"""Generates the terrain sprites in resource/sprite/environment/.

Run from the project root:  python tools/sprites/environment.py [output_dir]
File names must match graphics::register_game_textures() (src/graphics/texture_catalog.cpp)
and the counts below the constants in src/graphics/terrain_sprites.hpp.
"""
import math
import os
import random
import sys

from pixel import TILE_HEIGHT as H, TILE_WIDTH as W, diamond_tile, mix, on_diamond_rim, \
    shade, sheet, write_png

GRASS_VARIANTS = 4   # graphics::GrassVariants
WATER_FRAMES = 8     # graphics::WaterFrames


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
    for i in range(GRASS_VARIANTS):
        write_png(os.path.join(out, f"grass_{i}.png"), grass(i))
    write_png(os.path.join(out, "iron_ore.png"), ore(11, (120, 104, 96), (190, 120, 90)))
    write_png(os.path.join(out, "copper_ore.png"), ore(12, (150, 96, 60), (230, 150, 80)))
    write_png(os.path.join(out, "coal.png"), ore(13, (46, 44, 48), (90, 90, 100)))
    write_png(os.path.join(out, "water_sheet.png"),
              sheet([water(f) for f in range(WATER_FRAMES)], WATER_FRAMES))
    print(f"wrote environment sprites to {out}")


if __name__ == "__main__":
    main()
