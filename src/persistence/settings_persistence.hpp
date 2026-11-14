#pragma once

#include "event/event_bus.hpp"
#include "persistence/settings_store.hpp"

namespace persistence {

// Saves the display settings to disk every time the player changes them.
class SettingsPersistence {
public:
    SettingsPersistence(event::EventBus& bus, SettingsStore store);

private:
    SettingsStore store_;
    event::ScopedSubscription subscription_;
};

} // namespace persistence
