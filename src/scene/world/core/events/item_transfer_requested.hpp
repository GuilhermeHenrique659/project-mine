#pragma once

#include "core/entity.hpp"
#include "scene/world/core/items/item.hpp"

namespace world {

// Asks to move up to `count` of `item` from the inventory of `from` to the one of `to`
// (the inventory transfer system checks reach and room).
struct ItemTransferRequested {
    core::EntityId from = 0;
    core::EntityId to = 0;
    Item item = items::IronOre;
    int count = 0;
};

} // namespace world
