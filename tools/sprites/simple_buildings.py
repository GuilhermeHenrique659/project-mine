"""Simple sprites for every building other than the headquarters (see buildings.py).

Run from the project root:  python tools/sprites/simple_buildings.py [output_dir]

Each building is a plinth covering its footprint, a main block in its own colour and a
few roof details; the Oil Pump rocks its arm. Frames are sized from the footprint so the
anchor (the footprint centre) always sits at the same spot.
"""
import math
import os
import random
import sys

from buildings import AMBER, CONCRETE, CYAN, DARK_STEEL, HAZARD, MAGENTA, RED, RUST, STEEL, \
    hazard_stripes, panels, window_grid
from iso import IsoCanvas, lit_faces
from pixel import blank, shade, write_sprite

MARGIN = 8
PUMP_FRAMES, PUMP_FPS = 8, 6


def frame_for(size, height):
    """A blank frame and canvas for a size x size footprint with `height` px above it."""
    w = size * 64 + MARGIN * 2
    h = size * 32 + height + MARGIN * 2
    anchor = (w // 2, h - MARGIN - size * 16)
    image = blank(w, h)
    return image, IsoCanvas(image, anchor), anchor


def trim(canvas, half, z0, z1, color):
    canvas.box(-half, -half, half, half, z0, z1, color, shade(color, 0.85), shade(color, 0.65))


def plinth(canvas, size, rng, color=CONCRETE, z=6):
    half = size / 2 - 0.03
    top, left, right = lit_faces(color)
    canvas.box(-half, -half, half, half, 0, z, panels(top, [], [], rng=rng, dirt=0.12),
               left, right)


def block(canvas, half, z0, z1, color, rng, glow_left=None, glow_right=None, seams=3):
    top, left, right = lit_faces(color)
    cuts = [i / seams for i in range(1, seams)]
    canvas.box(-half, -half, half, half, z0, z1, panels(top, cuts, cuts, rng=rng),
               panels(left, cuts, [], glow_left, rng), panels(right, cuts, [], glow_right, rng))


def stack(canvas, x, y, z0, z1, color, rng, cap=AMBER, r=0.22):
    _, left, right = lit_faces(color)
    canvas.box(x - r, y - r, x + r, y + r, z0, z1, (24, 22, 22),
               panels(left, [], [0.5], rng=rng), panels(right, [], [0.5], rng=rng))
    canvas.box(x - r - 0.03, y - r - 0.03, x + r + 0.03, y + r + 0.03, z1 - 4, z1, cap,
               shade(cap, 0.85), shade(cap, 0.65))


def door(color):
    return lambda u, v: color if 0.4 < u < 0.6 and v < 0.45 else None


def factory(base, accent, stacks=1, seed=0, tank=None):
    def draw():
        rng = random.Random(seed)
        image, canvas, anchor = frame_for(3, 120)
        plinth(canvas, 3, rng)
        for i in range(stacks):
            stack(canvas, -1.1 + i * 0.6, -1.1, 6, 100 - i * 12, RUST, rng)
        block(canvas, 1.25, 6, 58, base, rng, window_grid(accent, CYAN, 4, 2, seed),
              door(shade(accent, 0.6)))
        trim(canvas, 1.27, 54, 58, accent)
        trim(canvas, 1.15, 58, 59, shade(base, 0.7))
        top, left, right = lit_faces(tank or DARK_STEEL)
        canvas.box(0.1, 0.1, 0.9, 0.9, 58, 78 if tank else 70, top, left, right)
        return [image], anchor
    return draw


def storage(base, accent, seed, liquid=False):
    def draw():
        rng = random.Random(seed)
        image, canvas, anchor = frame_for(2, 80)
        plinth(canvas, 2, rng)
        if liquid:
            for x, y in ((-0.45, -0.45), (0.45, 0.45)):
                top, left, right = lit_faces(base)
                canvas.box(x - 0.4, y - 0.4, x + 0.4, y + 0.4, 6, 50, top,
                           panels(left, [], [0.5], rng=rng), panels(right, [], [0.5], rng=rng))
                canvas.box(x - 0.42, y - 0.42, x + 0.42, y + 0.42, 32, 36, accent,
                           shade(accent, 0.85), shade(accent, 0.65))
        else:
            block(canvas, 0.85, 6, 40, base, rng, door(shade(base, 0.5)), None, seams=4)
            for x in (-0.4, 0.3):
                top, left, right = lit_faces(accent)
                canvas.box(x, 0.2, x + 0.4, 0.6, 40, 54, top, left, right)
        return [image], anchor
    return draw


def house():
    rng = random.Random(21)
    image, canvas, anchor = frame_for(2, 80)
    plinth(canvas, 2, rng)
    block(canvas, 0.75, 6, 36, (150, 130, 100), rng, window_grid(AMBER, AMBER, 2, 1, 3),
          door((80, 50, 30)), seams=2)
    for i in range(10):  # a stepped roof
        trim(canvas, 0.8 - i * 0.08, 36 + i * 3, 39 + i * 3, shade((150, 60, 50), 1.1 - i * 0.03))
    return [image], anchor


def mine():
    rng = random.Random(31)
    image, canvas, anchor = frame_for(2, 100)
    plinth(canvas, 2, rng, color=(100, 88, 72))
    canvas.box(-0.5, -0.5, 0.5, 0.5, 6, 7, (20, 18, 16), (20, 18, 16), (20, 18, 16))  # shaft
    for x, y in ((-0.6, -0.6), (0.6, -0.6), (-0.6, 0.6), (0.6, 0.6)):
        canvas.line((x, y, 6), (0, 0, 80), (150, 110, 70), 3)
    canvas.line((0, 0, 80), (0, 0, 8), (60, 60, 60), 1)
    canvas.dot((0, 0, 80), (200, 200, 210), 5)
    top, left, right = lit_faces(STEEL)
    canvas.box(0.45, 0.45, 0.9, 0.9, 6, 30, top, left, right)
    return [image], anchor


def coal_generator():
    rng = random.Random(41)
    image, canvas, anchor = frame_for(3, 130)
    plinth(canvas, 3, rng)
    stack(canvas, -1.0, -1.0, 6, 120, (70, 66, 64), rng, cap=RED, r=0.3)
    stack(canvas, -0.2, -1.0, 6, 104, (70, 66, 64), rng, cap=RED, r=0.3)
    block(canvas, 1.2, 6, 50, (64, 66, 70), rng, None, door(HAZARD))
    canvas.box(-1.22, -1.22, 1.22, 1.22, 46, 50, HAZARD, hazard_stripes,
               lambda u, v: shade(hazard_stripes(u, v), 0.7))
    for x in (-0.6, 0.0, 0.6):  # copper coils
        top, left, right = lit_faces((200, 120, 60))
        canvas.box(x - 0.2, 0.3, x + 0.2, 0.9, 50, 70, top, left, right)
    return [image], anchor


def barracks():
    rng = random.Random(51)
    image, canvas, anchor = frame_for(3, 110)
    plinth(canvas, 3, rng, color=(90, 96, 80))
    block(canvas, 1.25, 6, 44, (84, 96, 64), rng, window_grid(AMBER, AMBER, 5, 1, 9),
          door((40, 44, 30)))
    trim(canvas, 1.27, 44, 48, (60, 70, 46))
    canvas.line((-0.8, -0.8, 48), (-0.8, -0.8, 100), (190, 190, 200), 2)
    for z in range(84, 100):  # flag
        canvas.line((-0.8, -0.8, z), (-0.8, -0.3, z - 4), RED, 1)
    return [image], anchor


def training_target():
    image, canvas, anchor = frame_for(1, 60)
    canvas.box(-0.45, -0.45, 0.45, 0.45, 0, 4, (120, 90, 60), (100, 74, 50), (80, 58, 40))
    canvas.line((-0.25, 0.1, 4), (-0.25, 0.1, 30), (110, 80, 50), 2)
    canvas.line((0.25, -0.1, 4), (0.25, -0.1, 30), (110, 80, 50), 2)
    for r, col in ((14, (235, 235, 235)), (10, RED), (6, (235, 235, 235)), (2.5, RED)):
        canvas.dot((0, 0, 38), col, r)
    return [image], anchor


def pipe():
    image, canvas, anchor = frame_for(1, 30)
    metal = (150, 150, 160)
    canvas.box(-0.5, -0.14, 0.5, 0.14, 4, 16, metal, shade(metal, 0.85), shade(metal, 0.65))
    canvas.box(-0.14, -0.5, 0.14, 0.5, 4, 16, metal, shade(metal, 0.85), shade(metal, 0.65))
    canvas.box(-0.2, -0.2, 0.2, 0.2, 2, 20, (90, 92, 100), (75, 76, 84), (60, 60, 68))
    return [image], anchor


def oil_pump():
    frames = []
    for f in range(PUMP_FRAMES):
        rng = random.Random(61)
        image, canvas, anchor = frame_for(2, 100)
        plinth(canvas, 2, rng, color=(70, 68, 66))
        top, left, right = lit_faces(DARK_STEEL)
        canvas.box(0.4, 0.2, 0.9, 0.8, 6, 24, top, left, right)  # motor
        canvas.line((-0.1, 0.25, 6), (0, 0, 60), (190, 160, 40), 3)
        canvas.line((0.1, -0.25, 6), (0, 0, 60), (160, 130, 30), 3)
        tilt = math.sin(f / PUMP_FRAMES * 2 * math.pi) * 14
        head, tail = (-0.8, 0.0, 60 + tilt), (0.7, 0.0, 60 - tilt)
        canvas.line((-0.8, 0.0, 60 + tilt), (-0.8, 0.0, 8), (40, 40, 40), 1)
        canvas.line(tail, head, (200, 170, 50), 4)
        canvas.dot(head, (60, 60, 66), 6)
        canvas.dot(tail, (90, 90, 96), 5)
        canvas.dot((0, 0, 60), (40, 40, 44), 3)
        frames.append(image)
    return frames, anchor


BUILDINGS = {
    "storage": storage((120, 100, 70), (150, 120, 80), 71),
    "liquid_storage": storage((170, 172, 180), (40, 40, 44), 72, liquid=True),
    "steelworks": factory((90, 70, 60), AMBER, stacks=2, seed=81),
    "electronic_factory": factory(STEEL, CYAN, stacks=0, seed=82, tank=(60, 140, 90)),
    "tool_factory": factory((96, 90, 70), (200, 160, 60), seed=83),
    "ammo_factory": factory((80, 86, 60), RED, seed=84, tank=(110, 100, 60)),
    "weapon_factory": factory((60, 60, 72), MAGENTA, stacks=2, seed=85),
    "engine_factory": factory((96, 64, 56), AMBER, stacks=1, seed=86, tank=(70, 70, 78)),
    "tank_factory": factory((70, 80, 52), HAZARD, stacks=2, seed=87, tank=(90, 100, 64)),
    "house": house, "mine": mine, "coal_generator": coal_generator, "barracks": barracks,
    "training_target": training_target, "pipe": pipe,
}


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join("resource", "sprite", "buildings")
    os.makedirs(out, exist_ok=True)
    for name, draw in BUILDINGS.items():
        frames, anchor = draw()
        write_sprite(out, name, frames, 1, anchor=anchor, clips={"idle": (0, 1, 1, True)})
    frames, anchor = oil_pump()
    write_sprite(out, "oil_pump", frames, PUMP_FRAMES, anchor=anchor,
                 clips={"idle": (0, PUMP_FRAMES, PUMP_FPS, True)})
    print(f"wrote simple building sprites to {out}")


if __name__ == "__main__":
    main()
