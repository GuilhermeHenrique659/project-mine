#pragma once

#include <memory>

#include "event/event_bus.hpp"
#include "scene/scene.hpp"
#include "scene/world/world_menu.hpp"
#include "scene/world/world_model.hpp"
#include "ui/ui.hpp"

namespace scene::world {

// The in-game screen: the isometric world, plus the pause menu (WorldMenu), which is
// laid out, drawn and clickable only while the model is paused. The model must outlive
// the view.
class WorldView : public scene::View {
public:
    WorldView(event::EventBus& bus, const WorldModel& model);

    // Lays out the pause menu, centred on a screen of the given size.
    void layout(const ui::Canvas& canvas, ui::Size screen) const;
    void draw() const override;

    // Draws every entity that has a Position (a cube with its label, plus the state for
    // characters) through the isometric projection, with world (0, 0) at the centre of a screen of the given size.
    void draw_entities(ui::Canvas& canvas, ui::Size screen) const;

    // Draws the pause menu; does nothing while the game is not paused.
    void draw_menu(ui::Canvas& canvas, ui::Size screen) const;

    // Pointer position in screen space, forwarded to the pause menu only while paused.
    void handle_pointer(ui::Point position, bool released);

    WorldMenu& menu() { return menu_; }
    const WorldMenu& menu() const { return menu_; }

private:
    WorldView(const WorldView&) = delete;
    WorldView& operator=(const WorldView&) = delete;

    const WorldModel& model_;
    WorldMenu menu_;
};

} // namespace scene::world
