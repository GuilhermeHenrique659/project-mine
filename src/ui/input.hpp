#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

#include "ui/focus_group.hpp"
#include "ui/theme.hpp"
#include "ui/widget.hpp"

namespace ui {

// Single-line text field. Receives characters only while focused.
class Input : public Widget, public Focusable {
public:
    using ChangeHandler = std::function<void(const std::string&)>;

    Input(std::string value, std::string placeholder, std::size_t max_length, Style style,
          ChangeHandler on_change)
        : Widget(std::move(style)),
          value_(std::move(value)),
          placeholder_(std::move(placeholder)),
          max_length_(max_length),
          on_change_(std::move(on_change)) {}

    Size preferred_size(const Canvas& canvas) const override;
    void draw(Canvas& canvas) const override;
    void handle_pointer(Point position, bool released) override;

    void type_char(char character);
    void erase_last();

    void set_focused(bool focused) override { focused_ = focused; }
    // Confirming a text field has no action of its own.
    void activate() override {}
    bool focused() const { return focused_; }
    const std::string& value() const { return value_; }

private:
    std::string value_;
    std::string placeholder_;
    std::size_t max_length_;
    ChangeHandler on_change_;
    bool focused_ = false;
};

class InputBuilder : public HandlerBuilder<InputBuilder> {
public:
    InputBuilder() : style_(theme::input_style()) {}

    InputBuilder& style(Style style) { style_ = std::move(style); return *this; }
    InputBuilder& placeholder(std::string text) { placeholder_ = std::move(text); return *this; }
    InputBuilder& value(std::string text) { value_ = std::move(text); return *this; }
    InputBuilder& max_length(std::size_t length) { max_length_ = length; return *this; }
    InputBuilder& on_change(Input::ChangeHandler handler) { on_change_ = std::move(handler); return *this; }
    std::unique_ptr<Input> build() {
        auto input = std::make_unique<Input>(std::move(value_), std::move(placeholder_),
                                             max_length_, style_, std::move(on_change_));
        apply_handlers(*input);
        return input;
    }

private:
    Style style_;
    std::string placeholder_;
    std::string value_;
    std::size_t max_length_ = 32;
    Input::ChangeHandler on_change_;
};

inline InputBuilder input() { return InputBuilder(); }

} // namespace ui
