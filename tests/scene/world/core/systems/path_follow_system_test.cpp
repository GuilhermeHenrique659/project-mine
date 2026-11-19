#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

#include "core/registry.hpp"
#include "scene/world/core/components/mover.hpp"
#include "scene/world/core/components/path_follower.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/systems/path_follow_system.hpp"

namespace {

constexpr float Frame = 0.05f;

struct Fixture {
    core::Registry registry;
    world::PathFollowSystem system{registry};

    core::EntityId add_walker(world::WorldPoint at, std::vector<world::TileCoord> tiles) {
        const core::EntityId id = registry.create("walker");
        registry.add(id, world::Position{at});
        registry.add(id, world::Mover{2.0f, {}, false});
        registry.add(id, world::PathFollower{std::move(tiles)});
        return id;
    }

    // Walks every follower along its direction, as CharacterMoveSystem would.
    void run(int frames) {
        for (int i = 0; i < frames; ++i) {
            system.update(Frame);
            registry.each<world::Position, world::Mover>(
                [](core::EntityId, world::Position& position, world::Mover& mover) {
                    const float length = std::hypot(mover.direction.x, mover.direction.y);
                    if (length == 0.0f) {
                        return;
                    }
                    const float step = std::min(mover.speed * Frame, length);
                    position.point.x += mover.direction.x / length * step;
                    position.point.y += mover.direction.y / length * step;
                });
        }
    }
};

} // namespace

TEST(PathFollowSystem, AFollowerHeadsForTheNextTileCentre) {
    // Given: standing on the centre of the first tile
    Fixture f;
    const core::EntityId id = f.add_walker({0.5f, 0.5f}, {{0, 0}, {1, 0}});

    // When
    f.system.update(Frame);

    // Then
    const world::WorldPoint direction = f.registry.get<world::Mover>(id)->direction;
    EXPECT_GT(direction.x, 0.0f);
    EXPECT_FLOAT_EQ(direction.y, 0.0f);
}

TEST(PathFollowSystem, AFollowerStopsExactlyOnTheLastTileAndDropsThePath) {
    // Given
    Fixture f;
    const core::EntityId id = f.add_walker({0.5f, 0.5f}, {{0, 0}, {1, 0}, {2, 1}});

    // When
    f.run(100);

    // Then
    const world::WorldPoint at = f.registry.get<world::Position>(id)->point;
    EXPECT_FLOAT_EQ(at.x, 2.5f);
    EXPECT_FLOAT_EQ(at.y, 1.5f);
    EXPECT_FALSE(f.registry.has<world::PathFollower>(id));
    EXPECT_FLOAT_EQ(f.registry.get<world::Mover>(id)->direction.x, 0.0f);
}
