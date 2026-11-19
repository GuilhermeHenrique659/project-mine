#include "scene/new_game/new_game_view.hpp"

#include "game/game_events.hpp"
#include "game/save_game.hpp"
#include "ui/theme.hpp"

namespace scene::new_game {

namespace {
constexpr float CardWidth = 640.0f;
constexpr int TitleFontSize = 40;
constexpr int FieldFontSize = 28;
constexpr const char* EmptyNameMessage = "Type a name for the save";
constexpr const char* InvalidNameMessage = "Use only letters, numbers, spaces, - and _";
} // namespace

NewGameView::NewGameView(event::EventBus& bus) : bus_(bus) {
    auto title = ui::label("New Game")
                     .style(ui::theme::label_style()
                                .with_font_size(TitleFontSize)
                                .with_text_align(ui::TextAlign::Center))
                     .build();
    auto prompt = ui::label("Save name").build();
    auto name_input = ui::input()
                          .style(ui::theme::input_style().with_font_size(FieldFontSize))
                          .placeholder("My factory")
                          .max_length(game::MaxSaveNameLength)
                          .on_change([this](const std::string&) { error_label_->set_text(""); })
                          .on_submit([this] { submit(); })
                          .build();
    auto error = ui::label("")
                     .style(ui::theme::label_style().with_foreground(ui::theme::RedFace))
                     .build();

    ui::join_focus_group(focus_group_, *name_input);
    const ui::ButtonFactory button_factory(focus_group_);
    auto create =
        button_factory.create("Create", [this] { submit(); }, ui::theme::start_button_style());
    auto back = button_factory.create(
        "Back", [&bus] { bus.publish(game::BackToMenuRequested{}); });

    name_input_ = name_input.get();
    error_label_ = error.get();
    create_button_ = create.get();
    back_button_ = back.get();

    ui::CardBuilder builder = ui::card();
    builder.style(ui::theme::card_style().with_width(CardWidth));
    builder.child(std::move(title));
    builder.child(std::move(prompt));
    builder.child(std::move(name_input));
    builder.child(std::move(error));
    builder.child(std::move(create));
    builder.child(std::move(back));
    set_root(builder.build());
}

void NewGameView::handle_pointer(ui::Point position, bool released) {
    PanelView::handle_pointer(position, released);
    if (!released) {
        return;
    }
    // The field focuses itself when clicked and blurs when the click lands elsewhere;
    // the focus group is the one that decides, so bring both back in sync.
    if (name_input_->focused()) {
        focus_group_.focus(*name_input_);
        return;
    }
    focus_group_.refresh();
}

void NewGameView::type_char(char character) { name_input_->type_char(character); }

void NewGameView::erase_last() { name_input_->erase_last(); }

void NewGameView::submit() {
    const std::string name = game::normalize_save_name(name_input_->value());
    if (name.empty()) {
        error_label_->set_text(EmptyNameMessage);
        return;
    }
    if (!game::is_valid_save_name(name)) {
        error_label_->set_text(InvalidNameMessage);
        return;
    }
    bus_.publish(game::CreateGameRequested{name});
}

} // namespace scene::new_game
