"""Placeholder window icon: an orange pickaxe-ish diamond on a dark tile -> resource/icon.png."""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from pixel import write_png  # noqa: E402

SIZE = 32
BG = (30, 34, 42, 255)
BORDER = (90, 100, 115, 255)
ORE = (230, 140, 40, 255)
ORE_LIGHT = (255, 200, 110, 255)


def pixel_at(x, y):
    if x in (0, SIZE - 1) or y in (0, SIZE - 1):
        return BORDER
    c = (SIZE - 1) / 2
    d = abs(x - c) + abs(y - c)
    if d <= 11:
        return ORE_LIGHT if (x - c) + (y - c) < -5 else ORE
    return BG


def main():
    image = [[pixel_at(x, y) for x in range(SIZE)] for y in range(SIZE)]
    out = os.path.join("resource", "icon.png")
    write_png(out, image)
    print("wrote", out)


if __name__ == "__main__":
    main()
