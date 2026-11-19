"""Generates the unit sprites in resource/sprite/units/.

Run from the project root:  python tools/sprites/units.py [output_dir]

character_sheet.png: one row per facing, in world::Direction order (N, NE, E, SE, S, SW,
W, NW); column 0 is standing, columns 1..WALK_FRAMES the walk cycle. Frame size, anchor
and clips (idle, walk, run) go to character.sprite (pixel.write_sprite), read by the game.

The figure is a little 3D model (capsules and spheres in the character's own frame),
turned to each facing and projected the way the game's camera sees it, so all eight
directions are consistent and the walk swings legs and arms along the facing.
"""
import math
import os
import sys

from pixel import blank, shade, write_sprite

FRAME_W, FRAME_H = 32, 48
ANCHOR = (16, 44)        # the feet on the ground point
DIRECTIONS = 8
WALK_FRAMES = 6
WALK_FPS, RUN_FPS = 10, 16
COLUMNS = 1 + WALK_FRAMES

# Cyberpunk-industrial worker: hi-vis jumpsuit, dark armour bits, glowing visor.
SUIT = (214, 120, 40)
SUIT_DARK = (150, 80, 30)
ARMOUR = (52, 56, 66)
BOOTS = (34, 34, 40)
SKIN = (60, 64, 76)      # gloves
VISOR = (70, 245, 255)
PACK = (70, 74, 86)
PACK_LIGHT = (255, 60, 200)
OUTLINE = (16, 16, 22)


def facing_vectors(direction):
    """Forward and right on the ground (screen x right, screen y towards the viewer)."""
    angle = direction * math.pi / 4  # clockwise from North (screen up)
    forward = (math.sin(angle), -math.cos(angle))
    right = (-forward[1], forward[0])
    return forward, right


def project(point):
    gx, gy, z = point
    return ANCHOR[0] + gx, ANCHOR[1] + gy * 0.5 - z


class Figure:
    """Capsules drawn back to front (by ground depth) with a light from the top left."""

    def __init__(self):
        self.parts = []

    def capsule(self, a, b, radius, color, bias=0.0):
        depth = (a[1] + b[1]) / 2 + bias
        self.parts.append((depth, len(self.parts), a, b, radius, color))

    def render(self):
        image = blank(FRAME_W, FRAME_H)
        for _, _, a, b, radius, color in sorted(self.parts):
            (ax, ay), (bx, by) = project(a), project(b)
            for y in range(FRAME_H):
                for x in range(FRAME_W):
                    px, py = x + 0.5, y + 0.5
                    dx, dy = bx - ax, by - ay
                    length = dx * dx + dy * dy
                    t = 0.0 if length == 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / length))
                    cx, cy = ax + dx * t, ay + dy * t
                    d = math.hypot(px - cx, py - cy)
                    if d > radius:
                        continue
                    # Light from the top left: brighter where the offset points that way.
                    light = 1.0 - ((px - cx) + (py - cy)) / (radius * 2 + 0.01) * 0.35
                    image[y][x] = (*shade(color, light), 255)
        return outlined(image)


def outlined(image):
    """A one-pixel dark outline around the figure, so it reads on any ground."""
    out = [row[:] for row in image]
    for y in range(FRAME_H):
        for x in range(FRAME_W):
            if image[y][x][3]:
                continue
            if any(0 <= x + dx < FRAME_W and 0 <= y + dy < FRAME_H and image[y + dy][x + dx][3]
                   for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                out[y][x] = (*OUTLINE, 255)
    return out


def character(direction, phase, walking):
    forward, right = facing_vectors(direction)
    fig = Figure()

    def at(f, r, z):
        """A point `f` px forward, `r` px to the right, `z` px up from the feet."""
        return (forward[0] * f + right[0] * r, forward[1] * f + right[1] * r, z)

    swing = math.sin(phase) if walking else 0.0
    bob = abs(math.cos(phase)) * 1.0 if walking else 0.0

    # Legs and boots (left = -r). A leg swings forward while the other goes back.
    for side, s in ((-1, swing), (1, -swing)):
        hip = at(0, side * 2.5, 13 + bob)
        foot = at(s * 4.5, side * 2.5, max(0.0, -s) * 2.0 * (1 if walking else 0) + 1.5)
        fig.capsule(hip, foot, 2.2, SUIT_DARK if side < 0 else SUIT)
        fig.capsule(foot, at(s * 4.5 + 1.2, side * 2.5, 1.2), 1.8, BOOTS, bias=0.1)

    # Torso with an armour vest, and the backpack (behind, so drawn first when facing us).
    fig.capsule(at(0, 0, 14 + bob), at(0, 0, 24 + bob), 4.6, SUIT)
    fig.capsule(at(0.8, 0, 17 + bob), at(0.8, 0, 23 + bob), 3.8, ARMOUR, bias=0.2)
    fig.capsule(at(-4.0, 0, 16 + bob), at(-4.0, 0, 23 + bob), 3.0, PACK)
    fig.capsule(at(-6.6, 0, 21 + bob), at(-6.6, 0, 21.5 + bob), 0.9, PACK_LIGHT, bias=-0.1)

    # Arms swing opposite to the legs.
    for side, s in ((-1, -swing), (1, swing)):
        shoulder = at(0, side * 5.0, 23 + bob)
        hand = at(s * 3.5, side * 5.6, 15 + bob)
        fig.capsule(shoulder, hand, 1.8, SUIT_DARK if side < 0 else SUIT)
        fig.capsule(hand, hand, 1.6, SKIN, bias=0.05)

    # Helmet with a glowing visor across its front.
    head = at(0, 0, 30 + bob)
    fig.capsule(head, head, 4.4, ARMOUR)
    fig.capsule(at(3.2, -2.2, 30 + bob), at(3.2, 2.2, 30 + bob), 1.3, VISOR, bias=0.3)
    fig.capsule(at(-0.5, 0, 34.5 + bob), at(-2.5, 0, 34.0 + bob), 0.9, VISOR, bias=-0.2)
    return fig.render()


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join("resource", "sprite", "units")
    os.makedirs(out, exist_ok=True)
    frames = []
    for direction in range(DIRECTIONS):
        frames.append(character(direction, 0.0, False))
        frames += [character(direction, i / WALK_FRAMES * 2 * math.pi, True)
                   for i in range(WALK_FRAMES)]
    write_sprite(out, "character", frames, COLUMNS, anchor=ANCHOR,
                 clips={"idle": (0, 1, 0, True), "walk": (1, WALK_FRAMES, WALK_FPS, True),
                        "run": (1, WALK_FRAMES, RUN_FPS, True)})
    print(f"wrote unit sprites to {out}")


if __name__ == "__main__":
    main()
