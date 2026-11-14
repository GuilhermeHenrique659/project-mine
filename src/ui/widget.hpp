#pragma once

#include <functional>
#include <optional>
#include <string>
#include <utility>

#include "ui/canvas.hpp"
#include "ui/geometry.hpp"
#include "ui/style.hpp"

namespace ui {

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
    virtual void draw(Canvas& canvas) const = 0;
    // Pointer position in screen space; `released` is true on the frame the button is released.
    // Fires on_hover when the pointer enters, on_leave when it exits, on_click on release inside.
    virtual void handle_pointer(Point position, bool released);

    // Triggers the click handler without a pointer, e.g. from keyboard activation.
    virtual void click();

    void set_on_click(EventHandler handler) { on_click_ = std::move(handler); }
    void set_on_hover(EventHandler handler) { on_hover_ = std::move(handler); }
    void set_on_leave(EventHandler handler) { on_leave_ = std::move(handler); }

    bool hovered() const { return hovered_; }
    const Rect& bounds() const { return bounds_; }
    const Style& style() const { return style_; }

protected:
    Size resolve_size(Size content) const;
    void draw_box(Canvas& canvas, Color background, Color border) const;
    void draw_text_in_box(Canvas& canvas, const std::string& text, Color color) const;

    Style style_;
    Rect bounds_;

private:
    EventHandler on_click_;
    EventHandler on_hover_;
    EventHandler on_leave_;
    bool hovered_ = false;
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

} // namespace ui
