#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/registry.hpp"
#include "graphics/game_sprites.hpp"
#include "scene/world/core/components/building.hpp"
#include "scene/world/render/cube.hpp"

namespace scene::world {

// What the renderer needs to look an entity up this frame.
struct LookContext {
    const graphics::GameSprites& sprites;
    float seconds = 0.0f; // animation time
    float scale = 1.0f;   // times the default zoom
};

// How an entity looks this frame.
struct EntityLook {
    Cube cube;                                  // drawn when there is no sprite or it does not load
    std::optional<graphics::SpriteFrame> sprite;
    std::optional<::world::TileSize> footprint; // what it covers for draw order; none = a point
};

// How one kind of entity (a building, a worker, a character) is drawn. A new kind of entity
// is a new style added to EntityStyles; the renderer never tests entity types itself.
class EntityStyle {
public:
    virtual ~EntityStyle() = default;

    // Whether this style is the one for `id` (checked in EntityStyles order).
    virtual bool applies_to(const core::Registry& registry, core::EntityId id) const = 0;

    // Whether the entity is drawn at all this frame.
    virtual bool visible(const core::Registry&, core::EntityId) const { return true; }

    virtual EntityLook look(const core::Registry& registry, core::EntityId id,
                            const LookContext& context) const = 0;

    // The text above it; built only for entities that are on screen.
    virtual std::string label(const core::Registry& registry, const core::Entity& entity) const {
        (void)registry;
        return entity.label;
    }
};

// The style factory: gives each entity the first style that applies to it, or a plain cube
// with its label when none does.
class EntityStyles {
public:
    explicit EntityStyles(std::vector<std::unique_ptr<EntityStyle>> styles);

    const EntityStyle& style_for(const core::Registry& registry, core::EntityId id) const;

    // The game's styles: buildings, then workers, then other characters.
    static const EntityStyles& standard();

private:
    std::vector<std::unique_ptr<EntityStyle>> styles_;
};

} // namespace scene::world
