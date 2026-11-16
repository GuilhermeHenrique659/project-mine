#include "scene/world/render/workers_page.hpp"

#include <algorithm>
#include <string>

#include "scene/world/core/worker_report.hpp"
#include "text/parse.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {

constexpr int FontSize = 20;
constexpr float RowGap = 12.0f;
constexpr float SectionGap = 16.0f;
constexpr float ButtonGap = 8.0f;
constexpr float Indent = 24.0f;

std::string row_text(const ::world::WorkplaceEntry& entry) {
    return entry.name + " (" + text::format(entry.tile.x) + ", " + text::format(entry.tile.y) +
           ") - " + text::format(entry.assigned) + "/" + text::format(entry.needed) + " workers";
}

std::string idle_text(const ::world::WorkerReport& report) {
    return "Idle workers: " + text::format(report.idle);
}

constexpr const char* EmptyText = "No building needs workers";

} // namespace

WorkersPage::WorkersPage(const core::Registry& registry, AssignHandler on_assign)
    : ui::Widget(ui::theme::label_style()), registry_(registry), on_assign_(std::move(on_assign)) {}

ui::Size WorkersPage::preferred_size(const ui::Canvas& canvas) const {
    const ::world::WorkerReport report = ::world::worker_report(registry_);
    ui::Size size = canvas.measure_text(idle_text(report), FontSize);
    size.height += SectionGap;
    for (const auto& entry : report.workplaces) {
        const ui::Size text = canvas.measure_text(row_text(entry), FontSize);
        size.width = std::max(size.width, Indent + text.width);
        size.height += text.height + RowGap;
    }
    return size;
}

void WorkersPage::layout(ui::Rect bounds, const ui::Canvas& canvas) {
    ui::Widget::layout(bounds, canvas);
    const ::world::WorkerReport report = ::world::worker_report(registry_);
    rows_.clear();
    // Buttons sit right of the widest row text, so they line up in two columns.
    float text_width = 0.0f;
    for (const auto& entry : report.workplaces) {
        text_width = std::max(text_width, canvas.measure_text(row_text(entry), FontSize).width);
    }
    float y = bounds.y + static_cast<float>(FontSize) + SectionGap + RowGap;
    for (const auto& entry : report.workplaces) {
        const core::EntityId id = entry.id;
        Row row{ui::button("-").on_click([this, id] { on_assign_(id, -1); }).build(),
                ui::button("+").on_click([this, id] { on_assign_(id, 1); }).build()};
        const ui::Size remove = row.remove->preferred_size(canvas);
        const ui::Size add = row.add->preferred_size(canvas);
        const float x = bounds.x + Indent + text_width + ButtonGap;
        row.remove->layout({x, y, remove.width, remove.height}, canvas);
        row.add->layout({x + remove.width + ButtonGap, y, add.width, add.height}, canvas);
        y += std::max({remove.height, add.height, static_cast<float>(FontSize)}) + RowGap;
        rows_.push_back(std::move(row));
    }
}

void WorkersPage::draw(ui::Canvas& canvas) const {
    const ::world::WorkerReport report = ::world::worker_report(registry_);
    canvas.draw_text(idle_text(report), {bounds_.x, bounds_.y}, FontSize, ui::theme::Text);
    if (report.workplaces.empty()) {
        canvas.draw_text(EmptyText,
                         {bounds_.x + Indent, bounds_.y + static_cast<float>(FontSize) + SectionGap},
                         FontSize, ui::theme::Muted);
        return;
    }
    // The rows follow the layout; a workplace added since then shows on the next layout.
    const std::size_t count = std::min(rows_.size(), report.workplaces.size());
    for (std::size_t i = 0; i < count; ++i) {
        const ui::Rect& button = rows_[i].remove->bounds();
        canvas.draw_text(row_text(report.workplaces[i]), {bounds_.x + Indent, button.y}, FontSize,
                         ui::theme::Text);
        rows_[i].remove->draw(canvas);
        rows_[i].add->draw(canvas);
    }
}

void WorkersPage::handle_pointer(ui::Point position, bool released) {
    for (const Row& row : rows_) {
        row.remove->handle_pointer(position, released);
        row.add->handle_pointer(position, released);
    }
}

} // namespace scene::world
