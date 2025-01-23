
#include "frustum.hpp"

#include <functional>

#include <glm/glm.hpp>

namespace sp::shadows {

namespace {

auto make_plane(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) noexcept
   -> glm::vec4
{
   const glm::vec3 edge0 = v1 - v0;
   const glm::vec3 edge1 = v2 - v0;
   const glm::vec3 normal = normalize(cross(edge0, edge1));

   return {normal, -dot(normal, v0)};
}

bool outside_plane(const glm::vec4& plane, const glm::vec3& point) noexcept
{
   return dot(plane, glm::vec4{point, 1.0f}) < 0.0f;
}

bool outside_plane(const glm::vec4& plane, const glm::vec3& point, const float radius) noexcept
{
   return dot(plane, glm::vec4{point, 1.0f}) < -radius;
}

}

Frustum::Frustum(const glm::mat4& inv_projection_matrix,
                 const glm::vec3& ndc_min, const glm::vec3& ndc_max) noexcept
{
   std::array<glm::vec4, frustum_corner_COUNT> corners_proj;

   corners_proj[frustum_corner_bottom_left_near] = {ndc_min.x, ndc_min.y,
                                                    ndc_min.z, 1.0f};
   corners_proj[frustum_corner_bottom_right_near] = {ndc_max.x, ndc_min.y,
                                                     ndc_min.z, 1.0f};

   corners_proj[frustum_corner_top_left_near] = {ndc_min.x, ndc_max.y, ndc_min.z, 1.0f};
   corners_proj[frustum_corner_top_right_near] = {ndc_max.x, ndc_max.y, ndc_min.z, 1.0f};

   corners_proj[frustum_corner_bottom_left_far] = {ndc_min.x, ndc_min.y,
                                                   ndc_max.z, 1.0f};
   corners_proj[frustum_corner_bottom_right_far] = {ndc_max.x, ndc_min.y,
                                                    ndc_max.z, 1.0f};

   corners_proj[frustum_corner_top_left_far] = {ndc_min.x, ndc_max.y, ndc_max.z, 1.0f};
   corners_proj[frustum_corner_top_right_far] = {ndc_max.x, ndc_max.y, ndc_max.z, 1.0f};

   for (std::size_t i = 0; i < corners.size(); ++i) {
      const glm::vec4 position = corners_proj[i] * inv_projection_matrix;

      corners[i] = glm::vec3{position.x, position.y, position.z} / position.w;
   }

   planes[frustum_plane_near] = make_plane(corners[frustum_corner_top_left_near],
                                           corners[frustum_corner_top_right_near],
                                           corners[frustum_corner_bottom_left_near]);

   planes[frustum_plane_far] = make_plane(corners[frustum_corner_top_left_far],
                                          corners[frustum_corner_bottom_left_far],
                                          corners[frustum_corner_top_right_far]);

   planes[frustum_plane_bottom] =
      make_plane(corners[frustum_corner_bottom_left_near],
                 corners[frustum_corner_bottom_right_far],
                 corners[frustum_corner_bottom_left_far]);

   planes[frustum_plane_top] = make_plane(corners[frustum_corner_top_left_near],
                                          corners[frustum_corner_top_left_far],
                                          corners[frustum_corner_top_right_far]);

   planes[frustum_plane_left] = make_plane(corners[frustum_corner_top_left_near],
                                           corners[frustum_corner_bottom_left_far],
                                           corners[frustum_corner_top_left_far]);

   planes[frustum_plane_right] =
      make_plane(corners[frustum_corner_top_right_near],
                 corners[frustum_corner_top_right_far],
                 corners[frustum_corner_bottom_right_far]);
}

Frustum::Frustum(const glm::mat4& inv_projection_matrix, const float z_min,
                 const float z_max) noexcept
   : Frustum{inv_projection_matrix, {-1.0f, -1.0f, z_min}, {1.0f, 1.0f, z_max}}
{
}

Frustum::Frustum(const glm::mat4& inv_projection_matrix) noexcept
   : Frustum{inv_projection_matrix, 0.0f, 1.0f}
{
}

bool intersects_full(const Frustum& frustum, const Bounding_box& bbox) noexcept
{
   for (const auto& plane : frustum.planes) {
      if (outside_plane(plane, {bbox.min.x, bbox.min.y, bbox.min.z}) &
          outside_plane(plane, {bbox.max.x, bbox.min.y, bbox.min.z}) &
          outside_plane(plane, {bbox.min.x, bbox.max.y, bbox.min.z}) &
          outside_plane(plane, {bbox.max.x, bbox.max.y, bbox.min.z}) &
          outside_plane(plane, {bbox.min.x, bbox.min.y, bbox.max.z}) &
          outside_plane(plane, {bbox.max.x, bbox.min.y, bbox.max.z}) &
          outside_plane(plane, {bbox.min.x, bbox.max.y, bbox.max.z}) &
          outside_plane(plane, {bbox.max.x, bbox.max.y, bbox.max.z})) {
         return false;
      }
   }

   const auto outside_corner = [&](const float glm::vec3::*axis,
                                   auto comparator, const float corner) {
      bool outside = true;

      for (const auto& frustum_corner : frustum.corners) {
         outside &= comparator(frustum_corner.*axis, corner);
      }

      return outside;
   };

   // clang-format off
   if (outside_corner(&glm::vec3::x, std::greater<>{}, bbox.max.x)) return false;
   if (outside_corner(&glm::vec3::x, std::less<>{}, bbox.min.x))    return false;
   if (outside_corner(&glm::vec3::y, std::greater<>{}, bbox.max.y)) return false;
   if (outside_corner(&glm::vec3::y, std::less<>{}, bbox.min.y))    return false;
   if (outside_corner(&glm::vec3::z, std::greater<>{}, bbox.max.z)) return false;
   if (outside_corner(&glm::vec3::z, std::less<>{}, bbox.min.z))    return false;
   // clang-format on

   return true;
}

bool intersects_full(const Frustum& frustum, const Bounding_sphere& sphere) noexcept
{
   for (const auto& plane : frustum.planes) {
      if (outside_plane(plane, sphere.position, sphere.radius)) {
         return false;
      }
   }

   return true;
}

bool intersects(const Frustum& frustum, const Bounding_box& bbox) noexcept
{
   for (std::size_t i = frustum_plane_far; i < frustum.planes.size(); ++i) {
      const glm::vec4& plane = frustum.planes[i];

      if (outside_plane(plane, {bbox.min.x, bbox.min.y, bbox.min.z}) &
          outside_plane(plane, {bbox.max.x, bbox.min.y, bbox.min.z}) &
          outside_plane(plane, {bbox.min.x, bbox.max.y, bbox.min.z}) &
          outside_plane(plane, {bbox.max.x, bbox.max.y, bbox.min.z}) &
          outside_plane(plane, {bbox.min.x, bbox.min.y, bbox.max.z}) &
          outside_plane(plane, {bbox.max.x, bbox.min.y, bbox.max.z}) &
          outside_plane(plane, {bbox.min.x, bbox.max.y, bbox.max.z}) &
          outside_plane(plane, {bbox.max.x, bbox.max.y, bbox.max.z})) {
         return false;
      }
   }

   return true;
}

bool intersects(const Frustum& frustum, const Bounding_sphere& sphere) noexcept
{
   for (std::size_t i = frustum_plane_far; i < frustum.planes.size(); ++i) {
      const glm::vec4& plane = frustum.planes[i];

      if (outside_plane(plane, sphere.position, sphere.radius)) {
         return false;
      }
   }

   return true;
}

}
