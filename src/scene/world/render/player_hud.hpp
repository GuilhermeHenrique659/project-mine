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
// Clicking a construction selects it and closes the list; constructions the model refuses
// (WorldModel::build_status(): not implemented yet, no headquarters yet, unique and already
// built) are drawn disabled and cannot be selected. With a selection, a click on the world
// (outside the bar and the list) asks the model to build it on the tile under the pointer
// and clears the selection (a refused click keeps it and turns the preview a stronger red
// until the pointer leaves that tile); until then a translucent cube previews it on the tile under
// the pointer, green where it can be built and red where it cannot. It reads the mouse itself in
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

    // Where the selected construction would go: the tile under the pointer and whether it
    // can be built there. Empty without a selection or while the pointer is over the HUD.
    struct Preview {
        ::world::TileCoord tile;
        bool valid = false;
        // A click was refused on this tile (drawn a stronger red until the pointer moves on).
        bool rejected = false;
    };
    std::optional<Preview> preview() const;

    // The last construction picked from a list, null until one is.
    const ::world::Construction* selected() const { return selected_; }

private:
    bool is_disabled(const ::world::Construction& construction) const;
    // A translucent cube of the selection on the tile under the pointer (see preview()).
    void draw_preview(ui::Canvas& canvas) const;

    WorldModel& model_;
    std::shared_ptr<const input::Pointer> pointer_;
    std::optional<std::size_t> open_tab_;
    const ::world::Construction* selected_ = nullptr;
    // The tile of the last refused click, shown a stronger red while the pointer is on it.
    std::optional<::world::TileCoord> rejected_tile_;
    // Layout state, recomputed by every layout(); mutable because draw() is const.
    mutable ui::Size screen_;
    mutable ui::Rect bar_;
    mutable std::vector<ui::Rect> tabs_;
    mutable ui::Rect list_;
    mutable std::vector<ui::Rect> items_;
};

} // namespace scene::world
