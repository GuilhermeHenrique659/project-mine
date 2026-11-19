#pragma once

#include <memory>
#include <string>

#include "event/event_bus.hpp"
#include "scene/panel_view.hpp"
#include "ui/ui.hpp"

namespace scene::new_game {

// The New Game screen: asks for the save name. Create (or Enter in the field) publishes
// CreateGameRequested with the normalized name; a name that cannot be used only shows a
// message. Back publishes BackToMenuRequested (see game/game_events.hpp).
class NewGameView : public scene::PanelView {
public:
    explicit NewGameView(event::EventBus& bus);

    // Also keeps the focus group in sync with the name field.
    void handle_pointer(ui::Point position, bool released) override;

    // Text typed by the player; reaches the name field only while it has the focus.
    void type_char(char character);
    void erase_last();

    const ui::Input& name_input() const { return *name_input_; }
    // Empty while there is nothing wrong with the typed name.
    const std::string& error_message() const { return error_label_->text(); }
    const ui::Button& create_button() const { return *create_button_; }
    const ui::Button& back_button() const { return *back_button_; }

private:
    // Handlers capture `this`, so the view must stay where it was constructed.
    NewGameView(const NewGameView&) = delete;
    NewGameView& operator=(const NewGameView&) = delete;

    void submit();

    event::EventBus& bus_;
    // Non-owning, owned by the tree.
    ui::Input* name_input_ = nullptr;
    ui::Label* error_label_ = nullptr;
    ui::Button* create_button_ = nullptr;
    ui::Button* back_button_ = nullptr;
};

} // namespace scene::new_game
