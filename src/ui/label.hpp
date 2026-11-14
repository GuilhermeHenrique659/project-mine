#pragma once

#include <memory>
#include <string>

#include "ui/theme.hpp"
#include "ui/widget.hpp"

namespace ui {

// Static text.
class Label : public Widget {
public:
    Label(std::string text, Style style) : Widget(std::move(style)), text_(std::move(text)) {}

    Size preferred_size(const Canvas& canvas) const override;
    void draw(Canvas& canvas) const override;

    void set_text(std::string text) { text_ = std::move(text); }
    const std::string& text() const { return text_; }

private:
    std::string text_;
};

class LabelBuilder : public HandlerBuilder<LabelBuilder> {
public:
    explicit LabelBuilder(std::string text) : text_(std::move(text)), style_(theme::label_style()) {}

    LabelBuilder& style(Style style) { style_ = std::move(style); return *this; }
    std::unique_ptr<Label> build() {
        auto label = std::make_unique<Label>(std::move(text_), style_);
        apply_handlers(*label);
        return label;
    }

private:
    std::string text_;
    Style style_;
};

inline LabelBuilder label(std::string text) { return LabelBuilder(std::move(text)); }

} // namespace ui
