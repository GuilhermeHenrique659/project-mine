#include "scene/world/core/save/codecs.hpp"

#include <optional>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "scene/world/core/components/building.hpp"
#include "scene/world/core/components/inventory.hpp"
#include "scene/world/core/entities/building.hpp"
#include "scene/world/core/components/player_controlled.hpp"
#include "scene/world/core/components/position.hpp"
#include "text/parse.hpp"

namespace world {

namespace {

// Empty when the key is missing or its value is not exactly one float.
std::optional<float> parse_float(const Fields& fields, const std::string& key) {
    const auto found = fields.find(key);
    if (found == fields.end()) {
        return std::nullopt;
    }
    return text::parse<float>(found->second);
}

CharacterState parse_state(const std::string& text) {
    if (text == to_string(CharacterState::Moving)) {
        return CharacterState::Moving;
    }
    if (text == to_string(CharacterState::Running)) {
        return CharacterState::Running;
    }
    return CharacterState::Idle;
}

constexpr std::string_view InventoryPrefix = "inventory.";

// Save-file names of the item types: stable keys, not the names shown to the player.
constexpr std::array<std::pair<ItemType, std::string_view>, 3> ItemKeys{{
    {ItemType::IronOre, "IronOre"},
    {ItemType::CopperOre, "CopperOre"},
    {ItemType::Coal, "Coal"},
}};

std::string_view item_key(ItemType type) {
    for (const auto& [key_type, key] : ItemKeys) {
        if (key_type == type) {
            return key;
        }
    }
    return {};
}

std::optional<ItemType> parse_item_key(std::string_view text) {
    for (const auto& [type, key] : ItemKeys) {
        if (key == text) {
            return type;
        }
    }
    return std::nullopt;
}

// "<ItemType>:<count>", empty when the type is unknown or the count is not a valid
// stack of that item.
std::optional<ItemStack> parse_stack(std::string_view text) {
    const auto colon = text.find(':');
    if (colon == std::string_view::npos) {
        return std::nullopt;
    }
    const auto type = parse_item_key(text.substr(0, colon));
    const auto count = text::parse<int>(text.substr(colon + 1));
    if (!type || !count) {
        return std::nullopt;
    }
    const Item item = item_of(*type);
    if (*count <= 0 || *count > item.max_stack) {
        return std::nullopt;
    }
    return ItemStack{item, *count};
}

} // namespace

void PositionCodec::write(const core::Registry& registry, core::EntityId id,
                          Fields& fields) const {
    const Position* position = registry.get<Position>(id);
    if (position == nullptr) {
        return;
    }
    fields["position.x"] = text::format(position->point.x);
    fields["position.y"] = text::format(position->point.y);
}

void PositionCodec::read(core::Registry& registry, core::EntityId id,
                         const Fields& fields) const {
    Position* position = registry.get<Position>(id);
    if (position == nullptr) {
        return;
    }
    const auto x = parse_float(fields, "position.x");
    const auto y = parse_float(fields, "position.y");
    if (!x || !y) {
        return;
    }
    position->point = WorldPoint{*x, *y};
}

CharacterState saved_state(CharacterState state) {
    switch (state) {
        case CharacterState::Moving:
        case CharacterState::Running: return CharacterState::Idle;
        case CharacterState::Idle: break;
    }
    return state;
}

void CharacterCodec::write(const core::Registry& registry, core::EntityId id,
                           Fields& fields) const {
    const Character* character = registry.get<Character>(id);
    if (character == nullptr) {
        return;
    }
    fields["state"] = to_string(saved_state(character->state));
}

void CharacterCodec::read(core::Registry& registry, core::EntityId id,
                          const Fields& fields) const {
    Character* character = registry.get<Character>(id);
    const auto found = fields.find("state");
    if (character == nullptr || found == fields.end()) {
        return;
    }
    // The same rule as on write: movement intent is not saved, so nobody loads mid-stride.
    character->state = saved_state(parse_state(found->second));
}

void PlayerControlledCodec::write(const core::Registry& registry, core::EntityId id,
                                  Fields& fields) const {
    if (registry.has<PlayerControlled>(id)) {
        fields["player"] = "1";
    }
}

void PlayerControlledCodec::read(core::Registry& registry, core::EntityId id,
                                 const Fields& fields) const {
    const auto found = fields.find("player");
    if (found == fields.end() || found->second != "1") {
        return;
    }
    registry.add(id, PlayerControlled{});
}

void InventoryCodec::write(const core::Registry& registry, core::EntityId id,
                           Fields& fields) const {
    const Inventory* inventory = registry.get<Inventory>(id);
    if (inventory == nullptr) {
        return;
    }
    const auto& slots = inventory->slots();
    for (std::size_t index = 0; index < slots.size(); ++index) {
        if (!slots[index]) {
            continue;
        }
        fields[std::string(InventoryPrefix) + text::format(index)] =
            std::string(item_key(slots[index]->item.type)) + ":" + text::format(slots[index]->count);
    }
}

void InventoryCodec::read(core::Registry& registry, core::EntityId id,
                          const Fields& fields) const {
    Inventory* inventory = registry.get<Inventory>(id);
    if (inventory == nullptr) {
        return;
    }
    for (const auto& [key, value] : fields) {
        if (!key.starts_with(InventoryPrefix)) {
            continue;
        }
        const auto index = text::parse<std::size_t>(key.substr(InventoryPrefix.size()));
        const auto stack = parse_stack(value);
        if (!index || *index >= inventory->capacity() || !stack) {
            continue;
        }
        inventory->set_slot(*index, stack);
    }
}

void BuildingCodec::write(const core::Registry& registry, core::EntityId id,
                          Fields& fields) const {
    const Building* building = registry.get<Building>(id);
    if (building == nullptr) {
        return;
    }
    fields["building.type"] = std::string(building_type_key(building->type));
    fields["building.x"] = text::format(building->tile.x);
    fields["building.y"] = text::format(building->tile.y);
}

void BuildingCodec::read(core::Registry& registry, core::EntityId id,
                         const Fields& fields) const {
    const auto type_field = fields.find("building.type");
    const auto x_field = fields.find("building.x");
    const auto y_field = fields.find("building.y");
    if (!registry.has<Building>(id) || type_field == fields.end() || x_field == fields.end() ||
        y_field == fields.end()) {
        return;
    }
    const auto type = parse_building_type(type_field->second);
    const auto x = text::parse<int>(x_field->second);
    const auto y = text::parse<int>(y_field->second);
    if (!type || !x || !y) {
        return;
    }
    make_building(registry, id, *find_construction(*type), TileCoord{*x, *y});
}

void ConstructionSiteCodec::write(const core::Registry& registry, core::EntityId id,
                                  Fields& fields) const {
    const ConstructionSite* site = registry.get<ConstructionSite>(id);
    if (site == nullptr) {
        return;
    }
    fields["construction.progress"] = text::format(site->progress);
}

void ConstructionSiteCodec::read(core::Registry& registry, core::EntityId id,
                                 const Fields& fields) const {
    if (!registry.has<Building>(id)) {
        return;
    }
    const auto progress = parse_float(fields, "construction.progress");
    if (!progress) {
        registry.remove<ConstructionSite>(id);
        return;
    }
    if (ConstructionSite* site = registry.get<ConstructionSite>(id)) {
        site->progress = *progress;
    }
}

} // namespace world
