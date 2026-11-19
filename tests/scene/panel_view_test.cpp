#include <gtest/gtest.h>

#include <memory>

#include "input/input_map.hpp"
#include "scene/panel_view.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_keyboard.hpp"
#include "support/fake_pointer.hpp"
#include "ui/button.hpp"
#include "ui/card.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

// A panel with one button that counts its clicks.
class OneButton : public scene::PanelView {
public:
    OneButton() {
        auto button = ui::button("Go").on_click([this] { ++clicks; }).build();
        go = button.get();
        set_root(ui::card().child(std::move(button)).build());
    }

    ui::Button* go = nullptr;
    int clicks = 0;
};

ui::Point centre_of(const ui::Rect& rect) {
    return {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
}

} // namespace

TEST(PanelView, TheCardSitsCentredNearTheTop) {
    // Given
    OneButton view;
    FakeCanvas canvas;

    // When
    view.layout(canvas, Screen.width);

    // Then
    const ui::Rect card = view.root().bounds();
    EXPECT_FLOAT_EQ(card.y, scene::PanelView::CardTop);
    EXPECT_NEAR(card.x + card.width / 2.0f, Screen.width / 2.0f, 0.5f);
}

TEST(PanelController, AClickFromTheInjectedPointerReachesTheView) {
    // Given
    OneButton view;
    FakeCanvas canvas;
    view.draw(canvas, Screen);
    auto pointer = std::make_shared<FakePointer>();
    const ui::Point at = centre_of(view.go->bounds());
    pointer->at = {at.x, at.y};
    pointer->release = true;
    scene::PanelController controller(view, std::make_unique<FakeKeyboard>(),
                                      input::InputMap::menu_defaults(), pointer);

    // When
    controller.handle_input();

    // Then
    EXPECT_EQ(view.clicks, 1);
}

TEST(PanelController, CancelRunsTheCancelHandler) {
    // Given
    OneButton view;
    auto owned = std::make_unique<FakeKeyboard>();
    FakeKeyboard& keyboard = *owned;
    scene::PanelController controller(view, std::move(owned), input::InputMap::menu_defaults(),
                                      std::make_shared<FakePointer>());
    int cancels = 0;
    controller.on_cancel([&cancels] { ++cancels; });

    // When
    keyboard.pressed = {input::Key::Escape};
    controller.handle_input();

    // Then
    EXPECT_EQ(cancels, 1);
}
