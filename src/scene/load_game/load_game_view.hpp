#pragma once

#include <memory>
#include <vector>

#include "event/event_bus.hpp"
#include "game/save_game.hpp"
#include "scene/scene.hpp"
#include "ui/ui.hpp"

namespace scene::load_game {

// The Load Game screen: one button per saved game, in the order received (the caller
// passes them newest first). Picking one publishes LoadSaveRequested with its name; Back
// publishes BackToMenuRequested (see game/game_events.hpp).
class LoadGameView : public scene::View {
public:
    LoadGameView(event::EventBus& bus, const std::vector<game::SaveGame>& saves);

    // Centers the card horizontally on a screen of the given width.
    void layout(const ui::Canvas& canvas, float screen_width) const;
    void draw() const override;

    // Pointer position in screen space; positions come from the last layout.
    void handle_pointer(ui::Point position, bool released);

    // Keyboard navigation: every save, then Back.
    ui::FocusGroup& focus_group() { return focus_group_; }

    const std::vector<ui::Button*>& save_buttons() const { return save_buttons_; }
    const ui::Button& back_button() const { return *back_button_; }
    // Shown instead of the list when there is nothing to load; null otherwise.
    const ui::Label* empty_message() const { return empty_label_; }

private:
    LoadGameView(const LoadGameView&) = delete;
    LoadGameView& operator=(const LoadGameView&) = delete;

    std::unique_ptr<ui::Card> root_;
    // Non-owning, owned by root_.
    std::vector<ui::Button*> save_buttons_;
    ui::Button* back_button_ = nullptr;
    ui::Label* empty_label_ = nullptr;
    ui::FocusGroup focus_group_;
};

} // namespace scene::load_game
