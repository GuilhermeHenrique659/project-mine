#include "scene/world/render/workers_page.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include "text/parse.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {

constexpr int FontSize = 20;
constexpr float RowGap = 12.0f;
constexpr float SectionGap = 16.0f;
constexpr float ButtonGap = 8.0f;
constexpr float Indent = 24.0f;
constexpr float ScrollbarWidth = 10.0f;
constexpr const char* EmptyText = "No building needs workers";

using Workplaces = std::vector<::world::WorkplaceEntry>;

std::string row_text(const ::world::WorkplaceEntry& entry) {
    return entry.name + " (" + text::format(entry.tile.x) + ", " + text::format(entry.tile.y) +
           ") - " + text::format(entry.assigned) + "/" + text::format(entry.needed) + " workers";
}

std::string idle_text(const ::world::WorkerReport& report) {
    return "Idle workers: " + text::format(report.idle);
}

// Whether workplace i starts a new building-type group of the (grouped) list.
bool starts_group(const Workplaces& workplaces, std::size_t i) {
    return i == 0 || workplaces[i].type != workplaces[i - 1].type;
}

// "[-] Mine (2)" ("[+]" while hidden): the group's building name and its workplace count.
std::string heading_text(const Workplaces& workplaces, std::size_t first, bool hidden) {
    std::size_t count = 0;
    for (std::size_t i = first; i < workplaces.size() && workplaces[i].type == workplaces[first].type;
         ++i) {
        ++count;
    }
    return std::string{hidden ? "[+] " : "[-] "} + workplaces[first].name + " (" +
           text::format(count) + ")";
}

} // namespace

WorkersPage::WorkersPage(const core::Registry& registry, AssignHandler on_assign)
    : ui::Widget(ui::theme::label_style()), registry_(registry), on_assign_(std::move(on_assign)) {}

::world::WorkerReport WorkersPage::grouped_report() const {
    ::world::WorkerReport report = ::world::worker_report(registry_);
    std::stable_sort(report.workplaces.begin(), report.workplaces.end(),
                     [](const ::world::WorkplaceEntry& a, const ::world::WorkplaceEntry& b) {
                         return a.type < b.type;
                     });
    return report;
}

ui::Size WorkersPage::preferred_size(const ui::Canvas& canvas) const {
    const ::world::WorkerReport report = grouped_report();
    const Workplaces& workplaces = report.workplaces;
    ui::Size size = canvas.measure_text(idle_text(report), FontSize);
    size.height += SectionGap;
    for (std::size_t i = 0; i < workplaces.size(); ++i) {
        const bool hidden = hidden_.contains(workplaces[i].type);
        if (starts_group(workplaces, i)) {
            const auto heading = ui::button(heading_text(workplaces, i, hidden)).build();
            size.height += heading->preferred_size(canvas).height + RowGap;
        }
        if (hidden) {
            continue;
        }
        const ui::Size text = canvas.measure_text(row_text(workplaces[i]), FontSize);
        size.width = std::max(size.width, Indent + text.width);
        size.height += text.height + RowGap;
    }
    return size;
}

void WorkersPage::layout(ui::Rect bounds, const ui::Canvas& canvas) {
    ui::Widget::layout(bounds, canvas);
    const ::world::WorkerReport report = grouped_report();
    const Workplaces& workplaces = report.workplaces;
    rows_.clear();
    headings_.clear();
    // Buttons sit right of the widest row text, so they line up in two columns.
    float text_width = 0.0f;
    for (const auto& entry : workplaces) {
        text_width = std::max(text_width, canvas.measure_text(row_text(entry), FontSize).width);
    }
    // Every line top to bottom: a group's heading, then its rows unless the group is hidden.
    struct Line {
        std::size_t entry;
        bool heading;
    };
    std::vector<Line> lines;
    for (std::size_t i = 0; i < workplaces.size(); ++i) {
        if (starts_group(workplaces, i)) {
            lines.push_back({i, true});
        }
        if (!hidden_.contains(workplaces[i].type)) {
            lines.push_back({i, false});
        }
    }
    // Lines all have the height of a button; as many as fit below the title are shown.
    line_height_ = std::max(static_cast<float>(FontSize),
                            ui::button("+").build()->preferred_size(canvas).height) +
                   RowGap;
    list_top_ = bounds.y + static_cast<float>(FontSize) + SectionGap + RowGap;
    const float room = bounds.y + bounds.height - list_top_;
    shown_lines_ = bounds.height <= 0.0f
                       ? lines.size()
                       : std::min(lines.size(), static_cast<std::size_t>(std::max(
                                                    1.0f, std::floor(room / line_height_))));
    last_first_line_ = lines.size() - shown_lines_;
    first_line_ = std::min(first_line_, last_first_line_);

    for (std::size_t n = first_line_; n < first_line_ + shown_lines_; ++n) {
        const std::size_t i = lines[n].entry;
        const float y = list_top_ + static_cast<float>(n - first_line_) * line_height_;
        const ::world::BuildingType type = workplaces[i].type;
        if (lines[n].heading) {
            auto heading = ui::button(heading_text(workplaces, i, hidden_.contains(type)))
                               .on_click([this, type] {
                                   if (!hidden_.erase(type)) {
                                       hidden_.insert(type);
                                   }
                               })
                               .build();
            const ui::Size size = heading->preferred_size(canvas);
            heading->layout({bounds.x, y, size.width, size.height}, canvas);
            headings_.push_back(std::move(heading));
            continue;
        }
        const core::EntityId id = workplaces[i].id;
        Row row{i, ui::button("-").on_click([this, id] { on_assign_(id, -1); }).build(),
                ui::button("+").on_click([this, id] { on_assign_(id, 1); }).build()};
        const ui::Size remove = row.remove->preferred_size(canvas);
        const ui::Size add = row.add->preferred_size(canvas);
        const float x = bounds.x + Indent + text_width + ButtonGap;
        row.remove->layout({x, y, remove.width, remove.height}, canvas);
        row.add->layout({x + remove.width + ButtonGap, y, add.width, add.height}, canvas);
        rows_.push_back(std::move(row));
    }
}

void WorkersPage::scroll(float notches) {
    const auto lines = static_cast<long long>(std::lround(notches));
    const auto target = static_cast<long long>(first_line_) - lines;
    first_line_ = static_cast<std::size_t>(
        std::clamp<long long>(target, 0, static_cast<long long>(last_first_line_)));
}

void WorkersPage::draw_scrollbar(ui::Canvas& canvas) const {
    if (last_first_line_ == 0) {
        return;
    }
    const float height = static_cast<float>(shown_lines_) * line_height_ - RowGap;
    const ui::Rect track{bounds_.x + bounds_.width - ScrollbarWidth, list_top_, ScrollbarWidth,
                         height};
    canvas.fill_rect(track, ui::theme::Outline);
    // The thumb's share of the track is the share of lines shown.
    const auto total = static_cast<float>(shown_lines_ + last_first_line_);
    const float thumb_height = track.height * static_cast<float>(shown_lines_) / total;
    const float offset = (track.height - thumb_height) * static_cast<float>(first_line_) /
                         static_cast<float>(last_first_line_);
    canvas.fill_rect({track.x, track.y + offset, track.width, thumb_height}, ui::theme::Accent);
}

void WorkersPage::draw(ui::Canvas& canvas) const {
    const ::world::WorkerReport report = grouped_report();
    const Workplaces& workplaces = report.workplaces;
    canvas.draw_text(idle_text(report), {bounds_.x, bounds_.y}, FontSize, ui::theme::Text);
    if (workplaces.empty()) {
        canvas.draw_text(EmptyText,
                         {bounds_.x + Indent, bounds_.y + static_cast<float>(FontSize) + SectionGap},
                         FontSize, ui::theme::Muted);
        return;
    }
    // The rows follow the layout; a workplace added since then shows on the next layout.
    // Top to bottom: each heading before the rows below it.
    std::size_t next_heading = 0;
    for (const Row& row : rows_) {
        while (next_heading < headings_.size() &&
               headings_[next_heading]->bounds().y < row.remove->bounds().y) {
            headings_[next_heading++]->draw(canvas);
        }
        if (row.entry >= workplaces.size()) {
            continue;
        }
        canvas.draw_text(row_text(workplaces[row.entry]),
                         {bounds_.x + Indent, row.remove->bounds().y}, FontSize, ui::theme::Text);
        row.remove->draw(canvas);
        row.add->draw(canvas);
    }
    while (next_heading < headings_.size()) {
        headings_[next_heading++]->draw(canvas);
    }
    draw_scrollbar(canvas);
}

void WorkersPage::handle_pointer(ui::Point position, bool released) {
    for (const auto& heading : headings_) {
        heading->handle_pointer(position, released);
    }
    for (const Row& row : rows_) {
        row.remove->handle_pointer(position, released);
        row.add->handle_pointer(position, released);
    }
}

} // namespace scene::world
