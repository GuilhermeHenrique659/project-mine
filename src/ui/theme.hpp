#pragma once

#include "ui/geometry.hpp"
#include "ui/style.hpp"

// Default look of the components. Builders start from these styles.
namespace ui::theme {

constexpr Color Panel{40, 44, 52, 255};
constexpr Color Field{20, 22, 26, 255};
constexpr Color Accent{230, 160, 40, 255};
constexpr Color Text{245, 245, 245, 255};
constexpr Color Muted{130, 130, 130, 255};
constexpr Color Dark{0, 0, 0, 255};

inline Style label_style() { return Style{}.with_foreground(Text).with_font_size(20); }

inline Style button_style() {
    return Style{}
        .with_background(Panel)
        .with_foreground(Text)
        .with_border(Muted, 1.0f)
        .with_padding(Insets::symmetric(12.0f, 24.0f))
        .with_font_size(28)
        .with_text_align(TextAlign::Center)
        .with_hover_background(Accent)
        .with_hover_foreground(Dark);
}

inline Style input_style() {
    return Style{}
        .with_background(Field)
        .with_foreground(Text)
        .with_border(Muted, 1.0f)
        .with_focus_border_color(Accent)
        .with_padding(Insets::symmetric(8.0f, 12.0f))
        .with_font_size(20)
        .with_width(280.0f);
}

inline Style card_style() {
    return Style{}
        .with_background(Panel)
        .with_border(Muted, 1.0f)
        .with_padding(Insets::all(24.0f))
        .with_gap(16.0f);
}

} // namespace ui::theme
