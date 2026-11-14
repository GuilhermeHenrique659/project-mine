#pragma once

#include <memory>
#include <string>

#include "ui/button.hpp"
#include "ui/focus_group.hpp"

namespace ui {

// Creates buttons already wired to a FocusGroup, so every screen gets the same behavior:
// the button joins the group (keyboard navigation) and hovering it moves the focus to it
// (mouse and keyboard stay in sync). The group must outlive the factory and the buttons.
//   ui::ButtonFactory buttons(focus_group_);
//   auto play = buttons.create("Play", [&bus] { bus.publish(game::NewGameRequested{}); });
class ButtonFactory {
public:
    explicit ButtonFactory(FocusGroup& focus_group) : focus_group_(focus_group) {}

    std::unique_ptr<Button> create(std::string label, Widget::EventHandler on_click) const;

private:
    FocusGroup& focus_group_;
};

} // namespace ui
