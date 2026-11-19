#include "ui/composite.hpp"

namespace ui {

void Composite::ensure_synced(const Canvas& canvas) const {
    if (LayoutPass::active() && synced_pass_ == LayoutPass::generation()) {
        return;
    }
    sync(canvas);
    synced_pass_ = LayoutPass::active() ? LayoutPass::generation() : 0;
}

Size Composite::preferred_size(const Canvas& canvas) const {
    ensure_synced(canvas);
    return tree_ ? tree_->measure(canvas) : Size{};
}

void Composite::layout(Rect bounds, const Canvas& canvas) {
    const LayoutPass pass;
    ensure_synced(canvas);
    bounds_ = bounds;
    if (tree_) {
        tree_->layout(bounds, canvas);
        bounds_ = tree_->bounds(); // at least its preferred size, like any container
    }
}

void Composite::draw(Canvas& canvas) const {
    if (tree_) {
        tree_->draw(canvas);
    }
}

void Composite::draw_overlay(Canvas& canvas) const {
    if (tree_) {
        tree_->draw_overlay(canvas);
    }
}

bool Composite::blocks_pointer() const { return tree_ && tree_->blocks_pointer(); }

void Composite::handle_pointer(Point position, bool released) {
    Widget::handle_pointer(position, released);
    if (tree_) {
        tree_->handle_pointer(position, released);
    }
}

bool Composite::handle_scroll(Point position, float notches) {
    return tree_ && tree_->handle_scroll(position, notches);
}

} // namespace ui
