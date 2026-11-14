#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "event/event_bus.hpp"
#include "scene/scene.hpp"
#include "ui/ui.hpp"

namespace scene::menu {

// The main menu screen: a ui::Card with one button per action. Buttons hold no rule,
// clicking one only publishes a business event (see game/game_events.hpp).
// Also owns which button has keyboard focus.
class MenuView : public scene::View {
public:
    explicit MenuView(event::EventBus& bus);

    // Centers the card horizontally on a screen of the given width.
    void layout(const ui::Canvas& canvas, float screen_width) const;
    void draw() const override;

    // Pointer position in screen space; positions come from the last layout.
    void handle_pointer(ui::Point position, bool released);

    // Keyboard navigation, shared with any other scene through ui::FocusGroup.
    ui::FocusGroup& focus_group() { return focus_group_; }

    const std::vector<ui::Button*>& buttons() const { return buttons_; }

private:
    std::unique_ptr<ui::Card> root_;
    std::vector<ui::Button*> buttons_; // non-owning, owned by root_
    ui::FocusGroup focus_group_;
};

} // namespace scene::menu
