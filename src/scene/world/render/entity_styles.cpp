#include "scene/world/render/entity_styles.hpp"

#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/facing.hpp"
#include "scene/world/core/components/vehicle.hpp"
#include "scene/world/core/components/worker.hpp"
#include "scene/world/core/worker_report.hpp"
#include "scene/world/core/worker_type.hpp"

namespace scene::world {

namespace {

// 24 px at the default zoom: a unit's body, as wide as its collision.
constexpr float UnitCubeSize = ::world::CharacterSize * BuildingCubeSize;
const ui::Color BuildingFront{70, 110, 170, 255};
const ui::Color BuildingTop{120, 160, 220, 255};
const ui::Color SiteFront{120, 120, 120, 255};
const ui::Color SiteTop{170, 170, 170, 255};
const ui::Color VehicleFront{70, 90, 55, 255};
const ui::Color VehicleTop{110, 135, 85, 255};
// A vehicle fills most of its tile.
constexpr float VehicleCubeSize = 0.8f * BuildingCubeSize;

// Anything without a style of its own: a small cube with its label.
class PlainStyle : public EntityStyle {
public:
    bool applies_to(const core::Registry&, core::EntityId) const override { return true; }

    EntityLook look(const core::Registry&, core::EntityId,
                    const LookContext& context) const override {
        return {Cube(UnitCubeSize * context.scale), std::nullopt, std::nullopt};
    }
};

// A building covers its whole footprint: its sprite once finished, else a box, blue when
// finished and grey with its progress while a construction site ("Mine (25%)").
class BuildingStyle : public EntityStyle {
public:
    bool applies_to(const core::Registry& registry, core::EntityId id) const override {
        return registry.has<::world::Building>(id);
    }

    EntityLook look(const core::Registry& registry, core::EntityId id,
                    const LookContext& context) const override {
        const auto& building = *registry.get<::world::Building>(id);
        const float tile = BuildingCubeSize * context.scale;
        if (registry.has<::world::ConstructionSite>(id)) {
            return {Cube::footprint(tile, building.size, SiteFront, SiteTop), std::nullopt,
                    building.size};
        }
        return {Cube::footprint(tile, building.size, BuildingFront, BuildingTop),
                context.sprites.building(building.type, context.seconds), building.size};
    }

    std::string label(const core::Registry& registry, const core::Entity& entity) const override {
        const auto* site = registry.get<::world::ConstructionSite>(entity.id);
        if (site == nullptr || site->effort <= 0.0f) {
            return entity.label;
        }
        const int percent = static_cast<int>(100.0f * site->progress / site->effort);
        return entity.label + " (" + std::to_string(percent) + "%)";
    }
};

// A character: the frame of its facing and state, labelled with its state ("Player (Moving)").
class CharacterStyle : public EntityStyle {
public:
    bool applies_to(const core::Registry& registry, core::EntityId id) const override {
        return registry.has<::world::Character>(id);
    }

    EntityLook look(const core::Registry& registry, core::EntityId id,
                    const LookContext& context) const override {
        const auto& character = *registry.get<::world::Character>(id);
        const auto* facing = registry.get<::world::Facing>(id);
        return {Cube(UnitCubeSize * context.scale),
                context.sprites.character(
                    facing != nullptr ? facing->direction : ::world::Facing{}.direction,
                    character.state, context.seconds),
                std::nullopt};
    }

    // A passenger is inside its vehicle.
    bool visible(const core::Registry& registry, core::EntityId id) const override {
        return !registry.has<::world::Passenger>(id);
    }

    std::string label(const core::Registry& registry, const core::Entity& entity) const override {
        return entity.label + " (" +
               ::world::to_string(registry.get<::world::Character>(entity.id)->state) + ")";
    }
};

// A vehicle: an olive box, labelled with how many seats are taken ("Battle Tank (2/6)").
class VehicleStyle : public EntityStyle {
public:
    bool applies_to(const core::Registry& registry, core::EntityId id) const override {
        return registry.has<::world::Vehicle>(id);
    }

    EntityLook look(const core::Registry&, core::EntityId,
                    const LookContext& context) const override {
        return {Cube(VehicleCubeSize * context.scale, VehicleFront, VehicleTop), std::nullopt,
                std::nullopt};
    }

    std::string label(const core::Registry& registry, const core::Entity& entity) const override {
        const auto& vehicle = *registry.get<::world::Vehicle>(entity.id);
        return entity.label + " (" + std::to_string(vehicle.passengers.size()) + "/" +
               std::to_string(vehicle.seats) + ")";
    }
};

// A worker looks like a character but shows the work it does ("Worker (Miner)"), and is
// not drawn while at work inside its building.
class WorkerStyle : public CharacterStyle {
public:
    bool applies_to(const core::Registry& registry, core::EntityId id) const override {
        return registry.has<::world::Worker>(id) && registry.has<::world::Character>(id);
    }

    bool visible(const core::Registry& registry, core::EntityId id) const override {
        return !::world::is_at_work(registry, id) && CharacterStyle::visible(registry, id);
    }

    std::string label(const core::Registry& registry, const core::Entity& entity) const override {
        return entity.label + " (" +
               ::world::to_string(::world::worker_type(registry, entity.id)) + ")";
    }
};

} // namespace

EntityStyles::EntityStyles(std::vector<std::unique_ptr<EntityStyle>> styles)
    : styles_(std::move(styles)) {}

const EntityStyle& EntityStyles::style_for(const core::Registry& registry,
                                           core::EntityId id) const {
    for (const auto& style : styles_) {
        if (style->applies_to(registry, id)) {
            return *style;
        }
    }
    static const PlainStyle plain;
    return plain;
}

const EntityStyles& EntityStyles::standard() {
    static const EntityStyles styles = [] {
        std::vector<std::unique_ptr<EntityStyle>> list;
        // Most specific first: a worker and a vehicle are also characters.
        list.push_back(std::make_unique<BuildingStyle>());
        list.push_back(std::make_unique<VehicleStyle>());
        list.push_back(std::make_unique<WorkerStyle>());
        list.push_back(std::make_unique<CharacterStyle>());
        return EntityStyles(std::move(list));
    }();
    return styles;
}

} // namespace scene::world
