#pragma once

#include <string>
#include <vector>

namespace game {

struct Resolution {
    int width = 0;
    int height = 0;

    bool operator==(const Resolution&) const = default;
};

// Window resolutions the player can pick in the settings screen.
const std::vector<Resolution>& resolution_presets();

// Text shown for a resolution, e.g. "1280 x 720".
std::string resolution_label(Resolution resolution);

struct DisplaySettings {
    Resolution resolution{1280, 720};
    bool fullscreen = false;

    bool operator==(const DisplaySettings&) const = default;
};

} // namespace game
