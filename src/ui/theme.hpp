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

// Solid machine-style look: flat faces with a dark band on the bottom and left edges.
constexpr Color ButtonFace{112, 116, 124, 255};
constexpr Color ButtonFaceHover{82, 85, 92, 255};
constexpr Color ButtonShadow{44, 46, 52, 255};
constexpr Color GreenFace{58, 140, 68, 255};
constexpr Color GreenFaceHover{42, 108, 52, 255};
constexpr Color GreenShadow{22, 62, 30, 255};
constexpr Color RedFace{168, 58, 54, 255};
constexpr Color RedFaceHover{132, 42, 40, 255};
constexpr Color RedShadow{76, 22, 22, 255};
constexpr Color Outline{14, 15, 18, 255};

inline Style label_style() { return Style{}.with_foreground(Text).with_font_size(20); }

inline Style button_style() {
    return Style{}
        .with_background(ButtonFace)
        .with_foreground(Text)
        .with_border(Outline, 2.0f)
        .with_bevel(ButtonShadow, 5.0f)
        .with_padding(Insets::symmetric(20.0f, 24.0f))
        .with_font_size(32)
        .with_text_align(TextAlign::Center)
        .with_hover_background(ButtonFaceHover);
}

// Same machine-button pattern as button_style(), in another color.
inline Style colored_button_style(Color face, Color hover_face, Color shadow) {
    return button_style().with_background(face).with_hover_background(hover_face).with_bevel(shadow, 5.0f);
}

inline Style start_button_style() { return colored_button_style(GreenFace, GreenFaceHover, GreenShadow); }
inline Style quit_button_style() { return colored_button_style(RedFace, RedFaceHover, RedShadow); }

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

// Checkbox and dropdown share the gray machine-button face: flat face, darker on hover,
// dark band on the bottom and right edges, near-black outline.
inline Style control_style() {
    return Style{}
        .with_background(ButtonFace)
        .with_foreground(Text)
        .with_border(Outline, 2.0f)
        .with_bevel(ButtonShadow, 5.0f)
        .with_padding(Insets::symmetric(14.0f, 16.0f))
        .with_font_size(24)
        .with_hover_background(ButtonFaceHover);
}

inline Style checkbox_style() { return control_style(); }

inline Style dropdown_style() { return control_style(); }

// Tabs of a ui::TabBar: the gray machine-button face (darker on hover), with the padding
// around each title and the gap between tabs.
inline Style tab_style() {
    return control_style()
        .with_padding(Insets::symmetric(10.0f, 22.0f))
        .with_gap(6.0f)
        .with_font_size(22);
}

// The active tab: the green face of the Start button.
inline Style active_tab_style() {
    return tab_style().with_background(GreenFace).with_bevel(GreenShadow, 5.0f);
}

// A disabled tab: a dark face with a muted title.
inline Style disabled_tab_style() {
    return tab_style().with_background(ButtonShadow).with_foreground(Muted).with_bevel(Outline, 5.0f);
}

// An inventory slot: the gray machine-button face, darker on hover.
inline Style slot_style() {
    return control_style().with_padding({}).with_bevel(ButtonShadow, 4.0f);
}

// The small red X that closes a panel.
inline Style close_button_style() {
    return quit_button_style().with_font_size(24).with_padding(Insets::symmetric(6.0f, 14.0f));
}

inline Style card_style() {
    return Style{}
        .with_background(Panel)
        .with_border(Outline, 2.0f)
        .with_bevel(Dark, 6.0f)
        .with_padding(Insets::all(44.0f))
        .with_gap(26.0f);
}

} // namespace ui::theme
