#pragma once

#include "bounding_box.hpp"

#include <array>

#include <glm/glm.hpp>

namespace sp::shadows {

enum frustum_corner {
   frustum_corner_bottom_left_near,
   frustum_corner_bottom_right_near,
   frustum_corner_top_left_near,
   frustum_corner_top_right_near,

   frustum_corner_bottom_left_far,
   frustum_corner_bottom_right_far,
   frustum_corner_top_left_far,
   frustum_corner_top_right_far,

   frustum_corner_COUNT
};

enum frustum_planes {
   frustum_plane_near,
   frustum_plane_far,
   frustum_plane_top,
   frustum_plane_bottom,
   frustum_plane_left,
   frustum_plane_right,

   frustum_plane_COUNT
};

struct Frustum {
   Frustum(const glm::mat4& inv_projection_matrix, const glm::vec3& ndc_min,
           const glm::vec3& ndc_max) noexcept;

   Frustum(const glm::mat4& inv_projection_matrix, const float z_min,
           const float z_max) noexcept;

   explicit Frustum(const glm::mat4& inv_projection_matrix) noexcept;

   std::array<glm::vec3, frustum_corner_COUNT> corners;
   std::array<glm::vec4, frustum_plane_COUNT> planes;
};

struct Bounding_sphere {
   glm::vec3 position;
   float radius;
};

bool intersects(const Frustum& frustum, const Bounding_box& bbox) noexcept;

bool intersects(const Frustum& frustum, const Bounding_sphere& sphere) noexcept;

}
