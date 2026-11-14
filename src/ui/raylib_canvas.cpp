#include "ui/raylib_canvas.hpp"

#include <raylib.h>

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

void RaylibCanvas::stroke_rect(Rect rect, Color color, float thickness) {
    DrawRectangleLinesEx({rect.x, rect.y, rect.width, rect.height}, thickness, to_raylib(color));
}

void RaylibCanvas::draw_text(const std::string& text, Point position, int font_size, Color color) {
    DrawTextEx(ui_font(), text.c_str(), {position.x, position.y}, static_cast<float>(font_size),
               spacing_for(font_size), to_raylib(color));
}

Size RaylibCanvas::measure_text(const std::string& text, int font_size) const {
    const Vector2 measured = MeasureTextEx(ui_font(), text.c_str(), static_cast<float>(font_size),
                                           spacing_for(font_size));
    return {measured.x, static_cast<float>(font_size)};
}

} // namespace ui
