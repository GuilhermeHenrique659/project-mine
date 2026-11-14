#pragma once

#include <optional>

#include "ui/geometry.hpp"

namespace ui {

enum class TextAlign { Left, Center };

// Visual description of a widget, the equivalent of a CSS rule.
// Every with_* returns a modified copy, so styles chain and can be reused:
//   Style{}.with_background(Panel).with_padding(Insets::all(8))
struct Style {
    Color background{0, 0, 0, 0};
    Color foreground{255, 255, 255, 255};
    Color border_color{0, 0, 0, 0};
    float border_width = 0.0f;
    Insets padding;
    float gap = 0.0f; // space between children of a container
    std::optional<float> width;
    std::optional<float> height;
    int font_size = 20;
    TextAlign text_align = TextAlign::Left;
    std::optional<Color> hover_background;
    std::optional<Color> hover_foreground;
    std::optional<Color> focus_border_color;

    [[nodiscard]] Style with_background(Color value) const { Style s = *this; s.background = value; return s; }
    [[nodiscard]] Style with_foreground(Color value) const { Style s = *this; s.foreground = value; return s; }
    [[nodiscard]] Style with_border(Color color, float thickness) const {
        Style s = *this;
        s.border_color = color;
        s.border_width = thickness;
        return s;
    }
    [[nodiscard]] Style with_padding(Insets value) const { Style s = *this; s.padding = value; return s; }
    [[nodiscard]] Style with_gap(float value) const { Style s = *this; s.gap = value; return s; }
    [[nodiscard]] Style with_width(float value) const { Style s = *this; s.width = value; return s; }
    [[nodiscard]] Style with_height(float value) const { Style s = *this; s.height = value; return s; }
    [[nodiscard]] Style with_font_size(int value) const { Style s = *this; s.font_size = value; return s; }
    [[nodiscard]] Style with_text_align(TextAlign value) const { Style s = *this; s.text_align = value; return s; }
    [[nodiscard]] Style with_hover_background(Color value) const { Style s = *this; s.hover_background = value; return s; }
    [[nodiscard]] Style with_hover_foreground(Color value) const { Style s = *this; s.hover_foreground = value; return s; }
    [[nodiscard]] Style with_focus_border_color(Color value) const { Style s = *this; s.focus_border_color = value; return s; }
};

} // namespace ui
