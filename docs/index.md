# Documentation

Reference material for project-mine. The rules every change must follow (build, tests, coding standards) stay in [AGENTS.md](../AGENTS.md); these pages describe what the code does and how it is organised.

| Page | Contents |
|---|---|
| [gameplay.md](gameplay.md) | What the game does today, from the player's point of view |
| [architecture.md](architecture.md) | Module map, how the layers communicate, adding a screen |
| [world-scene.md](world-scene.md) | `scene/world/`: `WorldModel`, `WorldView` and the renderers |
| [world-logic.md](world-logic.md) | `scene/world/core/` (`world::`): components, systems, terrain, construction, isometric math |
| [persistence.md](persistence.md) | Settings, saves, data directory, entity serialization |
| [engine-modules.md](engine-modules.md) | `core/` ECS, `event/`, `ui/`, `input/`, `graphics/`, `logging/`, `text/` |
| [testing.md](testing.md) | Test layout and shared helpers |

Keep these pages up to date when the behaviour they describe changes.
