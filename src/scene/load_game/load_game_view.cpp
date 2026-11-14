#include "scene/load_game/load_game_view.hpp"

#include <raylib.h>

#include "game/game_events.hpp"
#include "ui/theme.hpp"

namespace scene::load_game {

namespace {
constexpr float CardWidth = 720.0f;
constexpr float CardTop = 80.0f;
constexpr float ButtonGap = 10.0f;
constexpr int TitleFontSize = 40;
constexpr int SaveFontSize = 24;
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
    for (const game::SaveGame& save : saves) {
        auto button = button_factory.create(
            save.name + "   " + game::format_created_at(save.created_at),
            [&bus, name = save.name] { bus.publish(game::LoadSaveRequested{name}); },
            ui::theme::button_style().with_font_size(SaveFontSize));
        save_buttons_.push_back(button.get());
        builder.child(std::move(button));
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
    root_ = builder.build();
}

void LoadGameView::layout(const ui::Canvas& canvas, float screen_width) const {
    const ui::Size size = root_->preferred_size(canvas);
    root_->layout({(screen_width - size.width) / 2.0f, CardTop, size.width, size.height}, canvas);
}

void LoadGameView::draw() const {
    ui::RaylibCanvas canvas;
    layout(canvas, static_cast<float>(GetScreenWidth()));
    root_->draw(canvas);
}

void LoadGameView::handle_pointer(ui::Point position, bool released) {
    root_->handle_pointer(position, released);
}

} // namespace scene::load_game
