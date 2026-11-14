#include "persistence/settings_persistence.hpp"

#include "game/game_events.hpp"

namespace persistence {

SettingsPersistence::SettingsPersistence(event::EventBus& bus, SettingsStore store)
    : store_(std::move(store)) {
    subscription_ = event::subscribe_scoped<game::DisplaySettingsChanged>(
        bus, [this](const game::DisplaySettingsChanged& event) { store_.save(event.settings); });
}

} // namespace persistence
