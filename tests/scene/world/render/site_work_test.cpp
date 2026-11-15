#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include "event/event_bus.hpp"
#include "scene/world/core/components/builder.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/render/site_work.hpp"
#include "support/fake_canvas.hpp"
#include "support/fake_input.hpp"
#include "support/fake_pointer.hpp"

namespace {

constexpr ui::Size Screen{1280.0f, 720.0f};
constexpr ::world::TileCoord SiteTile{1, 1}; // grass, in reach of the player at (0, 0)

// A save holding an unfinished mine on SiteTile (the player spawns at the origin).
game::SaveGame save_with_site() {
    game::SaveGame save{"Alpha", 0};
    save.entities.push_back({"building", "Mine",
                             {{"building.type", "Mine"},
                              {"building.x", "1"},
                              {"building.y", "1"},
                              {"construction.progress", "0"}}});
    return save;
}

class SiteWork : public testing::Test {
protected:
    SiteWork() : model(bus, input.handler, save_with_site()), work(model, pointer) {
        site = model.registry().entities().front().id;
        work.draw(canvas, Screen);
        const ::world::ScreenPoint over =
            model.camera().projection(Screen.width, Screen.height).tile_center(SiteTile);
        pointer->at = {over.x, over.y};
    }

    core::EntityId working_on() const {
        return model.registry().get<::world::Builder>(model.player_id())->site;
    }

    event::EventBus bus;
    FakeInput input;
    FakeCanvas canvas;
    std::shared_ptr<FakePointer> pointer = std::make_shared<FakePointer>();
    scene::world::WorldModel model;
    scene::world::SiteWork work;
    core::EntityId site = 0;
};

} // namespace

TEST_F(SiteWork, HoldingTheButtonOverASiteMakesThePlayerWorkOnIt) {
    // Given
    pointer->held = true;

    // When
    work.update();

    // Then
    EXPECT_EQ(working_on(), site);
}

TEST_F(SiteWork, ReleasingTheButtonStopsTheWork) {
    // Given
    pointer->held = true;
    work.update();

    // When
    pointer->held = false;
    work.update();

    // Then
    EXPECT_EQ(working_on(), 0u);
}

TEST_F(SiteWork, PausingStopsTheWorkAndHidesIt) {
    // Given
    pointer->held = true;
    work.update();

    // When
    model.toggle_pause();

    // Then
    EXPECT_FALSE(work.visible());
    EXPECT_EQ(working_on(), 0u);
}
