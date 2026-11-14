#pragma once

#include <memory>

#include "event/event_bus.hpp"
#include "game/display_settings.hpp"
#include "scene/scene.hpp"
#include "ui/ui.hpp"

namespace scene::settings {

// The settings screen: a ui::Card with a resolution dropdown, a fullscreen checkbox and a
// Back button. Controls hold no rule; changing one only publishes a business event
// (DisplaySettingsChanged / BackToMenuRequested, see game/game_events.hpp).
class SettingsView : public scene::View {
public:
    SettingsView(event::EventBus& bus, game::DisplaySettings current);

    // Centers the card horizontally on a screen of the given width.
    void layout(const ui::Canvas& canvas, float screen_width) const;
    void draw() const override;

    // Pointer position in screen space; positions come from the last layout.
    void handle_pointer(ui::Point position, bool released);

    // Keyboard navigation: dropdown, fullscreen checkbox, Back button.
    ui::FocusGroup& focus_group() { return focus_group_; }

    const ui::Dropdown& resolution_dropdown() const { return *resolution_dropdown_; }
    const ui::Checkbox& fullscreen_checkbox() const { return *fullscreen_checkbox_; }
    const ui::Button& back_button() const { return *back_button_; }

private:
    // Handlers capture `this`, so the view must stay where it was constructed.
    SettingsView(const SettingsView&) = delete;
    SettingsView& operator=(const SettingsView&) = delete;

    game::DisplaySettings current_;
    std::unique_ptr<ui::Card> root_;
    // Non-owning, owned by root_.
    ui::Dropdown* resolution_dropdown_ = nullptr;
    ui::Checkbox* fullscreen_checkbox_ = nullptr;
    ui::Button* back_button_ = nullptr;
    ui::FocusGroup focus_group_;
};

} // namespace scene::settings
