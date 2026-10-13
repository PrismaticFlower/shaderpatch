#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

namespace sp::game_support {

struct Light_options {
   std::uint32_t has_texture : 1 = 0;
   std::uint32_t texture_index : 22 = 0;
   std::uint32_t texture_srv_index : 2 = 0;
};

struct Point_light {
   glm::vec3 positionWS;
   float inv_range_sq;
   glm::vec3 color;
   float range;

   Light_options options;
};

struct Spot_light {
   glm::vec3 positionWS;
   float inv_range_sq;
   glm::vec3 directionWS;
   float range;
   glm::vec3 color;
   float cos_half_outer_angle;
   float cos_half_inner_angle;
   float outer_angle;
   float cone_radius;

   Light_options options;
};

struct Light_texture {
   std::uint32_t name_hash;
   glm::mat4 matrix;
};

/// @brief Acquire the directions of the global lights from the game.
/// @param global_light1_dir The output for the direction of the first global light.
/// @param global_light2_dir The output for the direction of the second global light.
void acquire_global_lights(glm::vec3& global_light1_dir,
                           glm::vec3& global_light2_dir) noexcept;

/// @brief Acquire a list of light entities from the game. This directly reads the game's data structures and is **NOT** thread safe.
/// @param point_lights The output list of point lights.
/// @param spot_lights The output list of spot lights.
/// @param textures The output list of textures.
void acquire_light_list(std::vector<Point_light>& point_lights,
                        std::vector<Spot_light>& spot_lights,
                        std::vector<Light_texture>& textures) noexcept;

/// @brief Show a window that allows exploring the game's lights. This directly reads the game's data structures and is **NOT** thread safe.
void show_light_list_imgui() noexcept;

}
