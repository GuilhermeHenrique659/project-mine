#include "game/display_settings.hpp"

namespace game {

const std::vector<Resolution>& resolution_presets() {
    static const std::vector<Resolution> presets{
        {1280, 720}, {1366, 768}, {1600, 900}, {1920, 1080}};
    return presets;
}

std::string resolution_label(Resolution resolution) {
    return std::to_string(resolution.width) + " x " + std::to_string(resolution.height);
}

} // namespace game
