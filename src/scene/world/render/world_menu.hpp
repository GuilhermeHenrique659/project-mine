#pragma once

#include <memory>

#include "event/event_bus.hpp"
#include "input/pointer.hpp"
#include "scene/world/render/renderer.hpp"
#include "scene/world/world_model.hpp"
#include "ui/ui.hpp"

namespace scene::world {

// The pause menu of the world: the save's name and creation date, a Save button
// (publishes SaveGameRequested with the given save) and a Main Menu button (publishes
// BackToMenuRequested). It is a Renderer that is visible only while the model is paused,
// and it reads the mouse itself in update() through the injected pointer, using the
// bounds of the last draw(). The bus, the model and the pointer's owner (shared) are
// injected; the bus and the model must outlive the menu.
class WorldMenu : public Renderer {
public:
    WorldMenu(event::EventBus& bus, const WorldModel& model,
              std::shared_ptr<const input::Pointer> pointer);

    bool visible() const override;
    void update() override;
    void draw(ui::Canvas& canvas, ui::Size screen) const override;

    // Centres the panel on a screen of the given size (draw() does it before painting).
    void layout(const ui::Canvas& canvas, ui::Size screen) const;

    // Keyboard navigation: Save, Main Menu.
    ui::FocusGroup& focus_group() { return focus_group_; }

    const ui::Label& name_label() const { return *name_label_; }
    const ui::Button& save_button() const { return *save_button_; }
    const ui::Button& menu_button() const { return *menu_button_; }

private:
    WorldMenu(const WorldMenu&) = delete;
    WorldMenu& operator=(const WorldMenu&) = delete;

    const WorldModel& model_;
    std::shared_ptr<const input::Pointer> pointer_;
    std::unique_ptr<ui::Card> root_;
    // Non-owning, owned by root_.
    ui::Label* name_label_ = nullptr;
    ui::Button* save_button_ = nullptr;
    ui::Button* menu_button_ = nullptr;
    ui::FocusGroup focus_group_;
};

} // namespace scene::world
