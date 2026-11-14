#include "scene/world/world_view.hpp"

#include <raylib.h>

#include <algorithm>
#include <string>
#include <vector>

#include "game/game_events.hpp"
#include "ui/theme.hpp"
#include "world/components/character.hpp"
#include "world/components/position.hpp"
#include "world/isometric.hpp"

namespace scene::world {

namespace {
constexpr float CardWidth = 560.0f;
constexpr float CardTop = 80.0f;
constexpr int TitleFontSize = 40;
constexpr float MarkerSize = 24.0f;
constexpr float CubeTopHeight = 10.0f;
constexpr float LabelGap = 4.0f;
constexpr int LabelFontSize = 16;
constexpr ui::Color CubeFrontColor{200, 160, 50, 255};
constexpr ui::Color CubeTopColor{240, 210, 110, 255};
} // namespace

WorldView::WorldView(event::EventBus& bus, const WorldModel& model) : model_(model) {
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

void WorldView::draw_entities(ui::Canvas& canvas, ui::Size screen) const {
    ::world::IsoProjection projection;
    projection.origin = {screen.width / 2.0f, screen.height / 2.0f};

    // Back to front: entities further down the screen are drawn later.
    struct Marker {
        ::world::WorldPoint point;
        std::string label;
    };
    std::vector<Marker> markers;
    for (const core::Entity& entity : model_.registry().entities()) {
        const auto* position = model_.registry().get<::world::Position>(entity.id);
        if (position == nullptr) {
            continue;
        }
        // Characters show their state next to the label: "Player (Moving)".
        std::string label = entity.label;
        if (const auto* character = model_.registry().get<::world::Character>(entity.id)) {
            label += std::string(" (") + ::world::to_string(character->state) + ")";
        }
        markers.push_back({position->point, std::move(label)});
    }
    std::sort(markers.begin(), markers.end(), [](const Marker& a, const Marker& b) {
        return ::world::depth_of(a.point) < ::world::depth_of(b.point);
    });

    for (const Marker& marker : markers) {
        const ::world::ScreenPoint center = projection.world_to_screen(marker.point);
        // A cube seen from the front: a lighter top face over the front face, which is
        // centred on the entity's position.
        const ui::Rect front{center.x - MarkerSize / 2.0f, center.y - MarkerSize / 2.0f,
                             MarkerSize, MarkerSize};
        const ui::Rect top{front.x, front.y - CubeTopHeight, MarkerSize, CubeTopHeight};
        canvas.fill_rect(top, CubeTopColor);
        canvas.fill_rect(front, CubeFrontColor);
        const ui::Size text = canvas.measure_text(marker.label, LabelFontSize);
        canvas.draw_text(marker.label,
                         {center.x - text.width / 2.0f, top.y - LabelGap - text.height},
                         LabelFontSize, ui::theme::Muted);
    }
}

void WorldView::draw() const {
    ui::RaylibCanvas canvas;
    draw_entities(canvas, {static_cast<float>(GetScreenWidth()),
                           static_cast<float>(GetScreenHeight())});
    layout(canvas, static_cast<float>(GetScreenWidth()));
    root_->draw(canvas);
}

void WorldView::handle_pointer(ui::Point position, bool released) {
    root_->handle_pointer(position, released);
}

} // namespace scene::world
