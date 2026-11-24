"""Placeholder UI icons (32x32) -> resource/icon/<name>.png.

Names follow src/graphics/icons.hpp: icon_<building>, icon_tab_<category>, item_<item>,
menu_<button>. Shapes are deliberately simple: a coloured glyph per thing.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from pixel import blank, shade, write_png  # noqa: E402

S = 32
OUT = os.path.join("resource", "icon")


def rgb(hex_color):
    return tuple(int(hex_color[i:i + 2], 16) for i in (0, 2, 4)) + (255,)


def paint(shape, color, outline=True):
    """shape(x, y) -> True inside; adds a top-left light and a dark outline."""
    img = blank(S, S)
    dark = shade(color, 0.45) + (255,)
    light = shade(color, 1.3) + (255,)
    for y in range(S):
        for x in range(S):
            if not shape(x + 0.5, y + 0.5):
                continue
            edge = outline and not all(shape(x + 0.5 + dx, y + 0.5 + dy)
                                       for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            if edge:
                img[y][x] = dark
            elif not shape(x - 1.5, y - 1.5):
                img[y][x] = light
            else:
                img[y][x] = color
    return img




# --- shapes -----------------------------------------------------------------

def rect(x0, y0, x1, y1):
    return lambda x, y: x0 <= x < x1 and y0 <= y < y1


def circle(cx, cy, r):
    return lambda x, y: (x - cx) ** 2 + (y - cy) ** 2 <= r * r


def union(*shapes):
    return lambda x, y: any(s(x, y) for s in shapes)


def minus(a, b):
    return lambda x, y: a(x, y) and not b(x, y)


def polygon(points):
    def inside(x, y):
        hit = False
        n = len(points)
        for i in range(n):
            (x1, y1), (x2, y2) = points[i], points[(i + 1) % n]
            if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
                hit = not hit
        return hit
    return inside


def house():
    return union(rect(7, 15, 25, 28), polygon([(4, 16), (16, 5), (28, 16)]))


def factory():
    return union(rect(4, 14, 28, 28), polygon([(4, 14), (12, 8), (12, 14)]),
                 polygon([(12, 14), (20, 8), (20, 14)]), rect(22, 4, 26, 14))


def tank():
    return union(rect(7, 9, 25, 26), circle(16, 9, 9.0), circle(16, 26, 4))


def droplet():
    return union(circle(16, 20, 8), polygon([(8.5, 18), (16, 3), (23.5, 18)]))


def rocks():
    return union(circle(11, 19, 7), circle(21, 21, 6), circle(16, 12, 6))


def gear():
    def inside(x, y):
        dx, dy = x - 16, y - 16
        r = math.hypot(dx, dy)
        a = math.atan2(dy, dx)
        outer = 12 if math.cos(a * 8) > 0.3 else 9.5
        return 4 < r <= outer
    return inside


def tower():
    return union(rect(12, 8, 20, 28), rect(9, 4, 23, 10), rect(7, 26, 25, 29))


def target():
    return lambda x, y: (int(math.hypot(x - 16, y - 16) / 3.5) % 2 == 0) and math.hypot(x - 16, y - 16) < 13


def bricks():
    return lambda x, y: 4 <= x < 28 and 8 <= y < 26 and (int(y) - 8) % 6 != 0 and \
        (int(x) + (4 if ((int(y) - 8) // 6) % 2 else 0)) % 8 != 0


def pickaxe():
    head = polygon([(4, 10), (16, 4), (28, 10), (26, 12), (16, 8), (6, 12)])
    handle = polygon([(15, 7), (18, 7), (18, 29), (15, 29)])
    return union(head, handle)


def pipe():
    return union(rect(3, 12, 29, 20), rect(3, 10, 7, 22), rect(25, 10, 29, 22))


def bolt():
    return polygon([(18, 3), (8, 18), (15, 18), (13, 29), (24, 13), (17, 13)])


def bullet():
    return union(rect(11, 13, 21, 28), circle(16, 13, 5))


def chip():
    legs = lambda x, y: (6 <= x < 26 and 4 <= y < 28 and int(x) % 4 < 2) or \
        (4 <= x < 28 and 6 <= y < 26 and int(y) % 4 < 2)
    return union(rect(8, 8, 24, 24), legs)


def wrench():
    return union(minus(circle(10, 10, 7), rect(7, 2, 13, 10)),
                 polygon([(12, 14), (15, 11), (28, 25), (25, 28)]))


def rifle():
    return union(rect(3, 12, 26, 16), rect(6, 16, 14, 22), rect(18, 16, 21, 21), rect(24, 10, 29, 13))


def ingot():
    return polygon([(4, 24), (10, 12), (26, 12), (28, 24)])


def triangle_play():
    return polygon([(9, 5), (27, 16), (9, 27)])


def folder():
    return union(rect(4, 10, 28, 26), rect(4, 7, 14, 11))


def cross():
    def inside(x, y):
        dx, dy = x - 16, y - 16
        return max(abs(dx), abs(dy)) < 12 and (abs(dx - dy) < 4 or abs(dx + dy) < 4)
    return inside


def turret():
    return union(circle(16, 20, 8), rect(16, 9, 29, 14), rect(6, 26, 26, 29))


def engine():
    """An engine block from the side: cooling fins on top, body, front pulley, exhaust, sump."""
    fins = lambda x, y: 9 <= x < 25 and 4 <= y < 10 and int(x - 9) % 4 < 2
    head = rect(8, 10, 26, 13)
    body = rect(7, 13, 27, 24)
    pulley = minus(circle(6, 18, 4.5), circle(6, 18, 1.5))
    exhaust = rect(27, 15, 31, 18)
    sump = rect(10, 24, 24, 28)
    return union(fins, head, body, pulley, exhaust, sump)


def battle_tank():
    """A tank from the side: treads, hull, turret and a long barrel."""
    treads = union(rect(4, 21, 28, 27), circle(4, 24, 3), circle(28, 24, 3))
    hull = polygon([(5, 21), (8, 16), (25, 16), (28, 21)])
    turret_ = union(rect(11, 10, 21, 16), circle(16, 11, 4))
    barrel = rect(20, 11, 31, 13)
    return union(treads, hull, turret_, barrel)


# --- catalogue ---------------------------------------------------------------

ICONS = {
    # buildings (keys from construction.cpp)
    "icon_headquarters": (tower(), "d9b84a"),
    "icon_storage": (rect(5, 8, 27, 26), "a07850"),
    "icon_liquid_storage": (tank(), "5a8fb0"),
    "icon_pipe": (pipe(), "8a9099"),
    "icon_house": (house(), "c0704a"),
    "icon_coal_generator": (bolt(), "f0c040"),
    "icon_mine": (pickaxe(), "a0a8b0"),
    "icon_oil_pump": (droplet(), "303038"),
    "icon_steelworks": (factory(), "7f8c99"),
    "icon_electronic_factory": (factory(), "4aa860"),
    "icon_tool_factory": (factory(), "c08a3a"),
    "icon_ammo_factory": (factory(), "b05050"),
    "icon_weapon_factory": (factory(), "6a5a8a"),
    "icon_engine_factory": (factory(), "8a4a3c"),
    "icon_tank_factory": (battle_tank(), "5a6a3c"),
    "icon_smelter": (factory(), "d06a30"),
    "icon_assembler": (gear(), "6a8ab0"),
    "icon_barracks": (house(), "5a7a40"),
    "icon_training_target": (target(), "d04040"),
    "icon_turret": (turret(), "707a50"),
    "icon_wall": (bricks(), "9a8a7a"),
    # construction tabs
    "icon_tab_special": (tower(), "d9b84a"),
    "icon_tab_mines": (pickaxe(), "a0a8b0"),
    "icon_tab_factories": (factory(), "8a9aa8"),
    "icon_tab_military": (turret(), "707a50"),
    # items (names from world::to_string)
    "item_iron_ore": (rocks(), "8a6a5a"),
    "item_copper_ore": (rocks(), "c87a40"),
    "item_coal": (rocks(), "38383c"),
    "item_steel": (ingot(), "a8b4c0"),
    "item_electronics": (chip(), "3a9a50"),
    "item_tool": (wrench(), "b0b8c0"),
    "item_ammo": (bullet(), "d0a040"),
    "item_weapon": (rifle(), "5a5048"),
    "item_oil": (droplet(), "2a2a30"),
    "item_diesel": (droplet(), "d8b030"),
    "item_engine": (engine(), "8c463c"),
    # title menu buttons
    "menu_new_game": (triangle_play(), "50c060"),
    "menu_load_game": (folder(), "d0a840"),
    "menu_settings": (gear(), "a0a8b4"),
    "menu_quit": (cross(), "d04848"),
}


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, (shape, color) in ICONS.items():
        write_png(os.path.join(OUT, name + ".png"), paint(shape, rgb(color)))
    print("wrote", len(ICONS), "icons to", OUT)


if __name__ == "__main__":
    main()
