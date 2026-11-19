#include "scene/world/render/energy_page.hpp"

#include <string>
#include <vector>

#include "scene/world/core/power_report.hpp"
#include "text/parse.hpp"
#include "scene/world/render/entity_name.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {

constexpr int FontSize = 20;
constexpr float LineGap = 8.0f;
// Extra space before each section title.
constexpr float SectionGap = 16.0f;
constexpr float Indent = 24.0f;
constexpr ui::Color Warning{230, 90, 80, 255};

struct Line {
    std::string text;
    ui::Color color;
    float indent = 0.0f;
    float gap_before = 0.0f;
};

std::string kw(float value) { return text::format(value) + " kW"; }

void add_section(std::vector<Line>& lines, const std::string& title,
                 const std::vector<::world::PowerEntry>& entries, const std::string& inactive) {
    lines.push_back({title + " (" + text::format(entries.size()) + ")", ui::theme::Accent, 0.0f,
                     SectionGap});
    if (entries.empty()) {
        lines.push_back({"None", ui::theme::Muted, Indent});
        return;
    }
    for (const ::world::PowerEntry& entry : entries) {
        std::string text = entity_name(entry.name, entry.id) + " - " + kw(entry.kw);
        if (!entry.active) {
            text += " " + inactive;
        }
        lines.push_back({text, entry.active ? ui::theme::Text : Warning, Indent});
    }
}

std::vector<Line> page_lines(const ::world::PowerReport& report) {
    std::vector<Line> lines;
    lines.push_back({"Produced: " + kw(report.produced_kw), ui::theme::Text});
    lines.push_back({"Consumed: " + kw(report.demand_kw), ui::theme::Text});
    if (report.overloaded()) {
        lines.push_back(
            {"Overloaded: consumers are idle until production covers demand", Warning});
    }
    add_section(lines, "Generators", report.generators, "(stopped)");
    add_section(lines, "Consumers", report.consumers, "(no power)");
    return lines;
}

} // namespace

EnergyPage::EnergyPage(const core::Registry& registry)
    : ui::Composite(ui::theme::label_style()), registry_(registry) {}

void EnergyPage::sync(const ui::Canvas&) const {
    const std::vector<Line> lines = page_lines(::world::power_report(registry_));
    std::vector<Slot> slots;
    for (const Line& line : lines) {
        slots.push_back({line.indent, line.gap_before});
    }
    if (slots != slots_) {
        ui::BoxBuilder column = ui::column();
        column.gap(LineGap);
        labels_.clear();
        for (const Slot& slot : slots) {
            auto label =
                ui::label("").style(ui::theme::label_style().with_font_size(FontSize)).build();
            labels_.push_back(label.get());
            column.child(ui::row()
                             .style(ui::Style{}.with_padding({slot.gap_before, 0.0f, 0.0f, slot.indent}))
                             .child(std::move(label))
                             .build());
        }
        set_tree(column.build());
        slots_ = std::move(slots);
    }
    for (std::size_t i = 0; i < lines.size(); ++i) {
        labels_[i]->set_text(lines[i].text);
        labels_[i]->set_color(lines[i].color);
    }
}

} // namespace scene::world
