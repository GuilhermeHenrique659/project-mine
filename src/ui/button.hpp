#pragma once

#include <memory>
#include <string>

#include "ui/focus_group.hpp"
#include "ui/theme.hpp"
#include "ui/widget.hpp"

namespace ui {

// Clickable text. Uses the hover style while the pointer is over it or while focused
// (keyboard navigation). A disabled button ignores the pointer entirely.
class Button : public Widget, public Focusable {
public:
    Button(std::string text, Style style, bool enabled)
        : Widget(std::move(style)), text_(std::move(text)), enabled_(enabled) {}

    Size preferred_size(const Canvas& canvas) const override;
    void draw(Canvas& canvas) const override;
    void handle_pointer(Point position, bool released) override;
    void click() override;

    void set_focused(bool focused) override { focused_ = focused; }
    void activate() override { click(); }
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool enabled() const { return enabled_; }
    const std::string& text() const { return text_; }
    void set_text(std::string text) { text_ = std::move(text); }

private:
    std::string text_;
    bool enabled_;
    bool focused_ = false;
};

class ButtonBuilder : public HandlerBuilder<ButtonBuilder> {
public:
    explicit ButtonBuilder(std::string text) : text_(std::move(text)), style_(theme::button_style()) {}

    ButtonBuilder& style(Style style) { style_ = std::move(style); return *this; }
    ButtonBuilder& enabled(bool enabled) { enabled_ = enabled; return *this; }
    std::unique_ptr<Button> build() {
        auto button = std::make_unique<Button>(std::move(text_), style_, enabled_);
        apply_handlers(*button);
        return button;
    }

private:
    std::string text_;
    Style style_;
    bool enabled_ = true;
};

inline ButtonBuilder button(std::string text) { return ButtonBuilder(std::move(text)); }

} // namespace ui
