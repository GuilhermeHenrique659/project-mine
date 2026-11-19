#pragma once

#include <memory>
#include <utility>

#include "ui/widget.hpp"

namespace ui {

// A view over a taller child, scrolled vertically (like CSS overflow-y: auto). The child is
// laid out at its full preferred height and moved up by offset(); drawing is clipped to the
// view and the pointer only reaches the child inside the view, so scrolled-out parts can
// neither be seen nor clicked. The view is as high as its max height (or as the room its
// parent gives it, e.g. with Flex{grow, shrink}), never taller than the content. While the
// content overflows, a scrollbar is drawn on the right, in room taken from the child's width.
// The wheel scrolls `step` pixels per notch (a row or a line, so whole rows stay aligned).
class ScrollBox : public Widget {
public:
    static constexpr float ScrollbarWidth = 10.0f;
    static constexpr float ScrollbarGap = 8.0f;

    ScrollBox(Style style, std::unique_ptr<Widget> owned, Widget& child, float step)
        : Widget(std::move(style)), owned_(std::move(owned)), child_(child), step_(step) {}

    Size preferred_size(const Canvas& canvas) const override;
    void layout(Rect bounds, const Canvas& canvas) override;
    void draw(Canvas& canvas) const override;
    void handle_pointer(Point position, bool released) override;
    bool handle_scroll(Point position, float notches) override;

    // Moves the content by wheel notches (positive = up), `step` pixels each, within
    // [0, max_offset()] of the last layout.
    void scroll(float notches);
    float offset() const { return offset_; }
    // How far the content can scroll, from the last layout (0 when it fits).
    float max_offset() const { return max_offset_; }

private:
    void draw_scrollbar(Canvas& canvas) const;

    std::unique_ptr<Widget> owned_; // null when the caller owns the child
    Widget& child_;
    float step_;
    float offset_ = 0.0f;
    float max_offset_ = 0.0f;
    float content_height_ = 0.0f;
};

class ScrollBoxBuilder : public HandlerBuilder<ScrollBoxBuilder> {
public:
    // A child owned by the caller, which must outlive the view.
    explicit ScrollBoxBuilder(Widget& child) : child_(&child) {}
    explicit ScrollBoxBuilder(std::unique_ptr<Widget> child)
        : owned_(std::move(child)), child_(owned_.get()) {}

    // Pixels per wheel notch.
    ScrollBoxBuilder& step(float value) { step_ = value; return *this; }
    ScrollBoxBuilder& max_height(float value) { style_.max_height = value; return *this; }

    // Single use: an owned child moves into the view.
    std::unique_ptr<ScrollBox> build() {
        auto box = std::make_unique<ScrollBox>(style_, std::move(owned_), *child_, step_);
        apply_handlers(*box);
        return box;
    }

private:
    Style style_;
    std::unique_ptr<Widget> owned_;
    Widget* child_;
    float step_ = 20.0f;
};

inline ScrollBoxBuilder scroll_box(Widget& child) { return ScrollBoxBuilder(child); }
inline ScrollBoxBuilder scroll_box(std::unique_ptr<Widget> child) {
    return ScrollBoxBuilder(std::move(child));
}

} // namespace ui
