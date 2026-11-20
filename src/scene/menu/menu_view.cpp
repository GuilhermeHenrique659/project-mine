#include "scene/menu/menu_view.hpp"

#include "game/game_events.hpp"
#include "graphics/sprite.hpp"
#include "ui/theme.hpp"

namespace scene::menu {

namespace {
constexpr float CardWidth = 560.0f;
constexpr float ButtonGap = 10.0f;
constexpr int TitleFontSize = 56;
} // namespace

MenuView::MenuView(event::EventBus& bus) {
    auto title = ui::label("Project Mine")
                     .style(ui::theme::label_style()
                                .with_font_size(TitleFontSize)
                                .with_text_align(ui::TextAlign::Center))
                     .build();

    const ui::ButtonFactory button_factory(focus_group_);
    auto start_new_game_button = button_factory.create(
        "New Game", [&bus] { bus.publish(game::NewGameRequested{}); },
        ui::theme::start_button_style());
    auto load_game_button = button_factory.create(
        "Load Game", [&bus] { bus.publish(game::LoadGameRequested{}); });
    auto settings_button = button_factory.create(
        "Settings", [&bus] { bus.publish(game::SettingsRequested{}); });
    auto quit_button =
        button_factory.create("Quit", [&bus] { bus.publish(game::QuitRequested{}); },
                              ui::theme::quit_button_style());

    // Icons are optional: without the texture the button stays text only.
    const auto set_icon = [](ui::Button& button, std::string texture) {
        button.set_icon([texture = std::move(texture)](ui::Canvas& canvas, ui::Rect rect) {
            graphics::draw_loaded_sprite(canvas, {texture, std::nullopt}, rect);
        });
    };
    set_icon(*start_new_game_button, "menu_new_game");
    set_icon(*load_game_button, "menu_load_game");
    set_icon(*settings_button, "menu_settings");
    set_icon(*quit_button, "menu_quit");

    buttons_ = {start_new_game_button.get(), load_game_button.get(), settings_button.get(),
                quit_button.get()};

    ui::CardBuilder builder = ui::card();
    builder.style(ui::theme::card_style().with_width(CardWidth).with_gap(ButtonGap));
    builder.child(std::move(title));
    builder.child(std::move(start_new_game_button));
    builder.child(std::move(load_game_button));
    builder.child(std::move(settings_button));
    builder.child(std::move(quit_button));
    set_root(builder.build());
}

} // namespace scene::menu
