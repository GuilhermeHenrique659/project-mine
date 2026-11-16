#include "scene/world/render/logistics_page.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "scene/world/core/logistics_report.hpp"
#include "text/parse.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {

constexpr int FontSize = 20;
constexpr float RowGap = 12.0f;
constexpr float SectionGap = 16.0f;
constexpr float ButtonGap = 8.0f;
constexpr float Indent = 24.0f;
constexpr const char* EmptyText = "No logistic routes: build a storage";

// "Storage (0, 0)".
std::string end_text(const ::world::LogisticEnd& end) {
    return end.name + " (" + text::format(end.tile.x) + ", " + text::format(end.tile.y) + ")";
}

std::string row_text(const ::world::LogisticEntry& entry) {
    const std::string length =
        entry.length > 0 ? text::format(entry.length) + " tiles" : std::string{"unreachable"};
    return end_text(entry.from) + " -> " + end_text(entry.to) + " - " + length + " - " +
           text::format(entry.haulers) + " workers";
}

std::string count_text(std::size_t count) { return "Logistic routes: " + text::format(count); }

std::pair<core::EntityId, core::EntityId> key_of(::world::RouteKey route) {
    return {route.from, route.to};
}

::world::HaulMode other(::world::HaulMode mode) {
    return mode == ::world::HaulMode::Take ? ::world::HaulMode::Put : ::world::HaulMode::Take;
}

} // namespace

LogisticsPage::LogisticsPage(const core::Registry& registry, const ::world::RouteNetwork& network,
                             AssignHandler on_assign, ModeHandler on_mode)
    : ui::Widget(ui::theme::label_style()), registry_(registry), network_(network),
      on_assign_(std::move(on_assign)), on_mode_(std::move(on_mode)) {}

::world::HaulMode LogisticsPage::chosen_mode(::world::RouteKey route) const {
    const auto chosen = modes_.find(key_of(route));
    if (chosen != modes_.end()) {
        return chosen->second;
    }
    for (const auto& entry : ::world::logistics_report(registry_, network_)) {
        if (entry.key == route && entry.mode) {
            return *entry.mode;
        }
    }
    return ::world::HaulMode::Take;
}

ui::Size LogisticsPage::preferred_size(const ui::Canvas& canvas) const {
    const auto report = ::world::logistics_report(registry_, network_);
    ui::Size size = canvas.measure_text(count_text(report.size()), FontSize);
    size.height += SectionGap;
    for (const auto& entry : report) {
        const ui::Size text = canvas.measure_text(row_text(entry), FontSize);
        size.width = std::max(size.width, Indent + text.width);
        size.height += text.height + RowGap;
    }
    return size;
}

void LogisticsPage::layout(ui::Rect bounds, const ui::Canvas& canvas) {
    ui::Widget::layout(bounds, canvas);
    const auto report = ::world::logistics_report(registry_, network_);
    rows_.clear();
    // Buttons sit right of the widest row text, so they line up in columns.
    float text_width = 0.0f;
    for (const auto& entry : report) {
        text_width = std::max(text_width, canvas.measure_text(row_text(entry), FontSize).width);
    }
    float y = bounds.y + static_cast<float>(FontSize) + SectionGap + RowGap;
    for (const auto& entry : report) {
        const ::world::RouteKey route = entry.key;
        const ::world::HaulMode mode = chosen_mode(route);
        Row row{ui::button(::world::to_string(mode))
                    .on_click([this, route, mode] {
                        const ::world::HaulMode next = other(mode);
                        modes_[key_of(route)] = next;
                        on_mode_(route, next);
                    })
                    .build(),
                ui::button("-")
                    .on_click([this, route] { on_assign_(route, chosen_mode(route), -1); })
                    .build(),
                ui::button("+")
                    .on_click([this, route] { on_assign_(route, chosen_mode(route), 1); })
                    .build()};
        float x = bounds.x + Indent + text_width + ButtonGap;
        float height = static_cast<float>(FontSize);
        for (ui::Button* button : {row.mode.get(), row.remove.get(), row.add.get()}) {
            const ui::Size size = button->preferred_size(canvas);
            button->layout({x, y, size.width, size.height}, canvas);
            x += size.width + ButtonGap;
            height = std::max(height, size.height);
        }
        y += height + RowGap;
        rows_.push_back(std::move(row));
    }
}

void LogisticsPage::draw(ui::Canvas& canvas) const {
    const auto report = ::world::logistics_report(registry_, network_);
    canvas.draw_text(count_text(report.size()), {bounds_.x, bounds_.y}, FontSize, ui::theme::Text);
    if (report.empty()) {
        canvas.draw_text(EmptyText,
                         {bounds_.x + Indent, bounds_.y + static_cast<float>(FontSize) + SectionGap},
                         FontSize, ui::theme::Muted);
        return;
    }
    // The rows follow the layout; a route added since then shows on the next layout.
    const std::size_t count = std::min(rows_.size(), report.size());
    for (std::size_t i = 0; i < count; ++i) {
        const ui::Rect& button = rows_[i].mode->bounds();
        canvas.draw_text(row_text(report[i]), {bounds_.x + Indent, button.y}, FontSize,
                         ui::theme::Text);
        rows_[i].mode->draw(canvas);
        rows_[i].remove->draw(canvas);
        rows_[i].add->draw(canvas);
    }
}

void LogisticsPage::handle_pointer(ui::Point position, bool released) {
    for (const Row& row : rows_) {
        row.mode->handle_pointer(position, released);
        row.remove->handle_pointer(position, released);
        row.add->handle_pointer(position, released);
    }
}

} // namespace scene::world
