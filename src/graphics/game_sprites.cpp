#include "graphics/game_sprites.hpp"

#include <cctype>
#include <cstdint>

namespace graphics {

namespace {

// "ElectronicFactory" -> "electronic_factory"
std::string snake_case(std::string_view name) {
    std::string result;
    for (const char letter : name) {
        const auto byte = static_cast<unsigned char>(letter);
        if (std::isupper(byte) && !result.empty()) {
            result += '_';
        }
        result += static_cast<char>(std::tolower(byte));
    }
    return result;
}

// A well-mixed hash of the tile, so variants do not form visible patterns.
int tile_variant(::world::TileCoord tile, int variants) {
    std::uint32_t h = static_cast<std::uint32_t>(tile.x) * 73856093u ^
                      static_cast<std::uint32_t>(tile.y) * 19349663u;
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    h ^= h >> 15;
    return static_cast<int>(h % static_cast<std::uint32_t>(variants));
}

constexpr const char* IdleClip = "idle";
constexpr const char* WalkClip = "walk";
constexpr const char* RunClip = "run";
constexpr const char* DriveClip = "drive";

} // namespace

std::string building_sprite_name(::world::BuildingType type) {
    return "buildings/" + snake_case(::world::building_type_key(type));
}

std::string tile_sprite_name(::world::TileType type) {
    switch (type) {
    case ::world::TileType::Water: return "environment/water";
    case ::world::TileType::IronOre: return "environment/iron_ore";
    case ::world::TileType::CopperOre: return "environment/copper_ore";
    case ::world::TileType::Coal: return "environment/coal";
    case ::world::TileType::CrudeOil: return "environment/crude_oil";
    case ::world::TileType::Grass: break;
    }
    return "environment/grass";
}

GameSprites::GameSprites(SpriteLibrary library) : library_(std::move(library)) {
    const auto idle_of = [this](const std::string& name) -> std::optional<ClipHandle> {
        const auto sprite = library_.find(name);
        if (!sprite) {
            return std::nullopt;
        }
        return library_.clip(*sprite, IdleClip);
    };
    for (std::size_t type = 0; type < BuildingTypes; ++type) {
        buildings_[type] = idle_of(building_sprite_name(static_cast<::world::BuildingType>(type)));
    }
    for (std::size_t type = 0; type < TileTypes; ++type) {
        tiles_[type] = idle_of(tile_sprite_name(static_cast<::world::TileType>(type)));
    }

    if (const auto tank = library_.find(BattleTankSpriteName)) {
        tank_idle_ = library_.clip(*tank, IdleClip);
        // Without a drive clip it glides parked.
        tank_drive_ = library_.has_clip(*tank, DriveClip) ? library_.clip(*tank, DriveClip)
                                                          : *tank_idle_;
    }

    const auto character = library_.find(CharacterSpriteName);
    if (!character) {
        return;
    }
    const ClipHandle walk = library_.clip(*character, WalkClip);
    character_[static_cast<std::size_t>(::world::CharacterState::Idle)] =
        library_.clip(*character, IdleClip);
    character_[static_cast<std::size_t>(::world::CharacterState::Moving)] = walk;
    // Without a run clip the character runs with the walk cycle.
    character_[static_cast<std::size_t>(::world::CharacterState::Running)] =
        library_.has_clip(*character, RunClip) ? library_.clip(*character, RunClip) : walk;
}

const GameSprites& GameSprites::none() {
    static const GameSprites empty;
    return empty;
}

std::optional<SpriteFrame> GameSprites::building(::world::BuildingType type,
                                                 float seconds) const {
    const auto& clip = buildings_[static_cast<std::size_t>(type)];
    if (!clip) {
        return std::nullopt;
    }
    return library_.frame(*clip, 0, seconds);
}

std::optional<SpriteFrame> GameSprites::tile(::world::TileType type, ::world::TileCoord tile,
                                             float seconds) const {
    const auto& clip = tiles_[static_cast<std::size_t>(type)];
    if (!clip) {
        return std::nullopt;
    }
    const int variants = library_.rows(clip->sprite);
    return library_.frame(*clip, variants > 1 ? tile_variant(tile, variants) : 0, seconds);
}

std::optional<SpriteFrame> GameSprites::character(::world::Direction facing,
                                                  ::world::CharacterState state,
                                                  float seconds) const {
    const auto& clip = character_[static_cast<std::size_t>(state)];
    if (!clip) {
        return std::nullopt;
    }
    return library_.frame(*clip, static_cast<int>(facing), seconds);
}

std::optional<SpriteFrame> GameSprites::battle_tank(::world::Direction facing,
                                                    ::world::CharacterState state,
                                                    float seconds) const {
    const auto& clip = state == ::world::CharacterState::Idle ? tank_idle_ : tank_drive_;
    if (!clip) {
        return std::nullopt;
    }
    return library_.frame(*clip, static_cast<int>(facing), seconds);
}

} // namespace graphics
