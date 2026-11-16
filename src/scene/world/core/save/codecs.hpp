#pragma once

#include "scene/world/core/components/character.hpp"
#include "scene/world/core/save/component_codec.hpp"

namespace world {

// "position.x" / "position.y".
class PositionCodec : public ComponentCodec {
public:
    void write(const core::Registry& registry, core::EntityId id, Fields& fields) const override;
    void read(core::Registry& registry, core::EntityId id, const Fields& fields) const override;
};

// The state a character is saved in: movement intent is not saved, so Moving and
// Running are written as Idle and a loaded character stands still.
CharacterState saved_state(CharacterState state);

// "state".
class CharacterCodec : public ComponentCodec {
public:
    void write(const core::Registry& registry, core::EntityId id, Fields& fields) const override;
    void read(core::Registry& registry, core::EntityId id, const Fields& fields) const override;
};

// "player" = "1".
class PlayerControlledCodec : public ComponentCodec {
public:
    void write(const core::Registry& registry, core::EntityId id, Fields& fields) const override;
    void read(core::Registry& registry, core::EntityId id, const Fields& fields) const override;
};

// "inventory.<slot>" = "<ItemType>:<count>" for each filled slot, e.g. "inventory.3" =
// "IronOre:50", so empty slots between stacks survive a load. On read, an entry with an
// unknown item, a slot outside the capacity or a count <= 0 or above the item's max stack
// is skipped.
class InventoryCodec : public ComponentCodec {
public:
    void write(const core::Registry& registry, core::EntityId id, Fields& fields) const override;
    void read(core::Registry& registry, core::EntityId id, const Fields& fields) const override;
};

// "building.type" (save key of the BuildingType), "building.x" / "building.y" (its tile).
// On read, a valid record re-places the building (position and site from the catalog).
class BuildingCodec : public ComponentCodec {
public:
    void write(const core::Registry& registry, core::EntityId id, Fields& fields) const override;
    void read(core::Registry& registry, core::EntityId id, const Fields& fields) const override;
};

// "construction.progress" while a building is unfinished. On read (buildings only), a
// missing value means it was finished, so the site is removed; the effort comes from the
// catalog, not the save.
class ConstructionSiteCodec : public ComponentCodec {
public:
    void write(const core::Registry& registry, core::EntityId id, Fields& fields) const override;
    void read(core::Registry& registry, core::EntityId id, const Fields& fields) const override;
};

// "worker.state" ("Idle" / "Working") and, by building tile, "worker.workplace.x/y",
// "worker.site.x/y" and "worker.route.x/y" + "worker.route.forward" ("1" / "0") for the
// route being walked. On read the tiles become a PendingWorkerLinks the WorkerSystem
// resolves; a missing building then leaves the worker idle.
class WorkerCodec : public ComponentCodec {
public:
    void write(const core::Registry& registry, core::EntityId id, Fields& fields) const override;
    void read(core::Registry& registry, core::EntityId id, const Fields& fields) const override;
};

} // namespace world
