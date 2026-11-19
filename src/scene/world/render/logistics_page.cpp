#include "scene/world/render/logistics_page.hpp"

#include <algorithm>
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
    : ui::Composite(ui::theme::label_style()), registry_(registry), network_(network),
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

void LogisticsPage::change_mode(::world::RouteKey route, bool take, bool put) const {
    // A route always carries one way at least: unchecking the last checked box is refused
    // (the next sync shows the box checked again).
    const auto next = ::world::haul_mode(take, put);
    if (!next) {
        return;
    }
    modes_[key_of(route)] = *next;
    on_mode_(route, *next);
}

// The route count, then the list: a heading row per group, an indented row per shown route
// with its text, the Take / Put checkboxes and the - / + buttons (or the storage hint).
void LogisticsPage::build_tree(const std::vector<Line>& lines) const {
    const auto text = [](ui::Color color) {
        return ui::label("")
            .style(ui::theme::label_style().with_font_size(FontSize).with_foreground(color))
            .build();
    };
    const ui::Style indented = ui::Style{}.with_padding({0.0f, 0.0f, 0.0f, Indent}).with_gap(ButtonGap);
    headings_.clear();
    rows_.clear();

    ui::BoxBuilder list = ui::column();
    list.gap(RowGap);
    if (lines.empty()) {
        auto empty = text(ui::theme::Muted);
        empty->set_text(EmptyText);
        list.child(ui::row().style(indented).child(std::move(empty)).build());
    }
    for (const Line& line : lines) {
        if (line.heading) {
            const ::world::BuildingType type = line.type;
            auto heading = ui::button("")
                               .on_click([this, type] {
                                   if (!hidden_.erase(type)) {
                                       hidden_.insert(type);
                                   }
                               })
                               .build();
            headings_.push_back(heading.get());
            list.child(ui::row().child(std::move(heading)).build());
            continue;
        }
        const ::world::RouteKey route = line.route;
        // Each box asks for the mode with its own direction switched, read at click time.
        auto take = ui::checkbox("Take")
                        .on_change([this, route](bool on) {
                            change_mode(route, on, ::world::puts(chosen_mode(route)));
                        })
                        .build();
        auto put = ui::checkbox("Put")
                       .on_change([this, route](bool on) {
                           change_mode(route, ::world::takes(chosen_mode(route)), on);
                       })
                       .build();
        auto remove = ui::button("-")
                          .on_click([this, route] { on_assign_(route, chosen_mode(route), -1); })
                          .build();
        auto add = ui::button("+")
                       .on_click([this, route] { on_assign_(route, chosen_mode(route), 1); })
                       .build();
        auto label = text(ui::theme::Text);
        rows_.push_back({label.get(), take.get(), put.get(), remove.get(), add.get()});
        list.child(ui::row()
                       .style(indented)
                       .child(std::move(label))
                       .child(std::move(take))
                       .child(std::move(put))
                       .child(std::move(remove))
                       .child(std::move(add))
                       .build());
    }

    auto count = text(ui::theme::Text);
    count_ = count.get();
    set_tree(ui::column().gap(SectionGap + RowGap).child(std::move(count)).child(list.build()).build());
}

void LogisticsPage::sync(const ui::Canvas& canvas) const {
    const auto report = ::world::logistics_report(registry_, network_);
    // Every line top to bottom: a group's heading, then its rows unless the group is hidden.
    std::vector<Line> lines;
    for (std::size_t i = 0; i < report.size(); ++i) {
        if (starts_group(report, i)) {
            lines.push_back({true, report[i].to_type, {}});
        }
        if (!hidden_.contains(report[i].to_type)) {
            lines.push_back({false, report[i].to_type, report[i].key});
        }
    }
    if (!built_ || lines != lines_) {
        build_tree(lines);
        lines_ = lines;
        built_ = true;
    }

    count_->set_text(count_text(report.size()));
    // The controls sit right of the widest row text, so they line up in columns.
    float text_width = 0.0f;
    for (const auto& entry : report) {
        text_width = std::max(text_width, canvas.measure_text(row_text(entry), FontSize).width);
    }
    std::size_t heading = 0;
    std::size_t row = 0;
    for (std::size_t i = 0; i < report.size(); ++i) {
        if (starts_group(report, i)) {
            headings_[heading++]->set_text(
                heading_text(report, i, hidden_.contains(report[i].to_type)));
        }
        if (hidden_.contains(report[i].to_type)) {
            continue;
        }
        const ::world::HaulMode mode = chosen_mode(report[i].key);
        Row& shown = rows_[row++];
        shown.text->set_text(row_text(report[i]));
        shown.text->set_width(text_width);
        shown.take->set_checked(::world::takes(mode));
        shown.put->set_checked(::world::puts(mode));
    }
}

} // namespace scene::world
