#pragma once

#include "scene/world/core/systems/shoot_system.hpp"

// A shot roll that always hits, for tests that check what a hit does.
inline const ::world::HitRoll AlwaysHit = [] { return 0.0f; };
