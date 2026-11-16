#include "scene/world/render/player_menu.hpp"

#include "scene/world/render/energy_page.hpp"
#include "scene/world/render/logistics_page.hpp"
#include "scene/world/render/workers_page.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {
constexpr float Padding = 32.0f;
// Space between the tab bar and the page under it.
constexpr float PageGap = 24.0f;
constexpr float BorderWidth = 2.0f;
constexpr float SeparatorHeight = 2.0f;
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
    auto logistics = std::make_unique<LogisticsPage>(
        model.registry(), model.routes(),
        [&model](::world::RouteKey route, ::world::HaulMode mode, int count) {
            model.request_haulers(route, mode, count);
        },
        [&model](::world::RouteKey route, ::world::HaulMode mode) {
            model.request_haul_mode(route, mode);
        });
    logistics_ = logistics.get();
    pages_.push_back(std::move(logistics));
}

bool PlayerMenu::visible() const { return model_.is_player_menu_open() && !model_.is_paused(); }

void PlayerMenu::update() {
    const input::PointerPosition at = pointer_->position();
    const ui::Point position{at.x, at.y};
    const bool released = pointer_->released();
    tab_bar_->handle_pointer(position, released);
    if (const auto& page = pages_[tab_bar_->active()]) {
        page->handle_pointer(position, released);
        // The mouse wheel over the Logistics page scrolls its routes.
        if (page.get() == logistics_ && page->bounds().contains(position)) {
            logistics_->scroll(pointer_->scroll());
        }
    }
    // Last: a click on it closes the menu.
    close_button_->handle_pointer(position, released);
}

void PlayerMenu::layout(const ui::Canvas& canvas, ui::Size screen) const {
    const float width = screen.width * ScreenCoverage;
    const float height = screen.height * ScreenCoverage;
    panel_ = {(screen.width - width) / 2.0f, (screen.height - height) / 2.0f, width, height};

    tab_bar_->layout({panel_.x + Padding, panel_.y + Padding, 0.0f, 0.0f}, canvas);
    const ui::Size close = close_button_->preferred_size(canvas);
    close_button_->layout({panel_.x + width - Padding - close.width, panel_.y + Padding,
                           close.width, close.height},
                          canvas);
    const ui::Rect& tabs = tab_bar_->bounds();
    const float page_y = tabs.y + tabs.height + PageGap;
    for (const auto& page : pages_) {
        if (!page) {
            continue;
        }
        page->layout({panel_.x + Padding, page_y, width - 2.0f * Padding,
                      panel_.y + height - Padding - page_y},
                     canvas);
    }
}

void PlayerMenu::draw(ui::Canvas& canvas, ui::Size screen) const {
    layout(canvas, screen);
    canvas.fill_rect(panel_, ui::theme::Panel);
    canvas.stroke_rect(panel_, ui::theme::Outline, BorderWidth);
    tab_bar_->draw(canvas);
    close_button_->draw(canvas);
    const ui::Rect& tabs = tab_bar_->bounds();
    canvas.fill_rect({panel_.x + Padding, tabs.y + tabs.height, panel_.width - 2.0f * Padding,
                      SeparatorHeight},
                     ui::theme::Accent);
    const auto& page = pages_[tab_bar_->active()];
    if (page) {
        page->draw(canvas);
    }
}

} // namespace scene::world
