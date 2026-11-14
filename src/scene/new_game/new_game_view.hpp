#pragma once

#include <memory>
#include <string>

#include "event/event_bus.hpp"
#include "scene/scene.hpp"
#include "ui/ui.hpp"

namespace scene::new_game {

// The New Game screen: asks for the save name. Create (or Enter in the field) publishes
// CreateGameRequested with the normalized name; a name that cannot be used only shows a
// message. Back publishes BackToMenuRequested (see game/game_events.hpp).
class NewGameView : public scene::View {
public:
    explicit NewGameView(event::EventBus& bus);

    // Centers the card horizontally on a screen of the given width.
    void layout(const ui::Canvas& canvas, float screen_width) const;
    void draw() const override;

    // Pointer position in screen space; positions come from the last layout.
    void handle_pointer(ui::Point position, bool released);

    // Text typed by the player; reaches the name field only while it has the focus.
    void type_char(char character);
    void erase_last();

    // Keyboard navigation: name field, Create, Back.
    ui::FocusGroup& focus_group() { return focus_group_; }

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
    std::unique_ptr<ui::Card> root_;
    // Non-owning, owned by root_.
    ui::Input* name_input_ = nullptr;
    ui::Label* error_label_ = nullptr;
    ui::Button* create_button_ = nullptr;
    ui::Button* back_button_ = nullptr;
    ui::FocusGroup focus_group_;
};

} // namespace scene::new_game
