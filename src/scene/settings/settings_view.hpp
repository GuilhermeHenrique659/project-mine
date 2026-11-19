#pragma once

#include <memory>

#include "event/event_bus.hpp"
#include "game/display_settings.hpp"
#include "scene/panel_view.hpp"
#include "ui/ui.hpp"

namespace scene::settings {

// The settings screen: a ui::Card with a resolution dropdown, a fullscreen checkbox and a
// Back button. Controls hold no rule; changing one only publishes a business event
// (DisplaySettingsChanged / BackToMenuRequested, see game/game_events.hpp).
class SettingsView : public scene::PanelView {
public:
    SettingsView(event::EventBus& bus, game::DisplaySettings current);

    const ui::Dropdown& resolution_dropdown() const { return *resolution_dropdown_; }
    const ui::Checkbox& fullscreen_checkbox() const { return *fullscreen_checkbox_; }
    const ui::Button& back_button() const { return *back_button_; }

private:
    // Handlers capture `this`, so the view must stay where it was constructed.
    SettingsView(const SettingsView&) = delete;
    SettingsView& operator=(const SettingsView&) = delete;

    game::DisplaySettings current_;
    // Non-owning, owned by the tree.
    ui::Dropdown* resolution_dropdown_ = nullptr;
    ui::Checkbox* fullscreen_checkbox_ = nullptr;
    ui::Button* back_button_ = nullptr;
};

} // namespace scene::settings
