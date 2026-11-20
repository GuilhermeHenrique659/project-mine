#include "graphics/icons.hpp"

#include <cctype>
#include <system_error>

namespace graphics {

int register_icons(TextureManager& textures, const std::filesystem::path& folder) {
    std::error_code error;
    std::filesystem::directory_iterator entries(folder, error);
    if (error) {
        return 0;
    }
    int count = 0;
    for (const auto& entry : entries) {
        if (!entry.is_regular_file() || entry.path().extension() != ".png") {
            continue;
        }
        textures.register_texture(entry.path().stem().string(), entry.path().string());
        ++count;
    }
    return count;
}

std::string item_icon_name(::world::ItemType type) {
    std::string name = "item_";
    for (const char letter : std::string_view(::world::to_string(type))) {
        name += letter == ' ' ? '_'
                              : static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
    }
    return name;
}

} // namespace graphics
