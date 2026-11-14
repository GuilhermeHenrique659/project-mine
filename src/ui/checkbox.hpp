#pragma once

#include <functional>
#include <memory>
#include <string>

#include "ui/focus_group.hpp"
#include "ui/theme.hpp"
#include "ui/widget.hpp"

namespace ui {

// Labelled on/off box. Clicking it (or activating it from the keyboard) toggles the value
// and notifies on_change with the new value.
class Checkbox : public Widget, public Focusable {
public:
    using ChangeHandler = std::function<void(bool)>;

    Checkbox(std::string label, bool checked, Style style, ChangeHandler on_change)
        : Widget(std::move(style)),
          label_(std::move(label)),
          checked_(checked),
          on_change_(std::move(on_change)) {}

    Size preferred_size(const Canvas& canvas) const override;
    void draw(Canvas& canvas) const override;
    void handle_pointer(Point position, bool released) override;
    void click() override;

    void set_focused(bool focused) override { focused_ = focused; }
    void activate() override { click(); }
    bool checked() const { return checked_; }
    const std::string& label() const { return label_; }

private:
    void toggle();

    std::string label_;
    bool checked_;
    ChangeHandler on_change_;
    bool focused_ = false;
};

class CheckboxBuilder : public HandlerBuilder<CheckboxBuilder> {
public:
    explicit CheckboxBuilder(std::string label)
        : label_(std::move(label)), style_(theme::checkbox_style()) {}

    CheckboxBuilder& style(Style style) { style_ = std::move(style); return *this; }
    CheckboxBuilder& checked(bool checked) { checked_ = checked; return *this; }
    CheckboxBuilder& on_change(Checkbox::ChangeHandler handler) { on_change_ = std::move(handler); return *this; }
    std::unique_ptr<Checkbox> build() {
        auto checkbox = std::make_unique<Checkbox>(std::move(label_), checked_, style_,
                                                   std::move(on_change_));
        apply_handlers(*checkbox);
        return checkbox;
    }

private:
    std::string label_;
    Style style_;
    bool checked_ = false;
    Checkbox::ChangeHandler on_change_;
};

inline CheckboxBuilder checkbox(std::string label) { return CheckboxBuilder(std::move(label)); }

} // namespace ui
