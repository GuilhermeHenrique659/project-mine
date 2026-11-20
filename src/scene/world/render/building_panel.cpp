#include "scene/world/render/building_panel.hpp"

#include <string>

#include "scene/world/core/components/solid_inventory.hpp"
#include "scene/world/core/components/military.hpp"
#include "scene/world/render/building_details.hpp"
#include "scene/world/render/entity_name.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {
constexpr float Padding = 24.0f;
constexpr float TitleGap = 8.0f;
constexpr float SectionGap = 24.0f;
constexpr float BorderWidth = 2.0f;
constexpr int TitleFontSize = 20;
constexpr int DetailFontSize = 16;
constexpr float DetailGap = 4.0f;
// Rows of a building's inventory shown at once; a bigger one (a storage) scrolls.
constexpr std::size_t BuildingVisibleRows = 4;

ui::Rect slot_or_empty(const InventoryGrid& grid, std::size_t index) {
    const auto& slots = grid.slot_bounds();
    return index < slots.size() ? slots[index] : ui::Rect{};
}
} // namespace

BuildingPanel::BuildingPanel(WorldModel& model, std::shared_ptr<const input::Pointer> pointer)
    : model_(model),
      pointer_(std::move(pointer)),
      player_grid_(std::make_unique<InventoryGrid>(model.registry(), model.player_id())),
      close_button_(std::make_unique<ui::Button>("X", ui::theme::close_button_style(), true)),
      recruit_button_(std::make_unique<ui::Button>("Recruit", ui::theme::button_style(), true)) {
    close_button_->set_on_click([&model] { model.close_transfer(); });
    recruit_button_->set_on_click([&model] { model.request_recruit(model.transfer_target()); });
    player_grid_->set_on_slot_click([this](std::size_t slot) {
        move_stack(model_.player_id(), model_.transfer_target(), slot);
    });
}

bool BuildingPanel::recruit_visible() const {
    return model_.registry().has<::world::Recruiter>(model_.transfer_target());
}

bool BuildingPanel::visible() const { return model_.transfer_target() != 0 && !model_.is_paused(); }

InventoryGrid& BuildingPanel::building_grid() const {
    const core::EntityId target = model_.transfer_target();
    if (!building_grid_ || building_ != target) {
        // The tree points at the old grid: drop it before the grid goes away.
        root_.reset();
        building_grid_ = std::make_unique<InventoryGrid>(model_.registry(), target);
        building_grid_->set_on_slot_click(
            [this, target](std::size_t slot) { move_stack(target, model_.player_id(), slot); });
        building_ = target;
    }
    return *building_grid_;
}

ui::Rect BuildingPanel::recruit_bounds() const {
    return recruit_visible() ? recruit_button_->bounds() : ui::Rect{};
}

ui::Rect BuildingPanel::player_slot_bounds(std::size_t index) const {
    return slot_or_empty(*player_grid_, index);
}

ui::Rect BuildingPanel::building_slot_bounds(std::size_t index) const {
    return slot_or_empty(building_grid(), index);
}

void BuildingPanel::move_stack(core::EntityId from, core::EntityId to, std::size_t slot) const {
    const auto* inventory = model_.registry().get<::world::SolidInventory>(from);
    if (inventory == nullptr || slot >= inventory->capacity() || !inventory->slots()[slot]) {
        return;
    }
    const ::world::ItemStack stack = *inventory->slots()[slot];
    model_.request_transfer(from, to, stack.item, stack.count);
}

void BuildingPanel::update() {
    if (!root_) {
        return; // not laid out yet
    }
    const input::PointerPosition at = pointer_->position();
    const ui::Point position{at.x, at.y};
    // Slots, Recruit and the red X all react through the tree; the building's grid is in a
    // ScrollBox, which also takes the wheel and keeps clicks off its scrolled-out rows.
    root_->handle_pointer(position, pointer_->released());
    root_->handle_scroll(position, pointer_->scroll());
}

void BuildingPanel::layout(const ui::Canvas& canvas, ui::Size screen) const {
    sync_tree();
    root_->layout(ui::centered(root_->preferred_size(canvas), screen), canvas);
}

void BuildingPanel::sync_tree() const {
    const core::EntityId target = model_.transfer_target();
    building_grid(); // drops the tree when the target changed
    const auto lines = building_details(model_.registry(), target);
    if (!root_ || lines.size() != detail_labels_.size()) {
        root_ = build_tree(lines.size());
    }
    const core::Entity* building = model_.registry().find(target);
    building_title_->set_text(building != nullptr ? entity_name(building->label, building->id) : "");
    for (std::size_t i = 0; i < lines.size(); ++i) {
        detail_labels_[i]->set_text(lines[i]);
    }
    details_->set_visible(!lines.empty());
    // A building without slots shows no grid (and no gap above it).
    const auto* inventory = model_.registry().get<::world::SolidInventory>(target);
    building_view_->set_visible(inventory != nullptr && inventory->capacity() > 0);
    recruit_row_->set_visible(recruit_visible());
}

// Player section (title + X, inventory), then the building section (name, details,
// inventory) and, for a recruiter, the Recruit button.
std::unique_ptr<ui::Box> BuildingPanel::build_tree(std::size_t detail_lines) const {
    const auto label = [](int font_size) {
        return ui::label("").style(ui::theme::label_style().with_font_size(font_size)).build();
    };

    ui::BoxBuilder details = ui::column();
    details.gap(DetailGap);
    detail_labels_.clear();
    for (std::size_t i = 0; i < detail_lines; ++i) {
        auto line = label(DetailFontSize);
        detail_labels_.push_back(line.get());
        details.child(std::move(line));
    }
    auto details_box = details.build();
    details_ = details_box.get();

    auto title = label(TitleFontSize);
    building_title_ = title.get();
    auto player_title = label(TitleFontSize);
    player_title->set_text("Player");

    auto building_view = ui::scroll_box(*building_grid_)
                             .step(InventoryGrid::RowStep)
                             .max_height(BuildingVisibleRows * InventoryGrid::RowStep -
                                         InventoryGrid::SlotGap)
                             .build();
    building_view_ = building_view.get();

    // Wrapped in a row so the stretch of the panel does not widen the button.
    auto recruit_row = ui::row().child_ref(*recruit_button_).build();
    recruit_row_ = recruit_row.get();

    return ui::column()
        .padding(Padding)
        .gap(SectionGap)
        .align(ui::Align::Stretch)
        .background(ui::theme::Panel)
        .border(ui::theme::Outline, BorderWidth)
        .child(ui::column()
                   .gap(TitleGap)
                   .align(ui::Align::Stretch)
                   .child(ui::row()
                              .justify(ui::Justify::SpaceBetween)
                              .align(ui::Align::Center)
                              .child(std::move(player_title))
                              .child_ref(*close_button_)
                              .build())
                   .child_ref(*player_grid_)
                   .build())
        .child(ui::column()
                   .gap(TitleGap)
                   .child(std::move(title))
                   .child(std::move(details_box))
                   .child(std::move(building_view))
                   .build())
        .child(std::move(recruit_row))
        .build();
}

void BuildingPanel::draw(ui::Canvas& canvas, ui::Size screen) const {
    layout(canvas, screen);
    root_->draw(canvas);
}

} // namespace scene::world
