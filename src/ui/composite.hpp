#pragma once

#include <memory>

#include "ui/box.hpp"
#include "ui/widget.hpp"

namespace ui {

// A widget made of other widgets: a tree (a Box, a Grid, ...) it keeps between frames. Before measuring or
// laying out it calls sync() at most once per LayoutPass (every time outside one), where
// the subclass builds the tree with set_tree() the first time or when its shape changes
// (rows added or removed) and otherwise only updates texts, colours and states in place,
// so controls keep their hover and focus. Measuring, layout, drawing and the pointer and
// wheel all go to the tree.
class Composite : public Widget {
public:
    using Widget::Widget;

    Size preferred_size(const Canvas& canvas) const override;
    void layout(Rect bounds, const Canvas& canvas) override;
    void draw(Canvas& canvas) const override;
    void draw_overlay(Canvas& canvas) const override;
    bool blocks_pointer() const override;
    void handle_pointer(Point position, bool released) override;
    bool handle_scroll(Point position, float notches) override;

protected:
    // Brings the tree up to date with what the widget shows.
    virtual void sync(const Canvas& canvas) const = 0;
    // Replaces the tree; pointers into the old one are invalid afterwards.
    void set_tree(std::unique_ptr<Widget> tree) const { tree_ = std::move(tree); }

private:
    void ensure_synced(const Canvas& canvas) const;

    mutable std::unique_ptr<Widget> tree_;
    mutable unsigned synced_pass_ = 0;
};

} // namespace ui
