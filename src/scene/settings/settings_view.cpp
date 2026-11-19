#include "scene/settings/settings_view.hpp"

#include <algorithm>

#include "game/game_events.hpp"
#include "ui/theme.hpp"

namespace scene::settings {

namespace {
constexpr float CardWidth = 640.0f;
constexpr int TitleFontSize = 40;

std::size_t preset_index(game::Resolution resolution) {
    const auto& presets = game::resolution_presets();
    const auto found = std::find(presets.begin(), presets.end(), resolution);
    return found == presets.end() ? 0 : static_cast<std::size_t>(found - presets.begin());
}
} // namespace

SettingsView::SettingsView(event::EventBus& bus, game::DisplaySettings current)
    : current_(current) {
    auto title = ui::label("Settings")
                     .style(ui::theme::label_style()
                                .with_font_size(TitleFontSize)
                                .with_text_align(ui::TextAlign::Center))
                     .build();
    auto resolution_label = ui::label("Resolution").build();

    std::vector<std::string> labels;
    for (const game::Resolution& resolution : game::resolution_presets()) {
        labels.push_back(game::resolution_label(resolution));
    }
    auto dropdown = ui::dropdown()
                        .options(std::move(labels))
                        .selected(preset_index(current_.resolution))
                        .on_change([this, &bus](std::size_t index) {
                            current_.resolution = game::resolution_presets()[index];
                            bus.publish(game::DisplaySettingsChanged{current_});
                        })
                        .build();
    auto checkbox = ui::checkbox("Fullscreen")
                        .checked(current_.fullscreen)
                        .on_change([this, &bus](bool checked) {
                            current_.fullscreen = checked;
                            bus.publish(game::DisplaySettingsChanged{current_});
                        })
                        .build();

    ui::join_focus_group(focus_group_, *dropdown);
    ui::join_focus_group(focus_group_, *checkbox);
    const ui::ButtonFactory button_factory(focus_group_);
    auto back = button_factory.create(
        "Back", [&bus] { bus.publish(game::BackToMenuRequested{}); });

    resolution_dropdown_ = dropdown.get();
    fullscreen_checkbox_ = checkbox.get();
    back_button_ = back.get();

    ui::CardBuilder builder = ui::card();
    builder.style(ui::theme::card_style().with_width(CardWidth));
    builder.child(std::move(title));
    builder.child(std::move(resolution_label));
    builder.child(std::move(dropdown));
    builder.child(std::move(checkbox));
    builder.child(std::move(back));
    set_root(builder.build());
}

} // namespace scene::settings
