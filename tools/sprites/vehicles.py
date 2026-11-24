"""Generates the vehicle sprites in resource/sprite/units/.

Run from the project root:  python tools/sprites/vehicles.py [output_dir]

battle_tank.png: one row per facing, in world::Direction order (N, NE, E, SE, S, SW, W, NW);
column 0 is parked, columns 1..DRIVE_FRAMES the drive cycle (the tread links roll). Frame
size, anchor and clips (idle, drive) go to battle_tank.sprite (pixel.write_sprite).

The tank is a small voxel model in its own frame (forward / right in tiles, up in pixels),
turned to each facing in world space and projected like the game camera (a tile is a
64x32 diamond), drawn with a depth buffer, so the eight directions stay consistent.
"""
import math
import os
import sys

from pixel import blank, shade, write_sprite

FRAME_W, FRAME_H = 80, 64
ANCHOR = (40, 42)            # the tank's centre on the ground point
DIRECTIONS = 8
DRIVE_FRAMES = 4
DRIVE_FPS = 8
COLUMNS = 1 + DRIVE_FRAMES
TILE_W, TILE_H = 64, 32
STEP = 0.02                  # voxel size along the ground, in tiles

# Olive drab with dark steel and a glowing sight, to match the cyberpunk-industrial kit.
HULL = (96, 110, 66)
HULL_DARK = (70, 80, 50)
TURRET = (106, 120, 72)
TREAD = (40, 40, 44)
TREAD_LINK = (70, 70, 76)
WHEEL = (58, 60, 64)
BARREL = (54, 58, 52)
SIGHT = (70, 245, 255)
HAZARD = (230, 180, 40)
OUTLINE = (16, 16, 22)


def world_forward(direction):
    """The world direction (x, y) a facing looks along: screen angle -> world, normalized."""
    angle = direction * math.pi / 4  # clockwise from North (screen up)
    sx, sy = math.sin(angle), -math.cos(angle)
    # Inverse of screen = ((x - y) * W/2, (x + y) * H/2).
    x = sx / TILE_W + sy / TILE_H
    y = sy / TILE_H - sx / TILE_W
    length = math.hypot(x, y)
    return x / length, y / length


def boxes(phase):
    """The model as boxes in the tank frame: (f0, f1, r0, r1, z0, z1, colour_at)."""
    def tread(f, r, z):
        # Links every 0.08 tiles along the track, sliding by `phase` while driving.
        link = (f + phase * 0.08) / 0.08
        return TREAD_LINK if (link % 1.0) < 0.35 else TREAD

    def hull(f, r, z):
        if z >= 13 and abs(r) > 0.2 and int((f + 0.5) / 0.1) % 2 == 0:
            return HAZARD if f > 0.38 else HULL_DARK
        return HULL

    def turret(f, r, z):
        if z >= 17 and 0.08 < f < 0.14 and -0.11 < r < -0.03:
            return SIGHT
        return TURRET

    return [
        # treads (left, right) with road wheels showing at the side
        (-0.48, 0.48, -0.32, -0.18, 0, 7, tread),
        (-0.48, 0.48, 0.18, 0.32, 0, 7, tread),
        (-0.44, 0.44, -0.33, -0.31, 1, 5, lambda f, r, z: WHEEL),
        (-0.44, 0.44, 0.31, 0.33, 1, 5, lambda f, r, z: WHEEL),
        # hull, a lower and an upper plate
        (-0.44, 0.46, -0.24, 0.24, 4, 10, lambda f, r, z: HULL_DARK),
        (-0.40, 0.42, -0.27, 0.27, 10, 14, hull),
        # turret and barrel
        (-0.18, 0.16, -0.15, 0.15, 14, 20, turret),
        (0.16, 0.62, -0.025, 0.025, 16, 18, lambda f, r, z: BARREL),
        (0.56, 0.64, -0.04, 0.04, 15, 19, lambda f, r, z: BARREL),
    ]


def frange(a, b, step):
    n = max(1, int(round((b - a) / step)))
    return [a + (i + 0.5) * (b - a) / n for i in range(n)]


def render(direction, phase):
    fx, fy = world_forward(direction)
    rx, ry = -fy, fx  # right of forward in world space
    image = blank(FRAME_W, FRAME_H)
    depth = [[-1e9] * FRAME_W for _ in range(FRAME_H)]

    def splat(f, r, z, color):
        x, y = f * fx + r * rx, f * fy + r * ry
        sx = ANCHOR[0] + (x - y) * TILE_W / 2
        sy = ANCHOR[1] + (x + y) * TILE_H / 2 - z
        d = (x + y) * TILE_H / 2 + z  # towards the viewer and up = in front
        for px in (int(sx), int(sx + 0.5)):
            py = int(sy)
            if 0 <= px < FRAME_W and 0 <= py < FRAME_H and d > depth[py][px]:
                depth[py][px] = d
                image[py][px] = (*color, 255)

    def side_light(nf, nr):
        # World normal of a side face; light from the top left of the screen (world -x),
        # so faces towards +y (screen left-down) are lit, towards +x darker.
        nx, ny = nf * fx + nr * rx, nf * fy + nr * ry
        return 0.82 + 0.12 * ny - 0.12 * nx

    for f0, f1, r0, r1, z0, z1, colour_at in boxes(phase):
        fs, rs = frange(f0, f1, STEP), frange(r0, r1, STEP)
        # top face
        for f in fs:
            for r in rs:
                splat(f, r, z1, shade(colour_at(f, r, z1), 1.12))
        # four sides, one pixel of height at a time
        for z in range(z0, z1):
            for f in fs:
                splat(f, r0, z, shade(colour_at(f, r0, z), side_light(0, -1)))
                splat(f, r1, z, shade(colour_at(f, r1, z), side_light(0, 1)))
            for r in rs:
                splat(f0, r, z, shade(colour_at(f0, r, z), side_light(-1, 0)))
                splat(f1, r, z, shade(colour_at(f1, r, z), side_light(1, 0)))
    return outlined(image)


def outlined(image):
    """A one-pixel dark outline around the silhouette, so it reads on any ground."""
    out = [row[:] for row in image]
    for y in range(FRAME_H):
        for x in range(FRAME_W):
            if image[y][x][3]:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < FRAME_W and 0 <= ny < FRAME_H and image[ny][nx][3]:
                    out[y][x] = (*OUTLINE, 255)
                    break
    return out


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join("resource", "sprite", "units")
    os.makedirs(out, exist_ok=True)
    frames = []
    for direction in range(DIRECTIONS):
        frames.append(render(direction, 0.0))
        for frame in range(DRIVE_FRAMES):
            frames.append(render(direction, frame / DRIVE_FRAMES))
    write_sprite(out, "battle_tank", frames, COLUMNS, anchor=ANCHOR,
                 clips={"idle": (0, 1, 0, True), "drive": (1, DRIVE_FRAMES, DRIVE_FPS, True)})
    print(f"wrote battle_tank to {out}")


if __name__ == "__main__":
    main()
