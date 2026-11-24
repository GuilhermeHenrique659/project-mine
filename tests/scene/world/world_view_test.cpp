#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "event/event_bus.hpp"
#include "input/pointer_gate.hpp"
#include "game/game_events.hpp"
#include "scene/world/render/entity_renderer.hpp"
#include "scene/world/render/world_menu.hpp"
#include "scene/world/world_view.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_input.hpp"
#include "support/fake_pointer.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};

using Calls = std::vector<std::string>;
using RendererPtr = std::unique_ptr<scene::world::Renderer>;

// Writes "update:<name>" / "draw:<name>" into a shared list.
class RecordingRenderer : public scene::world::Renderer {
public:
    RecordingRenderer(std::string name, Calls& calls, bool visible = true)
        : name_(std::move(name)), calls_(calls), visible_(visible) {}

    bool visible() const override { return visible_; }
    void update() override { calls_.push_back("update:" + name_); }
    void draw(ui::Canvas&, ui::Size) const override { calls_.push_back("draw:" + name_); }

private:
    std::string name_;
    Calls& calls_;
    bool visible_;
};

// Records whether it saw a click; optionally captures the pointer (a panel on top).
class ClickRenderer : public scene::world::Renderer {
public:
    ClickRenderer(std::shared_ptr<const input::Pointer> pointer, bool captures)
        : pointer_(std::move(pointer)), captures_(captures) {}

    bool captures_pointer(ui::Point) const override { return captures_; }
    void update() override { clicked = clicked || pointer_->released(); }
    void draw(ui::Canvas&, ui::Size) const override {}

    bool clicked = false;

private:
    std::shared_ptr<const input::Pointer> pointer_;
    bool captures_;
};

std::vector<RendererPtr> renderers_of(RendererPtr bottom, RendererPtr top) {
    std::vector<RendererPtr> renderers;
    renderers.push_back(std::move(bottom));
    renderers.push_back(std::move(top));
    return renderers;
}

// The real world screen: entities under the pause menu, with a fake mouse.
struct WorldScreen {
    WorldScreen() : model(bus, input.handler, game::SaveGame{"Alpha", 0}) {
        auto menu_renderer = std::make_unique<scene::world::WorldMenu>(bus, model, pointer);
        menu = menu_renderer.get();
        view = std::make_unique<scene::world::WorldView>(
            renderers_of(std::make_unique<scene::world::EntityRenderer>(model.registry(), model.camera()),
                         std::move(menu_renderer)));
    }

    event::EventBus bus;
    FakeInput input;
    std::shared_ptr<FakePointer> pointer = std::make_shared<FakePointer>();
    scene::world::WorldModel model;
    scene::world::WorldMenu* menu = nullptr; // owned by the view
    std::unique_ptr<scene::world::WorldView> view;
};

} // namespace

TEST(WorldView, DrawsTheRenderersInTheOrderGiven) {
    // Given
    Calls calls;
    scene::world::WorldView view(renderers_of(std::make_unique<RecordingRenderer>("bottom", calls),
                                              std::make_unique<RecordingRenderer>("top", calls)));
    FakeCanvas canvas;

    // When
    view.draw(canvas, Screen);

    // Then
    EXPECT_EQ(calls, (Calls{"draw:bottom", "draw:top"}));
}

TEST(WorldView, UpdatesTheTopmostRendererFirst) {
    // Given
    Calls calls;
    scene::world::WorldView view(renderers_of(std::make_unique<RecordingRenderer>("bottom", calls),
                                              std::make_unique<RecordingRenderer>("top", calls)));

    // When
    view.update();

    // Then
    EXPECT_EQ(calls, (Calls{"update:top", "update:bottom"}));
}

TEST(WorldView, AHiddenRendererIsNeitherUpdatedNorDrawn) {
    // Given
    Calls calls;
    scene::world::WorldView view(
        renderers_of(std::make_unique<RecordingRenderer>("shown", calls),
                     std::make_unique<RecordingRenderer>("hidden", calls, false)));
    FakeCanvas canvas;

    // When
    view.update();
    view.draw(canvas, Screen);

    // Then
    EXPECT_EQ(calls, (Calls{"update:shown", "draw:shown"}));
}

TEST(WorldView, PauseMenuIsDrawnOnlyWhilePaused) {
    // Given
    WorldScreen screen;
    FakeCanvas playing;
    FakeCanvas paused;

    // When
    screen.view->draw(playing, Screen);
    screen.model.toggle_pause();
    screen.view->draw(paused, Screen);

    // Then: while playing only the labels of the player and the starting tank are drawn
    ASSERT_EQ(playing.texts.size(), 2u);
    EXPECT_EQ(playing.texts[0].text, "Player (Idle)");
    EXPECT_EQ(playing.texts[1].text, "Battle Tank (0/6)");
    EXPECT_GT(paused.texts.size(), 1u);
}

TEST(WorldView, PauseMenuIgnoresClicksWhileThePlayIsNotPaused) {
    // Given: the menu has been laid out while paused, then the game resumes
    WorldScreen screen;
    int back_requests = 0;
    const auto subscription = event::subscribe_scoped<game::BackToMenuRequested>(
        screen.bus, [&back_requests](const game::BackToMenuRequested&) { ++back_requests; });
    FakeCanvas canvas;
    screen.model.toggle_pause();
    screen.view->draw(canvas, Screen);
    screen.model.toggle_pause();
    const ui::Rect button = screen.menu->menu_button().bounds();
    screen.pointer->at = {button.x + button.width / 2.0f, button.y + button.height / 2.0f};
    screen.pointer->release = true;

    // When
    screen.view->update();

    // Then
    EXPECT_EQ(back_requests, 0);
}

TEST(WorldView, ARendererThatCapturesThePointerKeepsTheClickFromTheOnesBelow) {
    // Given: a capturing panel above a renderer that also reads clicks
    auto mouse = std::make_shared<FakePointer>();
    mouse->release = true;
    auto gate = std::make_shared<input::PointerGate>(mouse);
    auto bottom = std::make_unique<ClickRenderer>(gate, false);
    auto top = std::make_unique<ClickRenderer>(gate, true);
    ClickRenderer& below = *bottom;
    ClickRenderer& above = *top;
    scene::world::WorldView view(renderers_of(std::move(bottom), std::move(top)), gate);

    // When
    view.update();

    // Then
    EXPECT_TRUE(above.clicked);
    EXPECT_FALSE(below.clicked);
}

TEST(WorldView, TheClickReachesTheRendererBelowANonCapturingOne) {
    // Given
    auto mouse = std::make_shared<FakePointer>();
    mouse->release = true;
    auto gate = std::make_shared<input::PointerGate>(mouse);
    auto bottom = std::make_unique<ClickRenderer>(gate, false);
    ClickRenderer& below = *bottom;
    scene::world::WorldView view(
        renderers_of(std::move(bottom), std::make_unique<ClickRenderer>(gate, false)), gate);
    gate->claim(); // left over from the previous frame

    // When
    view.update();

    // Then
    EXPECT_TRUE(below.clicked);
}
