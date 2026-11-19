#pragma once

#include <functional>
#include <optional>
#include <string>
#include <utility>

#include "ui/canvas.hpp"
#include "ui/geometry.hpp"
#include "ui/style.hpp"

namespace ui {

// One layout pass: while one is open (the outermost container's layout() opens it),
// Widget::measure() computes each widget's preferred size once and reuses it, so a deep
// tree is measured in linear time. Every new outermost pass measures again, so a text or
// content change shows up on the next layout. Single-threaded, like the rest of the UI.
class LayoutPass {
public:
    LayoutPass() {
        if (depth_++ == 0) {
            ++generation_;
        }
    }
    ~LayoutPass() { --depth_; }
    LayoutPass(const LayoutPass&) = delete;
    LayoutPass& operator=(const LayoutPass&) = delete;

    static bool active() { return depth_ > 0; }
    static unsigned generation() { return generation_; }

private:
    inline static int depth_ = 0;
    inline static unsigned generation_ = 0;
};

// Base of every UI component. A widget is described by a Style, measured with
// preferred_size(), positioned with layout() and rendered with draw().
// Behaviour is attached with event handlers: on_click, on_hover, on_leave.
class Widget {
public:
    using EventHandler = std::function<void()>;

    explicit Widget(Style style) : style_(std::move(style)) {}
    virtual ~Widget() = default;

    // Size the widget wants: the style's width/height, or content plus padding.
    virtual Size preferred_size(const Canvas& canvas) const = 0;
    virtual void layout(Rect bounds, const Canvas& canvas);
    // preferred_size(), computed once per LayoutPass (every time outside one). Containers
    // measure their children with it.
    Size measure(const Canvas& canvas) const;
    virtual void draw(Canvas& canvas) const = 0;
    // Drawn by the parent after all siblings, so popups (e.g. an open dropdown list)
    // appear on top of them.
    virtual void draw_overlay(Canvas&) const {}
    // True while the widget has a popup open: the parent then forwards pointer events
    // only to it, so clicks on the popup never reach the widgets underneath.
    virtual bool blocks_pointer() const { return false; }
    // Pointer position in screen space; `released` is true on the frame the button is released.
    // Fires on_hover when the pointer enters, on_leave when it exits, on_click on release inside.
    virtual void handle_pointer(Point position, bool released);

    // Mouse wheel notches (positive = up) at `position`. Returns true when a widget took
    // them (a scroll view under the pointer), so the caller stops passing them on.
    virtual bool handle_scroll(Point, float) { return false; }

    // Triggers the click handler without a pointer, e.g. from keyboard activation.
    virtual void click();

    void set_on_click(EventHandler handler) { on_click_ = std::move(handler); }
    void set_on_hover(EventHandler handler) { on_hover_ = std::move(handler); }
    void set_on_leave(EventHandler handler) { on_leave_ = std::move(handler); }

    // A hidden widget takes no room in a Box, is not drawn and ignores the pointer.
    void set_visible(bool visible) { visible_ = visible; }
    // A fixed width, or none to size to the content (a column of labels lined up).
    void set_width(std::optional<float> width) { style_.width = width; }
    // Replaces the whole look (a tab becoming active).
    void set_style(Style style) { style_ = std::move(style); }
    bool visible() const { return visible_; }

    bool hovered() const { return hovered_; }
    const Rect& bounds() const { return bounds_; }
    const Style& style() const { return style_; }

protected:
    Size resolve_size(Size content) const;
    void draw_box(Canvas& canvas, Color background, Color border) const;
    void draw_bevel(Canvas& canvas) const { draw_bevel(canvas, bounds_); }
    void draw_bevel(Canvas& canvas, Rect area) const;
    void draw_text_in_box(Canvas& canvas, const std::string& text, Color color) const;

    Style style_;
    Rect bounds_;

private:
    EventHandler on_click_;
    EventHandler on_hover_;
    EventHandler on_leave_;
    bool hovered_ = false;
    bool visible_ = true;
    mutable Size measured_;
    mutable unsigned measured_pass_ = 0; // LayoutPass generation of measured_, 0 = none
};

// Shared by every builder so any component can receive event handlers:
//   ui::label("x").on_hover([]{ ... }).on_click([]{ ... }).build()
template <typename Derived>
class HandlerBuilder {
public:
    Derived& on_click(Widget::EventHandler handler) {
        on_click_ = std::move(handler);
        return self();
    }
    Derived& on_hover(Widget::EventHandler handler) {
        on_hover_ = std::move(handler);
        return self();
    }
    Derived& on_leave(Widget::EventHandler handler) {
        on_leave_ = std::move(handler);
        return self();
    }

protected:
    void apply_handlers(Widget& widget) {
        widget.set_on_click(std::move(on_click_));
        widget.set_on_hover(std::move(on_hover_));
        widget.set_on_leave(std::move(on_leave_));
    }

private:
    Derived& self() { return static_cast<Derived&>(*this); }

    Widget::EventHandler on_click_;
    Widget::EventHandler on_hover_;
    Widget::EventHandler on_leave_;
};

// Paints `area` the way a button is painted: filled with `face`, outlined with the
// style's border and with its bevel (dark band on the bottom and right edges) inside it.
// For widgets that draw several button-like parts (tabs, inventory slots).
void draw_raised(Canvas& canvas, Rect area, Color face, const Style& style);

} // namespace ui
