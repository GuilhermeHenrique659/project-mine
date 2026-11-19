#include "scene/world/render/player_menu.hpp"

#include "scene/world/render/energy_page.hpp"
#include "scene/world/render/logistics_page.hpp"
#include "scene/world/render/workers_page.hpp"
#include "ui/box.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {
constexpr float Padding = 32.0f;
// Space between the tab bar and the page under it.
constexpr float PageGap = 24.0f;
constexpr float BorderWidth = 2.0f;
constexpr float SeparatorHeight = 2.0f;
// Pixels per wheel notch over a page: about one line of the Workers or Logistics list.
constexpr float PageScrollStep = 40.0f;
} // namespace

PlayerMenu::PlayerMenu(WorldModel& model, std::shared_ptr<const input::Pointer> pointer)
    : model_(model), pointer_(std::move(pointer)),
      tab_bar_(std::make_unique<ui::TabBar>(
          std::vector<ui::Tab>{{"Inventory", true},
                               {"Energy", true},
                               {"Workers", true},
                               {"Statistics", false},
                               {"Logistics", true}},
          ui::theme::tab_style(), ui::theme::active_tab_style(),
          ui::theme::disabled_tab_style())),
      close_button_(std::make_unique<ui::Button>("X", ui::theme::close_button_style(), true)) {
    close_button_->set_on_click([&model] { model.close_player_menu(); });
    pages_.push_back(std::make_unique<InventoryGrid>(model.registry(), model.player_id()));
    pages_.push_back(std::make_unique<EnergyPage>(model.registry()));
    pages_.push_back(std::make_unique<WorkersPage>(
        model.registry(),
        [&model](core::EntityId building, int count) { model.request_workers(building, count); }));
    pages_.push_back(nullptr); // Statistics: not available yet
    pages_.push_back(std::make_unique<LogisticsPage>(
        model.registry(), model.routes(),
        [&model](::world::RouteKey route, ::world::HaulMode mode, int count) {
            model.request_haulers(route, mode, count);
        },
        [&model](::world::RouteKey route, ::world::HaulMode mode) {
            model.request_haul_mode(route, mode);
        }));
    // Each page scrolls in its own view, so it keeps its place when the tab changes.
    for (const auto& page : pages_) {
        page_views_.push_back(
            page ? ui::scroll_box(*page).step(PageScrollStep).build() : nullptr);
    }
}

bool PlayerMenu::visible() const { return model_.is_player_menu_open() && !model_.is_paused(); }

const ui::ScrollBox* PlayerMenu::page_view() const {
    return page_views_[tab_bar_->active()].get();
}

void PlayerMenu::update() {
    if (!root_) {
        return; // not laid out yet
    }
    const input::PointerPosition at = pointer_->position();
    const ui::Point position{at.x, at.y};
    // Tabs, the page's buttons and the red X react through the tree; the page's view takes
    // the wheel.
    root_->handle_pointer(position, pointer_->released());
    root_->handle_scroll(position, pointer_->scroll());
}

// The panel: the tab bar and the X over an accent line, then the active page filling the
// rest (Flex: it grows into the room left and shrinks to it, scrolling what does not fit).
// Rebuilt only when the screen size changes; every part is owned by the menu.
std::unique_ptr<ui::Box> PlayerMenu::build_tree(ui::Size panel) const {
    ui::BoxBuilder root = ui::column();
    root.width(panel.width)
        .height(panel.height)
        .padding(Padding)
        .gap(PageGap)
        .align(ui::Align::Stretch)
        .background(ui::theme::Panel)
        .border(ui::theme::Outline, BorderWidth)
        .child(ui::column()
                   .align(ui::Align::Stretch)
                   .child(ui::row()
                              .justify(ui::Justify::SpaceBetween)
                              .child_ref(*tab_bar_)
                              .child_ref(*close_button_)
                              .build())
                   .child(ui::row().height(SeparatorHeight).background(ui::theme::Accent).build())
                   .build());
    for (const auto& view : page_views_) {
        if (view) {
            root.child_ref(*view, {1.0f, 1.0f});
        }
    }
    return root.build();
}

void PlayerMenu::layout(const ui::Canvas& canvas, ui::Size screen) const {
    const float width = screen.width * ScreenCoverage;
    const float height = screen.height * ScreenCoverage;
    panel_ = {(screen.width - width) / 2.0f, (screen.height - height) / 2.0f, width, height};
    if (!root_ || screen.width != screen_.width || screen.height != screen_.height) {
        root_ = build_tree({width, height});
        screen_ = screen;
    }
    for (std::size_t i = 0; i < page_views_.size(); ++i) {
        if (page_views_[i]) {
            page_views_[i]->set_visible(i == tab_bar_->active());
        }
    }
    root_->layout(panel_, canvas);
}

void PlayerMenu::draw(ui::Canvas& canvas, ui::Size screen) const {
    layout(canvas, screen);
    root_->draw(canvas);
}

} // namespace scene::world
