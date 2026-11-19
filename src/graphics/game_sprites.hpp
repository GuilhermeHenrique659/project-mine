#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "graphics/sprite_library.hpp"
#include "scene/world/core/components/character.hpp"
#include "scene/world/core/components/facing.hpp"
#include "scene/world/core/construction/building_type.hpp"
#include "scene/world/core/terrain/terrain.hpp"

namespace graphics {

// Which sprite the game shows for what, found by naming convention in the library, so a
// new sprite needs no code: drop `<name>.sprite` and its sheet under resource/sprite/.
//
//   buildings/<building key in snake_case>   a finished building, clip "idle",
//                                            anchored on its footprint centre
//   environment/<tile type in snake_case>    a terrain tile, clip "idle"; each row is a
//                                            variant, picked per tile
//   units/character                          one row per world::Direction (enum order),
//                                            clips "idle", "walk", "run"
//
// Names are resolved once, in the constructor; the lookups below only index arrays.
class GameSprites {
public:
    // No sprite at all: everything keeps its plain shape.
    GameSprites() : GameSprites(SpriteLibrary{}) {}
    explicit GameSprites(SpriteLibrary library);

    // A shared instance with no sprite, for views built without a library.
    static const GameSprites& none();

    // None when the type has no sprite (it keeps its box).
    std::optional<SpriteFrame> building(::world::BuildingType type, float seconds) const;
    // None when the type has no sprite (it keeps its colour).
    std::optional<SpriteFrame> tile(::world::TileType type, ::world::TileCoord tile,
                                    float seconds) const;
    // None without a character sprite (it keeps its box).
    std::optional<SpriteFrame> character(::world::Direction facing,
                                         ::world::CharacterState state, float seconds) const;

private:
    static constexpr std::size_t BuildingTypes =
        static_cast<std::size_t>(::world::BuildingType::TrainingTarget) + 1;
    static constexpr std::size_t TileTypes = static_cast<std::size_t>(::world::TileType::Coal) + 1;
    static constexpr std::size_t CharacterStates =
        static_cast<std::size_t>(::world::CharacterState::Running) + 1;

    SpriteLibrary library_;
    std::array<std::optional<ClipHandle>, BuildingTypes> buildings_;
    std::array<std::optional<ClipHandle>, TileTypes> tiles_;
    std::array<std::optional<ClipHandle>, CharacterStates> character_;
};

// The sprite names GameSprites looks for: "buildings/electronic_factory", "environment/iron_ore".
std::string building_sprite_name(::world::BuildingType type);
std::string tile_sprite_name(::world::TileType type);
inline constexpr const char* CharacterSpriteName = "units/character";

} // namespace graphics
