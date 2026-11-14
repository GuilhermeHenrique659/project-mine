#pragma once

#include <memory>

#include "event/event_bus.hpp"
#include "game/save_game.hpp"
#include "ui/ui.hpp"

namespace scene::world {

// The pause menu of the world: the save's name and creation date, a Save button
// (publishes SaveGameRequested with the given save) and a Main Menu button (publishes
// BackToMenuRequested). It knows nothing about when it is shown; WorldView decides.
class WorldMenu {
public:
    WorldMenu(event::EventBus& bus, const game::SaveGame& save);

    // Centres the panel on a screen of the given size.
    void layout(const ui::Canvas& canvas, ui::Size screen) const;
    void draw(ui::Canvas& canvas, ui::Size screen) const;

    // Pointer position in screen space; positions come from the last layout.
    void handle_pointer(ui::Point position, bool released);

    // Keyboard navigation: Save, Main Menu.
    ui::FocusGroup& focus_group() { return focus_group_; }

    const ui::Label& name_label() const { return *name_label_; }
    const ui::Button& save_button() const { return *save_button_; }
    const ui::Button& menu_button() const { return *menu_button_; }

private:
    WorldMenu(const WorldMenu&) = delete;
    WorldMenu& operator=(const WorldMenu&) = delete;

    std::unique_ptr<ui::Card> root_;
    // Non-owning, owned by root_.
    ui::Label* name_label_ = nullptr;
    ui::Button* save_button_ = nullptr;
    ui::Button* menu_button_ = nullptr;
    ui::FocusGroup focus_group_;
};

} // namespace scene::world
