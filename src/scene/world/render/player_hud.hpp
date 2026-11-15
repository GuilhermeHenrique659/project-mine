#pragma once

#include <cstddef>
#include <memory>
#include <optional>

#include "input/pointer.hpp"
#include "scene/world/core/construction/construction.hpp"
#include "scene/world/render/renderer.hpp"
#include "scene/world/world_model.hpp"

namespace scene::world {

// The player's heads-up display. Today only the build bar: a row of icon tabs at the
// bottom centre of the screen, one per construction category (see
// ::world::construction_catalog()), shown while the model's build bar is open (B) and the
// game is not paused. Clicking a tab opens a drop-up above it listing that category's
// constructions (icon + name); clicking it again closes it, clicking another tab switches.
// Clicking a construction selects it and closes the list. It reads the mouse itself in
// update() through the injected pointer, using the bounds of the last layout. The model
// must outlive the HUD.
class PlayerHud : public Renderer {
public:
    // Part of the screen width the bar covers (it grows past it if the tabs need more).
    static constexpr float BarWidthCoverage = 0.6f;

    PlayerHud(WorldModel& model, std::shared_ptr<const input::Pointer> pointer);

    bool visible() const override;
    void update() override;
    void draw(ui::Canvas& canvas, ui::Size screen) const override;

    // Places the bar, its tabs and the open list on a screen of the given size (draw()
    // does it before painting).
    void layout(const ui::Canvas& canvas, ui::Size screen) const;

    const ui::Rect& bar_bounds() const { return bar_; }
    std::size_t tab_count() const;
    ui::Rect tab_bounds(std::size_t index) const;

    // The tab whose list is open, if any.
    std::optional<std::size_t> open_tab() const { return open_tab_; }
    // Entries of the open list (0 when closed) and where each one is.
    std::size_t item_count() const;
    ui::Rect item_bounds(std::size_t index) const;
    // The panel of the open list (empty when closed).
    const ui::Rect& list_bounds() const { return list_; }

    // The last construction picked from a list, null until one is.
    const ::world::Construction* selected() const { return selected_; }

private:
    WorldModel& model_;
    std::shared_ptr<const input::Pointer> pointer_;
    std::optional<std::size_t> open_tab_;
    const ::world::Construction* selected_ = nullptr;
    // Layout state, recomputed by every layout(); mutable because draw() is const.
    mutable ui::Rect bar_;
    mutable std::vector<ui::Rect> tabs_;
    mutable ui::Rect list_;
    mutable std::vector<ui::Rect> items_;
};

} // namespace scene::world
