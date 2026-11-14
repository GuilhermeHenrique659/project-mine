#include "scene/world/core/save/codecs.hpp"

#include <optional>
#include <string>

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

} // namespace world
