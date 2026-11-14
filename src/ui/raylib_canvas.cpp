#include "ui/raylib_canvas.hpp"

#include <raylib.h>

#include <utility>

namespace ui {

namespace {
constexpr int FontAtlasSize = 64;
constexpr float SpacingRatio = 0.04f;
constexpr const char* FontCandidates[] = {
    "resource/fonts/ui.ttf",
    "C:/Windows/Fonts/segoeuib.ttf",
    "C:/Windows/Fonts/arialbd.ttf",
};

::Color to_raylib(Color color) { return {color.r, color.g, color.b, color.a}; }

// Solid (bold) UI font, loaded once on first use because it needs the window (GL context).
// Falls back to raylib's default font when no candidate file exists.
const Font& ui_font() {
    static const Font font = [] {
        for (const char* path : FontCandidates) {
            if (!FileExists(path)) {
                continue;
            }
            Font loaded = LoadFontEx(path, FontAtlasSize, nullptr, 0);
            if (loaded.texture.id == 0) {
                continue;
            }
            SetTextureFilter(loaded.texture, TEXTURE_FILTER_BILINEAR);
            return loaded;
        }
        return GetFontDefault();
    }();
    return font;
}

float spacing_for(int font_size) { return static_cast<float>(font_size) * SpacingRatio; }
} // namespace

void RaylibCanvas::fill_rect(Rect rect, Color color) {
    DrawRectangleRec({rect.x, rect.y, rect.width, rect.height}, to_raylib(color));
}

void RaylibCanvas::fill_triangle(Point a, Point b, Point c, Color color) {
    // Raylib only draws triangles whose corners run counter-clockwise on screen (y down).
    const float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cross > 0.0f) {
        std::swap(b, c);
    }
    DrawTriangle({a.x, a.y}, {b.x, b.y}, {c.x, c.y}, to_raylib(color));
}

void RaylibCanvas::stroke_rect(Rect rect, Color color, float thickness) {
    DrawRectangleLinesEx({rect.x, rect.y, rect.width, rect.height}, thickness, to_raylib(color));
}

void RaylibCanvas::draw_text(const std::string& text, Point position, int font_size, Color color) {
    DrawTextEx(ui_font(), text.c_str(), {position.x, position.y}, static_cast<float>(font_size),
               spacing_for(font_size), to_raylib(color));
}

void RaylibCanvas::draw_texture(const graphics::Texture& texture, Rect dest,
                                const TextureDraw& options) {
    const ::Texture2D raylib_texture{texture.id, texture.width, texture.height, texture.mipmaps,
                                     texture.format};
    const Rect source = options.source.value_or(
        Rect{0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height)});
    // A negative source size mirrors the texture.
    const ::Rectangle raylib_source{source.x, source.y,
                                    options.flip_x ? -source.width : source.width,
                                    options.flip_y ? -source.height : source.height};
    // Rotation turns around the origin: draw at the centre and offset the origin by half.
    const ::Vector2 origin{dest.width / 2.0f, dest.height / 2.0f};
    const ::Rectangle raylib_dest{dest.x + origin.x, dest.y + origin.y, dest.width, dest.height};
    DrawTexturePro(raylib_texture, raylib_source, raylib_dest, origin, options.rotation,
                   to_raylib(options.tint));
}

Size RaylibCanvas::measure_text(const std::string& text, int font_size) const {
    const Vector2 measured = MeasureTextEx(ui_font(), text.c_str(), static_cast<float>(font_size),
                                           spacing_for(font_size));
    return {measured.x, static_cast<float>(font_size)};
}

} // namespace ui
