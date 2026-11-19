#pragma once

#include <memory>
#include <vector>

#include "ui/widget.hpp"

namespace ui {

// How a child of a Box takes part in the main axis, like CSS flex-grow / flex-shrink:
// `grow` shares the free space by factor, `shrink` gives up the overflow by factor
// (never below the child's min size). Both default to 0: the child keeps its size.
struct Flex {
    float grow = 0.0f;
    float shrink = 0.0f;
};

// Main axis of a Box, like CSS flex-direction.
enum class Direction { Row, Column };

// Cross-axis placement of the children, like CSS align-items.
enum class Align { Start, Center, End, Stretch };

// Main-axis placement of the children, like CSS justify-content.
enum class Justify { Start, Center, End, SpaceBetween };

// Flexbox-like container: lays its children along one axis with the style's padding
// and gap, so callers describe the structure instead of computing positions.
//   ui::column().padding(24).gap(8).child(...).child(...).build()
// Children keep their preferred size on the main axis; on the cross axis they follow
// `align`. Fills its background and border, then draws its children.
class Box : public Widget {
public:
    struct Layout {
        Direction direction = Direction::Column;
        Align align = Align::Start;
        Justify justify = Justify::Start;
    };

    Box(Style style, Layout layout, std::vector<std::unique_ptr<Widget>> owned,
        std::vector<Widget*> children, std::vector<Flex> flex = {})
        : Widget(std::move(style)), layout_(layout), owned_(std::move(owned)),
          children_(std::move(children)), flex_(std::move(flex)) {
        flex_.resize(children_.size());
    }

    Size preferred_size(const Canvas& canvas) const override;
    void layout(Rect bounds, const Canvas& canvas) override;
    void draw(Canvas& canvas) const override;
    void handle_pointer(Point position, bool released) override;
    bool handle_scroll(Point position, float notches) override;

    // Every child in order, shown or hidden.
    const std::vector<Widget*>& children() const { return children_; }

private:
    // Indexes of the visible children, in order.
    std::vector<std::size_t> shown() const;

    Layout layout_;
    // Children the box owns; `children_` lists every child in order, owned or not.
    std::vector<std::unique_ptr<Widget>> owned_;
    std::vector<Widget*> children_;
    std::vector<Flex> flex_; // one per entry of children_
};

class BoxBuilder : public HandlerBuilder<BoxBuilder> {
public:
    explicit BoxBuilder(Direction direction) { layout_.direction = direction; }

    // Replaces the whole style: call it first, before padding(), gap() and the others.
    BoxBuilder& style(Style style) { style_ = std::move(style); return *this; }
    BoxBuilder& padding(float all) { style_.padding = Insets::all(all); return *this; }
    BoxBuilder& padding(float vertical, float horizontal) {
        style_.padding = Insets::symmetric(vertical, horizontal);
        return *this;
    }
    BoxBuilder& gap(float value) { style_.gap = value; return *this; }
    BoxBuilder& width(float value) { style_.width = value; return *this; }
    BoxBuilder& height(float value) { style_.height = value; return *this; }
    BoxBuilder& min_width(float value) { style_.min_width = value; return *this; }
    BoxBuilder& max_width(float value) { style_.max_width = value; return *this; }
    BoxBuilder& min_height(float value) { style_.min_height = value; return *this; }
    BoxBuilder& max_height(float value) { style_.max_height = value; return *this; }
    BoxBuilder& background(Color value) { style_.background = value; return *this; }
    BoxBuilder& border(Color color, float thickness) {
        style_.border_color = color;
        style_.border_width = thickness;
        return *this;
    }
    BoxBuilder& align(Align value) { layout_.align = value; return *this; }
    BoxBuilder& justify(Justify value) { layout_.justify = value; return *this; }

    // A child the box owns.
    BoxBuilder& child(std::unique_ptr<Widget> widget, Flex flex = {}) {
        children_.push_back(widget.get());
        flex_.push_back(flex);
        owned_.push_back(std::move(widget));
        return *this;
    }
    // A child owned by the caller, which must outlive the box (a panel's buttons, grids).
    BoxBuilder& child_ref(Widget& widget, Flex flex = {}) {
        children_.push_back(&widget);
        flex_.push_back(flex);
        return *this;
    }

    // Single use: the children move into the box.
    std::unique_ptr<Box> build() {
        auto box = std::make_unique<Box>(style_, layout_, std::move(owned_), std::move(children_),
                                         std::move(flex_));
        apply_handlers(*box);
        return box;
    }

private:
    Style style_;
    Box::Layout layout_;
    std::vector<std::unique_ptr<Widget>> owned_;
    std::vector<Widget*> children_;
    std::vector<Flex> flex_;
};

inline BoxBuilder row() { return BoxBuilder(Direction::Row); }
inline BoxBuilder column() { return BoxBuilder(Direction::Column); }

// Where a rect of `size` goes to sit in the middle of `area`.
inline Rect centered(Size size, Size area) {
    return {(area.width - size.width) / 2.0f, (area.height - size.height) / 2.0f, size.width,
            size.height};
}

} // namespace ui
