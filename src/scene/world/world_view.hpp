#pragma once

#include <memory>

#include "event/event_bus.hpp"
#include "scene/scene.hpp"
#include "scene/world/world_model.hpp"
#include "ui/ui.hpp"

namespace scene::world {

// The in-game screen. For now a panel with the save's name and creation date, a Save
// button (publishes SaveGameRequested with the model's state) and a Main Menu button
// (publishes BackToMenuRequested). The model must outlive the view.
class WorldView : public scene::View {
public:
    WorldView(event::EventBus& bus, const WorldModel& model);

    // Centers the panel horizontally on a screen of the given width.
    void layout(const ui::Canvas& canvas, float screen_width) const;
    void draw() const override;

    // Draws every entity that has a Position (a cube with its label, plus the state for
    // characters) through the isometric projection, with world (0, 0) at the centre of a screen of the given size.
    void draw_entities(ui::Canvas& canvas, ui::Size screen) const;

    // Pointer position in screen space; positions come from the last layout.
    void handle_pointer(ui::Point position, bool released);

    // Keyboard navigation: Save, Main Menu.
    ui::FocusGroup& focus_group() { return focus_group_; }

    const ui::Label& name_label() const { return *name_label_; }
    const ui::Button& save_button() const { return *save_button_; }
    const ui::Button& menu_button() const { return *menu_button_; }

private:
    WorldView(const WorldView&) = delete;
    WorldView& operator=(const WorldView&) = delete;

    const WorldModel& model_;
    std::unique_ptr<ui::Card> root_;
    // Non-owning, owned by root_.
    ui::Label* name_label_ = nullptr;
    ui::Button* save_button_ = nullptr;
    ui::Button* menu_button_ = nullptr;
    ui::FocusGroup focus_group_;
};

} // namespace scene::world
