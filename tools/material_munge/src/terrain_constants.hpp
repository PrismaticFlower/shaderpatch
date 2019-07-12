#pragma once

#include <string_view>

namespace sp {

constexpr auto terrain_max_objects = 8;
constexpr std::string_view terrain_material_name =
   "_SP_MATERIAL_terrain_geometry";
constexpr std::string_view terrain_model_name = "_SP_MODEL_terrain_geometry";
constexpr std::string_view terrain_object_name = "_SP_OBJECT_terrain_geometry";
constexpr std::string_view terrain_world_name = "_SP_WORLD_terrain_geometry";

}
