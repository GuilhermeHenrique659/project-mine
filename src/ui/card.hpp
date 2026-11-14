#pragma once

#include <memory>
#include <vector>

#include "ui/theme.hpp"
#include "ui/widget.hpp"

namespace ui {

// Container that stacks its children vertically, like a <div> with column flex.
// Children are stretched to the card's inner width.
class Card : public Widget {
public:
    Card(Style style, std::vector<std::unique_ptr<Widget>> children)
        : Widget(std::move(style)), children_(std::move(children)) {}

    Size preferred_size(const Canvas& canvas) const override;
    void layout(Rect bounds, const Canvas& canvas) override;
    void draw(Canvas& canvas) const override;
    void handle_pointer(Point position, bool released) override;

    const std::vector<std::unique_ptr<Widget>>& children() const { return children_; }

private:
    std::vector<std::unique_ptr<Widget>> children_;
};

class CardBuilder : public HandlerBuilder<CardBuilder> {
public:
    CardBuilder() : style_(theme::card_style()) {}

    CardBuilder& style(Style style) { style_ = std::move(style); return *this; }
    CardBuilder& child(std::unique_ptr<Widget> widget) {
        children_.push_back(std::move(widget));
        return *this;
    }
    std::unique_ptr<Card> build() {
        auto card = std::make_unique<Card>(style_, std::move(children_));
        apply_handlers(*card);
        return card;
    }

private:
    Style style_;
    std::vector<std::unique_ptr<Widget>> children_;
};

inline CardBuilder card() { return CardBuilder(); }

} // namespace ui
