#include "scene/world/render/energy_page.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "scene/world/core/power_report.hpp"
#include "text/parse.hpp"
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
        std::string text = entry.name + " - " + kw(entry.kw);
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
    : ui::Widget(ui::theme::label_style()), registry_(registry) {}

ui::Size EnergyPage::preferred_size(const ui::Canvas& canvas) const {
    ui::Size size{};
    for (const Line& line : page_lines(::world::power_report(registry_))) {
        const ui::Size text = canvas.measure_text(line.text, FontSize);
        size.width = std::max(size.width, line.indent + text.width);
        size.height += line.gap_before + text.height + LineGap;
    }
    return size;
}

void EnergyPage::draw(ui::Canvas& canvas) const {
    float y = bounds_.y;
    for (const Line& line : page_lines(::world::power_report(registry_))) {
        y += line.gap_before;
        canvas.draw_text(line.text, {bounds_.x + line.indent, y}, FontSize, line.color);
        y += static_cast<float>(FontSize) + LineGap;
    }
}

} // namespace scene::world
