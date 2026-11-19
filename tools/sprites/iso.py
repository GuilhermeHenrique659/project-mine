"""A tiny isometric rasterizer for building sprites: boxes and lines in world space.

World axes match the game (src/scene/world/core/isometric.hpp): +x goes right-down on
screen, +y goes left-down, one tile is 64x32 px. z is height in screen pixels.
Draw back to front (painter's order): later calls cover earlier ones.
"""
import math

from pixel import TILE_HEIGHT, TILE_WIDTH, shade


class IsoCanvas:
    def __init__(self, image, origin):
        """`origin`: the screen pixel of world (0, 0, 0)."""
        self.image = image
        self.ox, self.oy = origin
        self.height, self.width = len(image), len(image[0])

    def to_screen(self, x, y, z=0.0):
        return (self.ox + (x - y) * TILE_WIDTH / 2, self.oy + (x + y) * TILE_HEIGHT / 2 - z)

    def put(self, sx, sy, color):
        sx, sy = int(sx), int(sy)
        if 0 <= sx < self.width and 0 <= sy < self.height:
            self.image[sy][sx] = (*color[:3], 255)

    def box(self, x0, y0, x1, y1, z0, z1, top, left, right):
        """A box over [x0, x1] x [y0, y1] x [z0, z1]. Each face is a color or a function
        (u, v) -> color with u, v in [0, 1] across the face (v = 0 at the bottom / back)."""
        hw, hh = TILE_WIDTH / 2, TILE_HEIGHT / 2
        corners = [self.to_screen(x, y, z) for x in (x0, x1) for y in (y0, y1) for z in (z0, z1)]
        min_x = max(0, int(min(c[0] for c in corners)) - 1)
        max_x = min(self.width - 1, int(max(c[0] for c in corners)) + 1)
        min_y = max(0, int(min(c[1] for c in corners)) - 1)
        max_y = min(self.height - 1, int(max(c[1] for c in corners)) + 1)
        paint = lambda face, u, v: face(u, v) if callable(face) else face
        for sy in range(min_y, max_y + 1):
            for sx in range(min_x, max_x + 1):
                px, py = sx + 0.5 - self.ox, sy + 0.5 - self.oy
                # Top face: z = z1.
                a, b = px / hw, (py + z1) / hh  # a = x - y, b = x + y
                x, y = (a + b) / 2, (b - a) / 2
                if x0 <= x <= x1 and y0 <= y <= y1:
                    self.put(sx, sy, paint(top, (x - x0) / (x1 - x0), (y - y0) / (y1 - y0)))
                    continue
                # Left (front-left) face: y = y1.
                x = px / hw + y1
                z = (x + y1) * hh - py
                if x0 <= x <= x1 and z0 <= z <= z1:
                    self.put(sx, sy, paint(left, (x - x0) / (x1 - x0), (z - z0) / (z1 - z0)))
                    continue
                # Right (front-right) face: x = x1.
                y = x1 - px / hw
                z = (x1 + y) * hh - py
                if y0 <= y <= y1 and z0 <= z <= z1:
                    self.put(sx, sy, paint(right, (y1 - y) / (y1 - y0), (z - z0) / (z1 - z0)))

    def line(self, a, b, color, thickness=1):
        """A line between two world points (x, y, z)."""
        (ax, ay), (bx, by) = self.to_screen(*a), self.to_screen(*b)
        steps = int(max(abs(bx - ax), abs(by - ay))) + 1
        for i in range(steps + 1):
            t = i / steps
            sx, sy = ax + (bx - ax) * t, ay + (by - ay) * t
            for dx in range(thickness):
                for dy in range(thickness):
                    self.put(sx + dx - thickness // 2, sy + dy - thickness // 2, color)

    def dot(self, point, color, radius=1.0):
        sx, sy = self.to_screen(*point)
        r = math.ceil(radius)
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                if dx * dx + dy * dy <= radius * radius:
                    self.put(sx + dx, sy + dy, color)


def lit_faces(color):
    """Top / left / right shades of one material, lit from the top left."""
    return shade(color, 1.25), shade(color, 1.0), shade(color, 0.7)
