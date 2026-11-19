#include "ui/scroll_box.hpp"

#include <algorithm>

#include "ui/theme.hpp"

namespace ui {

namespace {
// Where the pointer is sent while it is outside the view: nowhere near any widget, so the
// child's hover ends and nothing is clicked.
constexpr Point Nowhere{-1.0e9f, -1.0e9f};
} // namespace

Size ScrollBox::preferred_size(const Canvas& canvas) const {
    const Size content = child_.measure(canvas);
    const bool overflows = style_.max_height && content.height > *style_.max_height;
    const float scrollbar = overflows ? ScrollbarGap + ScrollbarWidth : 0.0f;
    return resolve_size({content.width + scrollbar, content.height});
}

void ScrollBox::layout(Rect bounds, const Canvas& canvas) {
    const LayoutPass pass;
    const Size wanted = measure(canvas);
    const Size content = child_.measure(canvas);
    // The view takes the room given (a growing or shrinking child of a Box), its preferred
    // size when given none; never taller than the content.
    const float height = std::min(bounds.height > 0.0f ? bounds.height : wanted.height,
                                  content.height);
    bounds_ = {bounds.x, bounds.y, std::max(bounds.width, wanted.width), height};
    content_height_ = content.height;
    max_offset_ = std::max(0.0f, content.height - height);
    offset_ = std::clamp(offset_, 0.0f, max_offset_);
    const float scrollbar = max_offset_ > 0.0f ? ScrollbarGap + ScrollbarWidth : 0.0f;
    child_.layout({bounds_.x, bounds_.y - offset_, bounds_.width - scrollbar, content.height},
                  canvas);
}

void ScrollBox::scroll(float notches) {
    offset_ = std::clamp(offset_ - notches * step_, 0.0f, max_offset_);
}

void ScrollBox::draw(Canvas& canvas) const {
    draw_box(canvas, style_.background, style_.border_color);
    canvas.push_clip(bounds_);
    child_.draw(canvas);
    child_.draw_overlay(canvas);
    canvas.pop_clip();
    draw_scrollbar(canvas);
}

void ScrollBox::draw_scrollbar(Canvas& canvas) const {
    if (max_offset_ <= 0.0f) {
        return;
    }
    const Rect track{bounds_.x + bounds_.width - ScrollbarWidth, bounds_.y, ScrollbarWidth,
                     bounds_.height};
    canvas.fill_rect(track, theme::Outline);
    // The thumb's share of the track is the share of the content shown.
    const float thumb_height = track.height * bounds_.height / content_height_;
    const float thumb_y = (track.height - thumb_height) * offset_ / max_offset_;
    canvas.fill_rect({track.x, track.y + thumb_y, track.width, thumb_height}, theme::Accent);
}

void ScrollBox::handle_pointer(Point position, bool released) {
    Widget::handle_pointer(position, released);
    if (!bounds_.contains(position)) {
        child_.handle_pointer(Nowhere, false);
        return;
    }
    child_.handle_pointer(position, released);
}

bool ScrollBox::handle_scroll(Point position, float notches) {
    if (!bounds_.contains(position)) {
        return false;
    }
    // An inner scroll view under the pointer goes first.
    if (child_.handle_scroll(position, notches)) {
        return true;
    }
    if (max_offset_ <= 0.0f) {
        return false;
    }
    scroll(notches);
    return true;
}

} // namespace ui
