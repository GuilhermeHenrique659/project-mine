#include "scene/world/world_view.hpp"

#include <raylib.h>

#include "game/game_events.hpp"
#include "ui/theme.hpp"

namespace scene::world {

namespace {
constexpr float CardWidth = 560.0f;
constexpr float CardTop = 80.0f;
constexpr int TitleFontSize = 40;
} // namespace

WorldView::WorldView(event::EventBus& bus, const WorldModel& model) {
    auto name = ui::label(model.save_game().name)
                    .style(ui::theme::label_style()
                               .with_font_size(TitleFontSize)
                               .with_text_align(ui::TextAlign::Center))
                    .build();
    auto created =
        ui::label("Created " + game::format_created_at(model.save_game().created_at))
            .style(ui::theme::label_style()
                       .with_foreground(ui::theme::Muted)
                       .with_text_align(ui::TextAlign::Center))
            .build();

    const ui::ButtonFactory button_factory(focus_group_);
    auto save = button_factory.create(
        "Save", [&bus, &model] { bus.publish(game::SaveGameRequested{model.save_game()}); },
        ui::theme::start_button_style());
    auto menu = button_factory.create(
        "Main Menu", [&bus] { bus.publish(game::BackToMenuRequested{}); });

    name_label_ = name.get();
    save_button_ = save.get();
    menu_button_ = menu.get();

    ui::CardBuilder builder = ui::card();
    builder.style(ui::theme::card_style().with_width(CardWidth));
    builder.child(std::move(name));
    builder.child(std::move(created));
    builder.child(std::move(save));
    builder.child(std::move(menu));
    root_ = builder.build();
}

void WorldView::layout(const ui::Canvas& canvas, float screen_width) const {
    const ui::Size size = root_->preferred_size(canvas);
    root_->layout({(screen_width - size.width) / 2.0f, CardTop, size.width, size.height}, canvas);
}

void WorldView::draw() const {
    ui::RaylibCanvas canvas;
    layout(canvas, static_cast<float>(GetScreenWidth()));
    root_->draw(canvas);
}

void WorldView::handle_pointer(ui::Point position, bool released) {
    root_->handle_pointer(position, released);
}

} // namespace scene::world
