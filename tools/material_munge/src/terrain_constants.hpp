#pragma once

#include <string_view>

namespace sp {

constexpr auto terrain_max_objects = 6;
constexpr auto terrain_low_detail_length = 65;
constexpr std::string_view terrain_low_detail_suffix = "LOWD";
constexpr std::string_view terrain_material_name =
   "_SP_MATERIAL_terrain_geometry";
constexpr std::string_view terrain_model_name = "_SP_MODEL_terrain_geometry";
constexpr std::string_view terrain_world_name = "_SP_WORLD_terrain_geometry";
constexpr std::string_view terrain_height_texture_name =
   "_SP_TERRAIN_height_maps";
constexpr std::string_view terrain_albedo_ao_texture_name =
   "_SP_TERRAIN_albedo_ao_maps";
constexpr std::string_view terrain_normal_mr_texture_name =
   "_SP_TERRAIN_normal_mr_maps";
constexpr std::string_view terrain_diffuse_ao_texture_name =
   "_SP_TERRAIN_diffuse_ao_maps";
constexpr std::string_view terrain_normal_gloss_texture_name =
   "_SP_TERRAIN_normal_gloss_maps";

}
