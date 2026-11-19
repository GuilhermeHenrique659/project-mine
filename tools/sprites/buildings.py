"""Generates the building sprites in resource/sprite/buildings/.

Run from the project root:  python tools/sprites/buildings.py [output_dir]

Each building is a sheet of equal frames (one row) whose ground anchor, the screen point
of the footprint's centre, sits at a fixed pixel of the frame. Sizes, frame counts and the
anchor are written to the .sprite file next to the sheet (pixel.write_sprite); the game
finds it as buildings/<building type in snake_case>, so a new building sprite needs no C++.
"""
import math
import os
import random
import sys

from iso import IsoCanvas, lit_faces
from pixel import blank, mix, shade, write_sprite

# Headquarters: 5x5 tiles -> a 320x160 ground diamond.
HQ_FRAME_W, HQ_FRAME_H = 336, 352
HQ_ANCHOR = (168, 264)          # the footprint centre
HQ_FRAMES = 8                   # one antenna turn
HQ_FPS = 4

# Palette: grimy industrial metal with cyberpunk neon.
CONCRETE = (92, 92, 98)
STEEL = (58, 62, 72)
DARK_STEEL = (36, 38, 46)
RUST = (122, 74, 46)
HAZARD = (230, 180, 40)
CYAN = (60, 240, 250)
MAGENTA = (250, 60, 200)
AMBER = (255, 150, 40)
RED = (255, 50, 60)


def panels(base, seams_u, seams_v, glow=None, rng=None, dirt=0.08):
    """A face of riveted metal panels; `glow(u, v)` may return a neon color instead."""
    rng = rng or random.Random(0)
    noise = {}

    def face(u, v):
        if glow:
            neon = glow(u, v)
            if neon:
                return neon
        key = (round(u * 60), round(v * 60))
        if key not in noise:
            noise[key] = 1.0 - dirt + rng.random() * dirt * 2
        col = shade(base, noise[key])
        if any(abs(u - s) < 0.008 for s in seams_u) or any(abs(v - s) < 0.012 for s in seams_v):
            col = shade(col, 0.6)
        return col
    return face


def hazard_stripes(u, v):
    return HAZARD if int((u * 30 + v * 3)) % 2 == 0 else (30, 30, 30)


def window_grid(color_a, color_b, columns, rows, lit_seed):
    rng = random.Random(lit_seed)
    lit = {(c, r): rng.random() < 0.7 for c in range(columns) for r in range(rows)}

    def glow(u, v):
        if not (0.08 < u < 0.92 and 0.25 < v < 0.8):
            return None
        cu, cv = (u - 0.08) / 0.84 * columns, (v - 0.25) / 0.55 * rows
        c, r = int(cu), int(cv)
        if cu - c > 0.65 or cv - r > 0.6:
            return None
        if not lit.get((c, r)):
            return (20, 24, 34)
        return mix(color_a, color_b, (c + r) % 2 * 0.6)
    return glow


def neon_band(color, low, high):
    return lambda u, v: color if low < v < high else None


def headquarters(frame):
    image = blank(HQ_FRAME_W, HQ_FRAME_H)
    canvas = IsoCanvas(image, HQ_ANCHOR)
    rng = random.Random(7)

    # Concrete plinth with hazard edges.
    canvas.box(-2.45, -2.45, 2.45, 2.45, 0, 8, panels(CONCRETE, [], [], dirt=0.12, rng=rng),
               hazard_stripes, lambda u, v: shade(hazard_stripes(u, v), 0.7))

    # Back stacks (drawn first: behind the main block).
    for sx, sy in ((-1.9, -1.9), (1.3, -1.9), (-1.9, 1.3)):
        top, left, right = lit_faces(RUST)
        canvas.box(sx, sy, sx + 0.6, sy + 0.6, 8, 150,
                   (24, 22, 22), panels(left, [], [0.3, 0.6, 0.9], rng=rng),
                   panels(right, [], [0.3, 0.6, 0.9], rng=rng))
        canvas.box(sx - 0.05, sy - 0.05, sx + 0.65, sy + 0.65, 140, 146, AMBER,
                   shade(AMBER, 0.9), shade(AMBER, 0.7))

    # Main block: dark steel with window grids and a cyan roof strip.
    top, left, right = lit_faces(STEEL)
    canvas.box(-1.6, -1.6, 1.6, 1.6, 8, 96,
               panels(top, [0.33, 0.66], [0.33, 0.66], rng=rng),
               panels(left, [0.25, 0.5, 0.75], [0.5], window_grid(CYAN, MAGENTA, 6, 3, 1), rng),
               panels(right, [0.25, 0.5, 0.75], [0.5], window_grid(MAGENTA, CYAN, 6, 3, 2), rng))
    # Roof: a thin cyan neon trim around a dark deck.
    canvas.box(-1.62, -1.62, 1.62, 1.62, 92, 96, CYAN, shade(CYAN, 0.85), shade(CYAN, 0.65))
    top, _, _ = lit_faces(DARK_STEEL)
    canvas.box(-1.5, -1.5, 1.5, 1.5, 96, 97, panels(top, [0.25, 0.5, 0.75], [0.25, 0.5, 0.75],
                                                    rng=rng), top, top)

    # Front pipes along the left face, and a vent box at the right corner.
    for i, offset in enumerate((0.15, 0.35)):
        y = 1.6 + offset
        canvas.line((-1.4, y, 10), (1.4, y, 10), shade((150, 150, 160), 1 - i * 0.2), 3)
        canvas.line((1.4, y, 10), (1.4, y, 60 - i * 10), shade((150, 150, 160), 1 - i * 0.2), 3)
    top, left, right = lit_faces(DARK_STEEL)
    canvas.box(1.7, 0.4, 2.3, 1.4, 8, 36, panels(top, [0.5], [], rng=rng),
               panels(left, [], [0.2, 0.4, 0.6, 0.8], rng=rng),
               panels(right, [], [0.2, 0.4, 0.6, 0.8], rng=rng))

    # Upper control tower with a magenta sign band.
    top, left, right = lit_faces(DARK_STEEL)
    canvas.box(-0.8, -0.8, 0.8, 0.8, 96, 150, panels(top, [], [], rng=rng),
               panels(left, [0.5], [], neon_band(MAGENTA, 0.45, 0.62), rng),
               panels(right, [0.5], [], neon_band(MAGENTA, 0.45, 0.62), rng))
    canvas.box(-0.85, -0.85, 0.85, 0.85, 150, 154, (90, 95, 110), (70, 74, 86), (50, 54, 64))

    # Antenna: a lattice mast with a spinning radar arm (a dish on one end, a counterweight
    # on the other, so a whole turn reads), and a blinking tip light.
    base_z, mast_top = 154, 236
    canvas.line((-0.08, 0, base_z), (0, 0, mast_top), (170, 175, 190), 2)
    canvas.line((0.08, 0, base_z), (0, 0, mast_top), (130, 135, 150), 2)
    for z in range(base_z + 8, mast_top - 8, 10):
        canvas.line((-0.06, 0, z), (0.06, 0, z + 6), (120, 125, 140), 1)
    angle = frame / HQ_FRAMES * 2 * math.pi
    arm, arm_z = 1.35, mast_top - 10
    dx, dy = math.cos(angle) * arm, math.sin(angle) * arm
    dish, weight = (dx, dy, arm_z), (-dx * 0.45, -dy * 0.45, arm_z)
    canvas.line(weight, dish, (205, 210, 225), 3)
    canvas.line((dx * 0.5, dy * 0.5, arm_z), (dx * 0.8, dy * 0.8, arm_z + 10), (160, 165, 180), 2)
    canvas.dot(weight, (100, 100, 112), 3.5)
    canvas.dot(dish, (225, 230, 240), 6.5)
    canvas.dot(dish, CYAN, 3)
    canvas.dot((0, 0, arm_z), (80, 80, 92), 3)
    canvas.dot((0, 0, mast_top + 3), RED if frame % 2 == 0 else (110, 30, 30), 2.5)
    return image


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join("resource", "sprite", "buildings")
    os.makedirs(out, exist_ok=True)
    write_sprite(out, "headquarters", [headquarters(f) for f in range(HQ_FRAMES)], HQ_FRAMES,
                 anchor=HQ_ANCHOR, clips={"idle": (0, HQ_FRAMES, HQ_FPS, True)})
    print(f"wrote building sprites to {out}")


if __name__ == "__main__":
    main()
