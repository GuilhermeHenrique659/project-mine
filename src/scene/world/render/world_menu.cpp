#include "scene/world/render/world_menu.hpp"

#include "game/game_events.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {
constexpr float CardWidth = 560.0f;
constexpr int TitleFontSize = 40;
} // namespace

WorldMenu::WorldMenu(event::EventBus& bus, const WorldModel& model,
                     std::shared_ptr<const input::Pointer> pointer)
    : model_(model), pointer_(std::move(pointer)) {
    const game::SaveGame& save = model.save_game();
    auto name = ui::label(save.name)
                    .style(ui::theme::label_style()
                               .with_font_size(TitleFontSize)
                               .with_text_align(ui::TextAlign::Center))
                    .build();
    auto created = ui::label("Created " + game::format_created_at(save.created_at))
                       .style(ui::theme::label_style()
                                  .with_foreground(ui::theme::Muted)
                                  .with_text_align(ui::TextAlign::Center))
                       .build();

    const ui::ButtonFactory button_factory(focus_group_);
    auto save_button = button_factory.create(
        "Save", [&bus, save] { bus.publish(game::SaveGameRequested{save}); },
        ui::theme::start_button_style());
    auto menu = button_factory.create(
        "Main Menu", [&bus] { bus.publish(game::BackToMenuRequested{}); });

    name_label_ = name.get();
    save_button_ = save_button.get();
    menu_button_ = menu.get();

    ui::CardBuilder builder = ui::card();
    builder.style(ui::theme::card_style().with_width(CardWidth));
    builder.child(std::move(name));
    builder.child(std::move(created));
    builder.child(std::move(save_button));
    builder.child(std::move(menu));
    root_ = builder.build();
}

void WorldMenu::layout(const ui::Canvas& canvas, ui::Size screen) const {
    const ui::Size size = root_->preferred_size(canvas);
    root_->layout({(screen.width - size.width) / 2.0f, (screen.height - size.height) / 2.0f,
                   size.width, size.height},
                  canvas);
}

void WorldMenu::draw(ui::Canvas& canvas, ui::Size screen) const {
    layout(canvas, screen);
    root_->draw(canvas);
}

bool WorldMenu::visible() const { return model_.is_paused(); }

void WorldMenu::update() {
    const input::PointerPosition at = pointer_->position();
    root_->handle_pointer({at.x, at.y}, pointer_->released());
}

} // namespace scene::world
