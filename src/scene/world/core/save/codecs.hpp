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

} // namespace world
