#include <gtest/gtest.h>

#include "core/registry.hpp"
#include "event/event_bus.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/components/position.hpp"
#include "scene/world/core/events/item_transfer_requested.hpp"
#include "scene/world/core/systems/inventory_transfer_system.hpp"

namespace {

struct Fixture {
    core::Registry registry;
    event::EventBus bus;
    world::InventoryTransferSystem system{registry, bus};
    core::EntityId player = registry.create("Player");
    core::EntityId mine = registry.create("Mine");

    explicit Fixture(world::WorldPoint mine_at) {
        registry.add(player, world::Position{{0.0f, 0.0f}});
        registry.add(player, world::Inventory{4});
        registry.add(mine, world::Position{mine_at});
        registry.add(mine, world::Inventory{4});
        registry.get<world::Inventory>(mine)->add(world::items::Coal, 60);
    }

    int count(core::EntityId id) const {
        return registry.get<world::Inventory>(id)->count(world::items::Coal);
    }
};

} // namespace

TEST(InventoryTransferSystem, MovesTheRequestedItemsBetweenEntitiesInReach) {
    // Given
    Fixture f({1.5f, 0.0f});

    // When
    f.bus.publish(world::ItemTransferRequested{f.mine, f.player, world::items::Coal, 60});

    // Then
    EXPECT_EQ(f.count(f.mine), 0);
    EXPECT_EQ(f.count(f.player), 60);
}

TEST(InventoryTransferSystem, IgnoresARequestBetweenEntitiesTooFarApart) {
    // Given
    Fixture f({world::TransferReach + 0.5f, 0.0f});

    // When
    f.bus.publish(world::ItemTransferRequested{f.mine, f.player, world::items::Coal, 60});

    // Then
    EXPECT_EQ(f.count(f.mine), 60);
    EXPECT_EQ(f.count(f.player), 0);
}

TEST(InventoryTransferSystem, IgnoresARequestToAnEntityWithoutAnInventory) {
    // Given
    Fixture f({1.0f, 0.0f});
    f.registry.remove<world::Inventory>(f.player);

    // When
    f.bus.publish(world::ItemTransferRequested{f.mine, f.player, world::items::Coal, 60});

    // Then
    EXPECT_EQ(f.count(f.mine), 60);
}
