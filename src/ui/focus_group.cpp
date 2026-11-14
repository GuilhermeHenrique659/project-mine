#include "ui/focus_group.hpp"

#include <algorithm>

namespace ui {

void FocusGroup::add(Focusable& item) {
    items_.push_back(&item);
    if (items_.size() == 1) {
        focus_index(0);
        return;
    }
    item.set_focused(false);
}

void FocusGroup::focus_next() {
    if (items_.empty()) {
        return;
    }
    focus_index((focused_ + 1) % items_.size());
}

void FocusGroup::focus_previous() {
    if (items_.empty()) {
        return;
    }
    focus_index((focused_ + items_.size() - 1) % items_.size());
}

void FocusGroup::focus(const Focusable& item) {
    const auto found = std::find(items_.begin(), items_.end(), &item);
    if (found == items_.end()) {
        return;
    }
    focus_index(static_cast<std::size_t>(found - items_.begin()));
}

void FocusGroup::activate_focused() {
    if (items_.empty()) {
        return;
    }
    items_[focused_]->activate();
}

void FocusGroup::refresh() {
    if (items_.empty()) {
        return;
    }
    focus_index(focused_);
}

void FocusGroup::focus_index(std::size_t index) {
    focused_ = index;
    for (std::size_t i = 0; i < items_.size(); ++i) {
        items_[i]->set_focused(i == focused_);
    }
}

} // namespace ui
