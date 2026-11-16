#pragma once

#include <optional>
#include <string_view>

namespace world {

// The buildings that exist in the simulation. One value per implemented building; the
// catalog lists others without a type until they are implemented.
enum class BuildingType { Headquarters, Mine, Storage, Steelworks, ElectronicFactory };

// Stable save-file name of a building type ("Headquarters").
std::string_view building_type_key(BuildingType type);
std::optional<BuildingType> parse_building_type(std::string_view key);

} // namespace world
