#include "scene/world/render/workers_page.hpp"

#include <algorithm>
#include <string>

#include "text/parse.hpp"
#include "scene/world/render/entity_name.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {

constexpr int FontSize = 20;
constexpr float RowGap = 12.0f;
constexpr float SectionGap = 16.0f;
constexpr float ButtonGap = 8.0f;
constexpr float Indent = 24.0f;
constexpr const char* EmptyText = "No building needs workers";

using Workplaces = std::vector<::world::WorkplaceEntry>;

std::string row_text(const ::world::WorkplaceEntry& entry) {
    return entity_name(entry.name, entry.id) + " (" + text::format(entry.tile.x) + ", " + text::format(entry.tile.y) +
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
    : ui::Composite(ui::theme::label_style()), registry_(registry), on_assign_(std::move(on_assign)) {}

::world::WorkerReport WorkersPage::grouped_report() const {
    ::world::WorkerReport report = ::world::worker_report(registry_);
    std::stable_sort(report.workplaces.begin(), report.workplaces.end(),
                     [](const ::world::WorkplaceEntry& a, const ::world::WorkplaceEntry& b) {
                         return a.type < b.type;
                     });
    return report;
}

// The idle count, then the list: a heading row per group, an indented row per shown
// workplace with its text and the - / + buttons (or the "no building" hint).
void WorkersPage::build_tree(const std::vector<Line>& lines) const {
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
        const core::EntityId id = line.id;
        Row row;
        auto label = text(ui::theme::Text);
        auto remove = ui::button("-").on_click([this, id] { on_assign_(id, -1); }).build();
        auto add = ui::button("+").on_click([this, id] { on_assign_(id, 1); }).build();
        row = {label.get(), remove.get(), add.get()};
        rows_.push_back(row);
        list.child(ui::row()
                       .style(indented)
                       .child(std::move(label))
                       .child(std::move(remove))
                       .child(std::move(add))
                       .build());
    }

    auto idle = text(ui::theme::Text);
    idle_ = idle.get();
    set_tree(ui::column().gap(SectionGap + RowGap).child(std::move(idle)).child(list.build()).build());
}

void WorkersPage::sync(const ui::Canvas& canvas) const {
    const ::world::WorkerReport report = grouped_report();
    const Workplaces& workplaces = report.workplaces;
    // Every line top to bottom: a group's heading, then its rows unless the group is hidden.
    std::vector<Line> lines;
    for (std::size_t i = 0; i < workplaces.size(); ++i) {
        if (starts_group(workplaces, i)) {
            lines.push_back({true, workplaces[i].type, 0});
        }
        if (!hidden_.contains(workplaces[i].type)) {
            lines.push_back({false, workplaces[i].type, workplaces[i].id});
        }
    }
    if (!built_ || lines != lines_) {
        build_tree(lines);
        lines_ = lines;
        built_ = true;
    }

    idle_->set_text(idle_text(report));
    // Buttons sit right of the widest row text, so they line up in two columns.
    float text_width = 0.0f;
    for (const auto& entry : workplaces) {
        text_width = std::max(text_width, canvas.measure_text(row_text(entry), FontSize).width);
    }
    std::size_t heading = 0;
    std::size_t row = 0;
    for (std::size_t i = 0; i < workplaces.size(); ++i) {
        if (starts_group(workplaces, i)) {
            headings_[heading++]->set_text(
                heading_text(workplaces, i, hidden_.contains(workplaces[i].type)));
        }
        if (hidden_.contains(workplaces[i].type)) {
            continue;
        }
        rows_[row].text->set_text(row_text(workplaces[i]));
        rows_[row].text->set_width(text_width);
        ++row;
    }
}

} // namespace scene::world
