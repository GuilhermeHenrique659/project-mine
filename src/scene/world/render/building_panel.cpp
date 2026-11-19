#include "scene/world/render/building_panel.hpp"

#include <string>

#include "scene/world/core/components/inventory.hpp"
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
        building_grid_ = std::make_unique<InventoryGrid>(model_.registry(), target, BuildingVisibleRows);
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
    const auto* inventory = model_.registry().get<::world::Inventory>(from);
    if (inventory == nullptr || slot >= inventory->capacity() || !inventory->slots()[slot]) {
        return;
    }
    const ::world::ItemStack stack = *inventory->slots()[slot];
    model_.request_transfer(from, to, stack.item, stack.count);
}

void BuildingPanel::update() {
    const input::PointerPosition at = pointer_->position();
    const ui::Point position{at.x, at.y};
    const bool released = pointer_->released();
    player_grid_->handle_pointer(position, released);
    building_grid().handle_pointer(position, released);
    if (building_grid().bounds().contains(position)) {
        building_grid().scroll(pointer_->scroll());
    }
    if (!released) {
        return;
    }
    const core::EntityId player = model_.player_id();
    const core::EntityId building = model_.transfer_target();
    const auto& player_slots = player_grid_->slot_bounds();
    for (std::size_t i = 0; i < player_slots.size(); ++i) {
        if (player_slots[i].contains(position)) {
            move_stack(player, building, i);
            return;
        }
    }
    const auto& building_slots = building_grid().slot_bounds();
    for (std::size_t i = 0; i < building_slots.size(); ++i) {
        if (building_slots[i].contains(position)) {
            move_stack(building, player, i);
            return;
        }
    }
    if (recruit_visible() && recruit_button_->bounds().contains(position)) {
        recruit_button_->handle_pointer(position, released);
        return;
    }
    // Last: a click on it closes the panel.
    close_button_->handle_pointer(position, released);
}

void BuildingPanel::layout(const ui::Canvas& canvas, ui::Size screen) const {
    root_ = build_tree();
    root_->layout(ui::centered(root_->preferred_size(canvas), screen), canvas);
}

// Player section (title + X, inventory), then the building section (name, details,
// inventory) and, for a recruiter, the Recruit button.
std::unique_ptr<ui::Box> BuildingPanel::build_tree() const {
    const auto title = [](std::string text) {
        return ui::label(std::move(text))
            .style(ui::theme::label_style().with_font_size(TitleFontSize))
            .build();
    };
    const core::Entity* building = model_.registry().find(model_.transfer_target());

    ui::BoxBuilder details = ui::column();
    details.gap(DetailGap);
    const auto lines = building_details(model_.registry(), model_.transfer_target());
    for (const std::string& line : lines) {
        details.child(ui::label(line)
                          .style(ui::theme::label_style().with_font_size(DetailFontSize))
                          .build());
    }

    ui::BoxBuilder building_section = ui::column();
    building_section.gap(TitleGap).child(
        title(building != nullptr ? entity_name(building->label, building->id) : ""));
    if (!lines.empty()) {
        building_section.child(details.build());
    }
    // A building without slots gets no grid (and no gap above it).
    const auto* inventory = model_.registry().get<::world::Inventory>(model_.transfer_target());
    if (inventory != nullptr && inventory->capacity() > 0) {
        building_section.child_ref(building_grid());
    }

    ui::BoxBuilder panel = ui::column();
    panel.padding(Padding)
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
                              .child(title("Player"))
                              .child_ref(*close_button_)
                              .build())
                   .child_ref(*player_grid_)
                   .build())
        .child(building_section.build());
    if (recruit_visible()) {
        // Wrapped in a row so the stretch of the panel does not widen the button.
        panel.child(ui::row().child_ref(*recruit_button_).build());
    }
    return panel.build();
}

void BuildingPanel::draw(ui::Canvas& canvas, ui::Size screen) const {
    layout(canvas, screen);
    root_->draw(canvas);
}

} // namespace scene::world
