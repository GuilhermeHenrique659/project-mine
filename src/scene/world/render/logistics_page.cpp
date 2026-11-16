#include "scene/world/render/logistics_page.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "scene/world/core/logistics_report.hpp"
#include "scene/world/render/entity_name.hpp"
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
constexpr const char* EmptyText = "No logistic routes: build a storage";

// "Storage #3 (0, 0)".
std::string end_text(const ::world::LogisticEnd& end, core::EntityId id) {
    return entity_name(end.name, id) + " (" + text::format(end.tile.x) + ", " +
           text::format(end.tile.y) + ")";
}

std::string row_text(const ::world::LogisticEntry& entry) {
    const std::string length =
        entry.length > 0 ? text::format(entry.length) + " tiles" : std::string{"unreachable"};
    return end_text(entry.from, entry.key.from) + " -> " + end_text(entry.to, entry.key.to) +
           " - " + length + " - " +
           text::format(entry.haulers) + " workers";
}

std::string count_text(std::size_t count) { return "Logistic routes: " + text::format(count); }

// Whether entry i starts a new destination-type group of the (grouped) report.
bool starts_group(const std::vector<::world::LogisticEntry>& report, std::size_t i) {
    return i == 0 || report[i].to_type != report[i - 1].to_type;
}

// "[-] Mine (2)" ("[+]" while hidden): the group's building name and its route count.
std::string heading_text(const std::vector<::world::LogisticEntry>& report, std::size_t first,
                         bool hidden) {
    std::size_t count = 0;
    for (std::size_t i = first; i < report.size() && report[i].to_type == report[first].to_type;
         ++i) {
        ++count;
    }
    return std::string{hidden ? "[+] " : "[-] "} + report[first].to.name + " (" +
           text::format(count) + ")";
}

std::pair<core::EntityId, core::EntityId> key_of(::world::RouteKey route) {
    return {route.from, route.to};
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
    for (std::size_t i = 0; i < report.size(); ++i) {
        const bool hidden = hidden_.contains(report[i].to_type);
        if (starts_group(report, i)) {
            const auto heading = ui::button(heading_text(report, i, hidden)).build();
            size.height += heading->preferred_size(canvas).height + RowGap;
        }
        if (hidden) {
            continue;
        }
        const ui::Size text = canvas.measure_text(row_text(report[i]), FontSize);
        size.width = std::max(size.width, Indent + text.width);
        size.height += text.height + RowGap;
    }
    return size;
}

void LogisticsPage::layout(ui::Rect bounds, const ui::Canvas& canvas) {
    ui::Widget::layout(bounds, canvas);
    const auto report = ::world::logistics_report(registry_, network_);
    rows_.clear();
    headings_.clear();
    // Buttons sit right of the widest row text, so they line up in columns.
    float text_width = 0.0f;
    for (const auto& entry : report) {
        text_width = std::max(text_width, canvas.measure_text(row_text(entry), FontSize).width);
    }
    // Every line top to bottom: a group's heading, then its rows unless the group is hidden.
    struct Line {
        std::size_t entry;
        bool heading;
    };
    std::vector<Line> lines;
    for (std::size_t i = 0; i < report.size(); ++i) {
        if (starts_group(report, i)) {
            lines.push_back({i, true});
        }
        if (!hidden_.contains(report[i].to_type)) {
            lines.push_back({i, false});
        }
    }
    // Lines all have the height of a button; as many as fit below the title are shown.
    line_height_ = std::max(static_cast<float>(FontSize),
                            ui::button("Take").build()->preferred_size(canvas).height) +
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
        const ::world::BuildingType type = report[i].to_type;
        if (lines[n].heading) {
            auto heading = ui::button(heading_text(report, i, hidden_.contains(type)))
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
        const ::world::RouteKey route = report[i].key;
        const ::world::HaulMode mode = chosen_mode(route);
        // Each box asks for the mode with its direction switched; unchecking the last
        // checked one is refused (a route always carries one way at least).
        const auto change = [this, route, mode](bool take, bool put) {
            const auto next = ::world::haul_mode(take, put);
            if (!next) {
                return;
            }
            modes_[key_of(route)] = *next;
            on_mode_(route, *next);
        };
        Row row{i,
                ui::checkbox("Take")
                    .checked(::world::takes(mode))
                    .on_change([change, mode](bool take) { change(take, ::world::puts(mode)); })
                    .build(),
                ui::checkbox("Put")
                    .checked(::world::puts(mode))
                    .on_change([change, mode](bool put) { change(::world::takes(mode), put); })
                    .build(),
                ui::button("-")
                    .on_click([this, route] { on_assign_(route, chosen_mode(route), -1); })
                    .build(),
                ui::button("+")
                    .on_click([this, route] { on_assign_(route, chosen_mode(route), 1); })
                    .build()};
        float x = bounds.x + Indent + text_width + ButtonGap;
        for (ui::Widget* button : std::initializer_list<ui::Widget*>{
                 row.take.get(), row.put.get(), row.remove.get(), row.add.get()}) {
            const ui::Size size = button->preferred_size(canvas);
            button->layout({x, y, size.width, size.height}, canvas);
            x += size.width + ButtonGap;
        }
        rows_.push_back(std::move(row));
    }
}

void LogisticsPage::scroll(float notches) {
    const auto lines = static_cast<long long>(std::lround(notches));
    const auto target = static_cast<long long>(first_line_) - lines;
    first_line_ = static_cast<std::size_t>(
        std::clamp<long long>(target, 0, static_cast<long long>(last_first_line_)));
}

void LogisticsPage::draw_scrollbar(ui::Canvas& canvas) const {
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
    // Top to bottom: each heading before the rows below it.
    std::size_t next_heading = 0;
    for (const Row& row : rows_) {
        while (next_heading < headings_.size() &&
               headings_[next_heading]->bounds().y < row.take->bounds().y) {
            headings_[next_heading++]->draw(canvas);
        }
        if (row.entry >= report.size()) {
            continue;
        }
        const ui::Rect& button = row.take->bounds();
        canvas.draw_text(row_text(report[row.entry]), {bounds_.x + Indent, button.y}, FontSize,
                         ui::theme::Text);
        row.take->draw(canvas);
        row.put->draw(canvas);
        row.remove->draw(canvas);
        row.add->draw(canvas);
    }
    while (next_heading < headings_.size()) {
        headings_[next_heading++]->draw(canvas);
    }
    draw_scrollbar(canvas);
}

void LogisticsPage::handle_pointer(ui::Point position, bool released) {
    for (const auto& heading : headings_) {
        heading->handle_pointer(position, released);
    }
    for (const Row& row : rows_) {
        row.take->handle_pointer(position, released);
        row.put->handle_pointer(position, released);
        row.remove->handle_pointer(position, released);
        row.add->handle_pointer(position, released);
    }
}

} // namespace scene::world
