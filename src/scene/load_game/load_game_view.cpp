#include "scene/load_game/load_game_view.hpp"

#include "game/game_events.hpp"
#include "ui/scroll_box.hpp"
#include "ui/theme.hpp"

namespace scene::load_game {

namespace {
constexpr float CardWidth = 720.0f;
constexpr float ButtonGap = 10.0f;
constexpr int TitleFontSize = 40;
constexpr int SaveFontSize = 24;
// Saves shown at once; a longer list scrolls (one save per wheel notch).
constexpr float VisibleSaves = 4.0f;
} // namespace

LoadGameView::LoadGameView(event::EventBus& bus, const std::vector<game::SaveGame>& saves) {
    auto title = ui::label("Load Game")
                     .style(ui::theme::label_style()
                                .with_font_size(TitleFontSize)
                                .with_text_align(ui::TextAlign::Center))
                     .build();

    ui::CardBuilder builder = ui::card();
    builder.style(ui::theme::card_style().with_width(CardWidth).with_gap(ButtonGap));
    builder.child(std::move(title));

    const ui::ButtonFactory button_factory(focus_group_);
    ui::BoxBuilder list = ui::column();
    list.gap(ButtonGap).align(ui::Align::Stretch);
    float save_height = 0.0f;
    for (const game::SaveGame& save : saves) {
        auto button = button_factory.create(
            save.name + "   " + game::format_created_at(save.created_at),
            [&bus, name = save.name] { bus.publish(game::LoadSaveRequested{name}); },
            ui::theme::button_style().with_font_size(SaveFontSize));
        save_buttons_.push_back(button.get());
        save_height = button->style().padding.vertical() + static_cast<float>(SaveFontSize);
        list.child(std::move(button));
    }
    if (!saves.empty()) {
        const float step = save_height + ButtonGap;
        builder.child(ui::scroll_box(list.build())
                          .step(step)
                          .max_height(VisibleSaves * step - ButtonGap)
                          .build());
    }
    if (saves.empty()) {
        auto empty = ui::label("No saved games yet")
                         .style(ui::theme::label_style()
                                    .with_foreground(ui::theme::Muted)
                                    .with_text_align(ui::TextAlign::Center))
                         .build();
        empty_label_ = empty.get();
        builder.child(std::move(empty));
    }

    auto back = button_factory.create(
        "Back", [&bus] { bus.publish(game::BackToMenuRequested{}); });
    back_button_ = back.get();
    builder.child(std::move(back));
    set_root(builder.build());
}

} // namespace scene::load_game
