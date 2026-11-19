#include "ui/canvas.hpp"

#include <algorithm>

namespace ui {

void Canvas::push_clip(Rect rect) {
    if (!clips_.empty()) {
        const Rect& outer = clips_.back();
        const float left = std::max(rect.x, outer.x);
        const float top = std::max(rect.y, outer.y);
        const float right = std::min(rect.x + rect.width, outer.x + outer.width);
        const float bottom = std::min(rect.y + rect.height, outer.y + outer.height);
        rect = {left, top, std::max(0.0f, right - left), std::max(0.0f, bottom - top)};
    }
    clips_.push_back(rect);
    apply_clip(clips_.back());
}

void Canvas::pop_clip() {
    if (clips_.empty()) {
        return;
    }
    clips_.pop_back();
    apply_clip(clips_.empty() ? std::nullopt : std::optional<Rect>{clips_.back()});
}

} // namespace ui
