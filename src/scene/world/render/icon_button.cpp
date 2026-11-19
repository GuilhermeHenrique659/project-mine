#include "scene/world/render/icon_button.hpp"

#include <optional>

#include "graphics/sprite.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {
// Space between the face's edge and the icon, and between the icon and the title.
constexpr float IconInset = 8.0f;
constexpr int FontSize = 20;
} // namespace

IconButton::IconButton(std::string icon, std::string title, ui::Size size)
    : ui::Widget(ui::theme::slot_style()), icon_(std::move(icon)), title_(std::move(title)),
      size_(size) {}

ui::Size IconButton::preferred_size(const ui::Canvas&) const { return size_; }

void IconButton::handle_pointer(ui::Point position, bool released) {
    if (!enabled_) {
        return;
    }
    ui::Widget::handle_pointer(position, released);
}

void IconButton::draw(ui::Canvas& canvas) const {
    const ui::Color face = !enabled_  ? ui::theme::ButtonShadow
                           : active_  ? ui::theme::GreenFace
                           : hovered() ? ui::theme::ButtonFaceHover
                                       : ui::theme::ButtonFace;
    ui::draw_raised(canvas, bounds_, face, style_);
    // A square icon as high as the face allows, on its left.
    const float side = bounds_.height - 2.0f * IconInset;
    const ui::Rect icon{bounds_.x + IconInset, bounds_.y + IconInset,
                        title_.empty() ? bounds_.width - 2.0f * IconInset : side, side};
    graphics::draw_sprite(canvas, {icon_, std::nullopt}, icon);
    if (title_.empty()) {
        return;
    }
    const float text_y = bounds_.y + (bounds_.height - FontSize) / 2.0f;
    canvas.draw_text(title_, {icon.x + icon.width + IconInset, text_y}, FontSize,
                     enabled_ ? ui::theme::Text : ui::theme::Muted);
}

} // namespace scene::world
