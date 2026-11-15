#include "scene/world/core/save/codecs.hpp"

#include <optional>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "scene/world/core/components/inventory.hpp"
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

} // namespace world
