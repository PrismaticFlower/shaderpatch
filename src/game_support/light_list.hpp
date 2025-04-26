#pragma once

#include <vector>

#include <glm/glm.hpp>

namespace sp::game_support {

struct Point_light {
   glm::vec3 positionWS;
   float inv_range_sq;
   glm::vec3 color;
};

/// @brief Acquire a list of light entities from the game. This directly reads the game's data structures and is **NOT** thread safe.
/// @param point_lights The output list of point lights.
void acquire_light_list(std::vector<Point_light>& point_lights) noexcept;

/// @brief Show a window that allows exploring the game's lights. This directly reads the game's data structures and is **NOT** thread safe.
void show_light_list_imgui() noexcept;

}
