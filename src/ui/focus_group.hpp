#pragma once

#include <cstddef>
#include <vector>

namespace ui {

// A widget that can take keyboard focus and be triggered from the keyboard.
class Focusable {
public:
    virtual ~Focusable() = default;
    virtual void set_focused(bool focused) = 0;
    // Called when the user confirms the focused item (e.g. Enter).
    virtual void activate() = 0;
};

// Keyboard navigation over an ordered list of focusable widgets. Exactly one item is
// focused at a time and navigation wraps around. A scene maps its keys to these calls:
//   group.add(play_button); group.add(quit_button);
//   if (key_down) group.focus_next();
//   if (key_enter) group.activate_focused();
// Items are not owned; they must outlive the group.
class FocusGroup {
public:
    // Appends an item. The first item added receives focus.
    void add(Focusable& item);

    void focus_next();
    void focus_previous();
    // Jumps to an item (e.g. the one under the mouse). Items outside the group are ignored.
    void focus(const Focusable& item);
    void activate_focused();
    // Re-applies the focus to the current item, for widgets that can drop it on their own
    // (a text field blurs when the player clicks elsewhere).
    void refresh();

private:
    void focus_index(std::size_t index);

    std::vector<Focusable*> items_;
    std::size_t focused_ = 0;
};

// Adds a control to the group and makes hovering it move the focus there, the same
// mouse/keyboard sync ui::ButtonFactory gives buttons. Both must outlive each other's use.
template <typename Control>
void join_focus_group(FocusGroup& group, Control& control) {
    control.set_on_hover([&group, &control] { group.focus(control); });
    group.add(control);
}

} // namespace ui
