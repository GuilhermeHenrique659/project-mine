#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "event/event_bus.hpp"
#include "scene/panel_view.hpp"
#include "ui/ui.hpp"

namespace scene::menu {

// The main menu screen: a ui::Card with one button per action. Buttons hold no rule,
// clicking one only publishes a business event (see game/game_events.hpp).
// Also owns which button has keyboard focus.
class MenuView : public scene::PanelView {
public:
    explicit MenuView(event::EventBus& bus);

    const std::vector<ui::Button*>& buttons() const { return buttons_; }

private:
    std::vector<ui::Button*> buttons_; // non-owning, owned by the tree
};

} // namespace scene::menu
