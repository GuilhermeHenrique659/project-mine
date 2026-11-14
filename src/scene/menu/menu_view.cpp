#include "scene/menu/menu_view.hpp"

#include <raylib.h>

#include "game/game_events.hpp"

namespace scene::menu {

namespace {
constexpr float CardWidth = 400.0f;
constexpr float CardTop = 160.0f;
constexpr int TitleFontSize = 48;
constexpr int HintFontSize = 20;
} // namespace

MenuView::MenuView(event::EventBus& bus) {
    auto title = ui::label("Project Mine")
                     .style(ui::theme::label_style()
                                .with_font_size(TitleFontSize)
                                .with_text_align(ui::TextAlign::Center))
                     .build();

    const ui::ButtonFactory button_factory(focus_group_);
    auto start_new_game_button = button_factory.create(
        "New Game", [&bus] { bus.publish(game::NewGameRequested{}); });
    auto load_game_button = button_factory.create(
        "Load Game", [&bus] { bus.publish(game::LoadGameRequested{}); });
    auto quit_button =
        button_factory.create("Quit", [&bus] { bus.publish(game::QuitRequested{}); });

    buttons_ = {start_new_game_button.get(), load_game_button.get(), quit_button.get()};

    ui::CardBuilder builder = ui::card();
    builder.style(ui::theme::card_style().with_width(CardWidth));
    builder.child(std::move(title));
    builder.child(std::move(start_new_game_button));
    builder.child(std::move(load_game_button));
    builder.child(std::move(quit_button));
    root_ = builder.build();
}

void MenuView::layout(const ui::Canvas& canvas, float screen_width) const {
    const ui::Size size = root_->preferred_size(canvas);
    root_->layout({(screen_width - size.width) / 2.0f, CardTop, size.width, size.height}, canvas);
}

void MenuView::draw() const {
    ui::RaylibCanvas canvas;
    layout(canvas, static_cast<float>(GetScreenWidth()));
    root_->draw(canvas);

    DrawText("UP/DOWN to navigate, ENTER to select", 20, GetScreenHeight() - 40, HintFontSize,
             GRAY);
}

void MenuView::handle_pointer(ui::Point position, bool released) {
    root_->handle_pointer(position, released);
}

} // namespace scene::menu
