#pragma once

#include "../shadows/frustum.hpp"

#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace sp::core {

class Shadow_camera {
public:
   Shadow_camera() noexcept
   {
      update();
   }

   void look_at(const glm::vec3 eye, const glm::vec3 at, const glm::vec3 up) noexcept
   {
      _view_matrix = glm::transpose(glm::lookAtLH(eye, at, up));

      update();
   }

   void set_projection(const float min_x, const float min_y, const float max_x,
                       const float max_y, const float min_z, const float max_z)
   {
      _projection_matrix = {1.0f, 0.0f, 0.0f, 0.0f, //
                            0.0f, 1.0f, 0.0f, 0.0f, //
                            0.0f, 0.0f, 1.0f, 0.0f, //
                            0.0f, 0.0f, 0.0f, 1.0f};

      _near_clip = min_z;
      _far_clip = max_z;

      const auto inv_x_range = 1.0f / (min_x - max_x);
      const auto inv_y_range = 1.0f / (max_y - min_y);
      const auto inv_z_range = 1.0f / (max_z - min_z);

      _projection_matrix[0][0] = inv_x_range + inv_x_range;
      _projection_matrix[1][1] = inv_y_range + inv_y_range;
      _projection_matrix[2][2] = inv_z_range;

      _projection_matrix[0][3] = -(max_x + min_x) * inv_x_range;
      _projection_matrix[1][3] = -(max_y + min_y) * inv_y_range;
      _projection_matrix[2][3] = -min_z * inv_z_range;

      update();
   }

   void set_stabilization(const glm::vec2 stabilization)
   {
      _stabilization = stabilization;

      update();
   }

   auto view_projection_matrix() const noexcept -> glm::mat4
   {
      return _view_projection_matrix;
   }

   auto inv_view_projection_matrix() const noexcept -> glm::mat4
   {
      return _inv_view_projection_matrix;
   }

   auto texture_matrix() const noexcept -> glm::mat4
   {
      return _texture_matrix;
   }

private:
   void update() noexcept
   {
      _view_projection_matrix = _view_matrix * _projection_matrix;

      _view_projection_matrix[0][3] += _stabilization.x;
      _view_projection_matrix[1][3] += _stabilization.y;

      _inv_view_projection_matrix = glm::inverse(glm::dmat4{_view_projection_matrix});

      constexpr glm::mat4 bias_matrix{0.5f, 0.0f,  0.0f, 0.5f, //
                                      0.0f, -0.5f, 0.0f, 0.5f, //
                                      0.0f, 0.0f,  1.0f, 0.0f, //
                                      0.0f, 0.0f,  0.0f, 1.0f};

      _texture_matrix = _view_projection_matrix * bias_matrix;
   }

   float _near_clip = 0.0f;
   float _far_clip = 0.0f;

   glm::vec2 _stabilization = {0.0f, 0.0f};
   glm::mat4 _view_matrix;
   glm::mat4 _projection_matrix;
   glm::mat4 _view_projection_matrix;
   glm::mat4 _inv_view_projection_matrix;
   glm::mat4 _texture_matrix;
};

inline auto make_shadow_cascade_splits(const float near_clip, const float far_clip)
   -> std::array<float, 5>
{
   const float clip_ratio = far_clip / near_clip;
   const float clip_range = far_clip - near_clip;

   std::array<float, 5> cascade_splits{};

   for (int i = 0; i < cascade_splits.size(); ++i) {
      const float split = (near_clip * std::pow(clip_ratio, i / 4.0f));
      const float split_normalized = (split - near_clip) / clip_range;

      cascade_splits[i] = split_normalized;
   }

   return cascade_splits;
}
inline auto make_cascade_shadow_camera(const glm::vec3 light_direction,
                                       const float near_split,
                                       const float far_split, const float shadow_res,
                                       const shadows::Frustum& view_frustum) -> Shadow_camera
{
   auto view_frustum_corners = view_frustum.corners;

   for (int i = 0; i < 4; ++i) {
      glm::vec3 corner_ray = view_frustum_corners[i + 4] - view_frustum_corners[i];

      view_frustum_corners[i + 4] = view_frustum_corners[i] + (corner_ray * far_split);
      view_frustum_corners[i] += (corner_ray * near_split);
   }

   glm::vec3 view_frustum_center{0.0f, 0.0f, 0.0f};

   for (const auto& corner : view_frustum_corners) {
      view_frustum_center += corner;
   }

   view_frustum_center /= 8.0f;

   float radius = std::numeric_limits<float>::lowest();

   for (const auto& corner : view_frustum_corners) {
      radius = glm::max(glm::distance(corner, view_frustum_center), radius);
   }

   glm::vec3 bounds_max{radius};
   glm::vec3 bounds_min{-radius};
   glm::vec3 casecase_extents{bounds_max - bounds_min};

   glm::vec3 shadow_camera_position =
      view_frustum_center + light_direction * -bounds_min.z;

   Shadow_camera shadow_camera;
   shadow_camera.set_projection(bounds_min.x, bounds_min.y, bounds_max.x,
                                bounds_max.y, 0.0f, casecase_extents.z);
   shadow_camera.look_at(shadow_camera_position, view_frustum_center,
                         glm::vec3{0.0f, 1.0f, 0.0f});

   auto shadow_view_projection = shadow_camera.view_projection_matrix();

   glm::vec4 shadow_origin = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f} * shadow_view_projection;
   shadow_origin /= shadow_origin.w;
   shadow_origin *= shadow_res / 2.0f;

   glm::vec2 rounded_origin = glm::round(shadow_origin);
   glm::vec2 rounded_offset = rounded_origin - glm::vec2{shadow_origin};
   rounded_offset *= 2.0f / shadow_res;

   shadow_camera.set_stabilization(rounded_offset);

   return shadow_camera;
}

inline auto make_shadow_cascades(const glm::vec3 light_direction,
                                 const glm::mat4& view_proj_matrix, const float near_clip,
                                 const float far_clip, const float shadow_res)
   -> std::array<Shadow_camera, 4>
{
   // TODO: Proper cascade splits.
   // const std::array cascade_splits = make_shadow_cascade_splits(near_clip, far_clip);
   const std::array cascade_splits = {0.0f, 0.0005f, 0.0015f, 0.0025f, 0.01f};

   (void)near_clip, (void)far_clip;

   shadows::Frustum view_frustum{glm::inverse(view_proj_matrix)};

   std::array<Shadow_camera, 4> cameras;

   for (int i = 0; i < 4; ++i) {
      cameras[i] = make_cascade_shadow_camera(light_direction, cascade_splits[i],
                                              cascade_splits[i + 1], shadow_res,
                                              view_frustum);
   }

   return cameras;
}

}