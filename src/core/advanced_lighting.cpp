#include "advanced_lighting.hpp"

#include "lighting/clusters.hpp"

#include "../shadows/frustum.hpp"

#include "../logger.hpp"

#include <bit>

namespace sp::core {

struct Spot_light_bounds {};

namespace {

struct GPU_point_light {
   glm::vec3 positionWS;
   float inv_range_sq;
   glm::vec3 color;
   std::uint32_t padding;
};

struct GPU_spot_light {
   glm::vec3 positionWS;
   float inv_range_sq;
   glm::vec3 directionWS;
   float cone_outer_param;
   glm::vec3 color;
   float cone_inner_param;
};

}

Advanced_lighting::Advanced_lighting(ID3D11Device5& device) noexcept
   : _clusters{std::make_unique<Light_clusters>(device)},
     _device{copy_raw_com_ptr(device)}
{
}

Advanced_lighting::~Advanced_lighting() = default;

void Advanced_lighting::update(ID3D11DeviceContext4& dc,
                               const Light_cluster_matrices& matrices)
{
   glm::vec3 global_light2_dir;

   game_support::acquire_global_lights(global_light1_dir, global_light2_dir);
   game_support::acquire_light_list(point_lights, spot_lights);

#if 0
   static std::vector<game_support::Point_light> extra_lights;

   if (extra_lights.empty()) {
      srand(1);
#if 1
      for (int i = 0; i < 2048; ++i) {
         float range = 4.0f;
         float inv_range_sq = 1.0f / (range * range);

         float rand_max = RAND_MAX;

         extra_lights.push_back({
            .positionWS = {std::lerp(-64.0f, 104.0f, rand() / rand_max),
                           std::lerp(3.0f, 33.0f, rand() / rand_max),
                           std::lerp(-182.0f, 32.0f, rand() / rand_max)},
            .inv_range_sq = inv_range_sq,
            .color = {rand() / rand_max * 0.5f + 0.5f, rand() / rand_max * 0.5f + 0.5f,
                      rand() / rand_max * 0.5f + 0.5f},
            .range = range,
         });
      }
#elif 0
      for (int i = 0; i < 4096; ++i) {
         float range = 4.0f;
         float inv_range_sq = 1.0f / (range * range);

         float rand_max = RAND_MAX;

         game_support::Point_light light = {
            .positionWS = {std::lerp(-64.0f, 104.0f, rand() / rand_max),
                           std::lerp(3.0f, 33.0f, rand() / rand_max),
                           std::lerp(-182.0f, 32.0f, rand() / rand_max)},
            .inv_range_sq = inv_range_sq,
            .color = {rand() / rand_max * 0.5f + 0.5f, rand() / rand_max * 0.5f + 0.5f,
                      rand() / rand_max * 0.5f + 0.5f},
            .range = range,
         };
         glm::vec3 positionWS = light.positionWS;

         if (positionWS.x >= -12.0f and positionWS.x <= 18.0f and
             positionWS.y >= 6.0f and positionWS.y <= 26.0f and
             positionWS.z >= -39.0f and positionWS.z <= -5.0f) {
            extra_lights.push_back(light);
         }
      }
#elif 0
      for (int i = 0; i < 128; ++i) {
         float range = 4.0f;
         float inv_range_sq = 1.0f / (range * range);

         float rand_max = RAND_MAX;

         extra_lights.push_back({
            .positionWS = {std::lerp(-12.0f, 18.0f, rand() / rand_max),
                           std::lerp(6.0f, 26.0f, rand() / rand_max),
                           std::lerp(-39.0f, -5.0f, rand() / rand_max)},
            .inv_range_sq = inv_range_sq,
            .color = {rand() / rand_max * 0.5f + 0.5f, rand() / rand_max * 0.5f + 0.5f,
                      rand() / rand_max * 0.5f + 0.5f},
            .range = range,
         });
      }
#endif
   }

   point_lights.insert(point_lights.end(), extra_lights.begin(), extra_lights.end());
#endif
   visible_point_lights.clear();
   visible_point_lights.reserve(point_lights.size());

   shadows::Frustum frustum{glm::inverse(matrices.view_proj_matrix)};

   // ASYNC OPPORTUNITY (+ SIMD) - Could run in paralell with preallocated storage and atomics.
   for (const game_support::Point_light& light : point_lights) {
      if (shadows::intersects_full(frustum, {.position = light.positionWS,
                                             .radius = light.range})) {
         visible_point_lights.push_back(light);
      }
   }

#if 0
   static std::vector<game_support::Spot_light> extra_spot_lights;

   if (extra_spot_lights.empty()) {
      srand(2);
#if 0
      for (int i = 0; i < 4096; ++i) {
         const float pi = 3.14159265358979323846f;

         float range = 8.0f;
         float inv_range_sq = 1.0f / (range * range);

         float rand_max = RAND_MAX;

         float outer_angle = std::lerp(pi * 0.25f, pi * 0.75f, rand() / rand_max);
         float inner_angle =
            std::lerp(outer_angle * 0.5f, outer_angle * 0.875f, rand() / rand_max);

         float cos_half_outer_angle = cosf(outer_angle * 0.5f);
         float cos_half_inner_angle = cosf(inner_angle * 0.5f);

         const game_support::Spot_light light = {
            .positionWS = {std::lerp(-64.0f, 104.0f, rand() / rand_max),
                           std::lerp(3.0f, 33.0f, rand() / rand_max),
                           std::lerp(-182.0f, 32.0f, rand() / rand_max)},
            .inv_range_sq = inv_range_sq,
            .directionWS =
               glm::normalize(glm::vec3{std::lerp(-2.0f, 2.0f, rand() / rand_max),
                                        std::lerp(-2.0f, 2.0f, rand() / rand_max),
                                        std::lerp(-2.0f, 2.0f, rand() / rand_max)}),
            .range = range,
            .color = {rand() / rand_max * 0.5f + 0.5f, rand() / rand_max * 0.5f + 0.5f,
                      rand() / rand_max * 0.5f + 0.5f},
            .cos_half_outer_angle = cos_half_outer_angle,
            .cos_half_inner_angle = cos_half_inner_angle,
            .outer_angle = outer_angle,
            .cone_radius = range * tanf(outer_angle * 0.5f),
         };

         extra_spot_lights.push_back(light);
      }
#elif 1
      srand(2);

      for (int i = 0; i < 2048; ++i) {
         const float pi = 3.14159265358979323846f;

         float range = 4.0f;
         float inv_range_sq = 1.0f / (range * range);

         float rand_max = RAND_MAX;

         float outer_angle = pi * 0.75f;
         float inner_angle = pi * 0.5f;

         float cos_half_outer_angle = cosf(outer_angle * 0.5f);
         float cos_half_inner_angle = cosf(inner_angle * 0.5f);

         const game_support::Spot_light light = {
            .positionWS = {std::lerp(-64.0f, 104.0f, rand() / rand_max),
                           std::lerp(3.0f, 33.0f, rand() / rand_max),
                           std::lerp(-182.0f, 32.0f, rand() / rand_max)},
            .inv_range_sq = inv_range_sq,
            .directionWS =
               glm::normalize(glm::vec3{std::lerp(-2.0f, 2.0f, rand() / rand_max),
                                        std::lerp(-2.0f, 2.0f, rand() / rand_max),
                                        std::lerp(-2.0f, 2.0f, rand() / rand_max)}),
            .range = range,
            .color = {rand() / rand_max * 0.5f + 0.5f, rand() / rand_max * 0.5f + 0.5f,
                      rand() / rand_max * 0.5f + 0.5f},
            .cos_half_outer_angle = cos_half_outer_angle,
            .cos_half_inner_angle = cos_half_inner_angle,
            .outer_angle = outer_angle,
            .cone_radius = range * tanf(outer_angle * 0.5f),
         };

         extra_spot_lights.push_back(light);
      }
#endif
   }

   spot_lights.insert(spot_lights.end(), extra_spot_lights.begin(),
                      extra_spot_lights.end());
#endif

   visible_spot_lights.clear();
   visible_spot_lights.reserve(spot_lights.size());

   // ASYNC OPPORTUNITY (+ SIMD) - Could run in paralell with preallocated storage and atomics.
   for (const game_support::Spot_light& light : spot_lights) {
      // TODO: Cull using AABB not overly large bounding sphere.
      if (shadows::intersects_full(frustum, {.position = light.positionWS,
                                             .radius = light.range})) {
         visible_spot_lights.push_back(light);
      }
   }

   visible_spot_lights = spot_lights;

   upload_light_buffers(dc);

   // ASYNC OPPORTUNITY - Could run at the same time as upload_light buffers. (clusters.upload(dc) however can not).
   _clusters->build(matrices, visible_point_lights, spot_lights);
   _clusters->upload(dc);
}

auto Advanced_lighting::get_srvs() const noexcept
   -> std::array<ID3D11ShaderResourceView*, 4>
{
   return {
      _clusters->cluster_index_srv.get(),
      _clusters->cluster_light_lists_srv.get(),
      _point_lights_srv.get(),
      _spot_lights_srv.get(),
   };
}

void Advanced_lighting::upload_light_buffers(ID3D11DeviceContext4& dc) noexcept
{
   if (visible_point_lights.empty()) return;

   if (visible_point_lights.size() > _point_lights_capacity) {
      _point_lights_capacity = std::bit_ceil(visible_point_lights.size());

      const D3D11_BUFFER_DESC point_lights_desc = {
         .ByteWidth = sizeof(GPU_point_light) * _point_lights_capacity,
         .Usage = D3D11_USAGE_DYNAMIC,
         .BindFlags = D3D11_BIND_SHADER_RESOURCE,
         .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
         .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
         .StructureByteStride = sizeof(GPU_point_light),
      };

      if (FAILED(_device->CreateBuffer(&point_lights_desc, nullptr,
                                       _point_lights_buffer.clear_and_assign()))) {
         log_and_terminate("Failed to create point lights buffer!");
      }

      if (FAILED(_device->CreateShaderResourceView(_point_lights_buffer.get(), nullptr,
                                                   _point_lights_srv.clear_and_assign()))) {
         log_and_terminate("Failed to create point lights SRV!");
      }
   }

   D3D11_MAPPED_SUBRESOURCE mapped_point_lights;

   if (FAILED(dc.Map(_point_lights_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                     &mapped_point_lights))) {
      log_and_terminate("Failed to map point lights buffer!");
   }

   GPU_point_light* point_light_upload =
      reinterpret_cast<GPU_point_light*>(mapped_point_lights.pData);

   for (std::size_t i = 0; i < visible_point_lights.size(); ++i) {
      const game_support::Point_light& light = visible_point_lights[i];

      GPU_point_light gpu_light = {.positionWS = light.positionWS,
                                   .inv_range_sq = light.inv_range_sq,
                                   .color = light.color};

      std::memcpy(point_light_upload + i, &gpu_light, sizeof(gpu_light));
   }

   dc.Unmap(_point_lights_buffer.get(), 0);

   if (visible_spot_lights.empty()) return;

   if (visible_spot_lights.size() > _spot_lights_capacity) {
      _spot_lights_capacity = std::bit_ceil(visible_spot_lights.size());

      const D3D11_BUFFER_DESC spot_lights_desc = {
         .ByteWidth = sizeof(GPU_spot_light) * _spot_lights_capacity,
         .Usage = D3D11_USAGE_DYNAMIC,
         .BindFlags = D3D11_BIND_SHADER_RESOURCE,
         .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
         .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
         .StructureByteStride = sizeof(GPU_spot_light),
      };

      if (FAILED(_device->CreateBuffer(&spot_lights_desc, nullptr,
                                       _spot_lights_buffer.clear_and_assign()))) {
         log_and_terminate("Failed to create spot lights buffer!");
      }

      if (FAILED(_device->CreateShaderResourceView(_spot_lights_buffer.get(), nullptr,
                                                   _spot_lights_srv.clear_and_assign()))) {
         log_and_terminate("Failed to create spot lights SRV!");
      }
   }

   D3D11_MAPPED_SUBRESOURCE mapped_spot_lights;

   if (FAILED(dc.Map(_spot_lights_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                     &mapped_spot_lights))) {
      log_and_terminate("Failed to map spot lights buffer!");
   }

   GPU_spot_light* spot_light_upload =
      reinterpret_cast<GPU_spot_light*>(mapped_spot_lights.pData);

   for (std::size_t i = 0; i < visible_spot_lights.size(); ++i) {
      const game_support::Spot_light& light = visible_spot_lights[i];

      GPU_spot_light gpu_light = {.positionWS = light.positionWS,
                                  .inv_range_sq = light.inv_range_sq,
                                  .directionWS = light.directionWS,
                                  .cone_outer_param = light.cos_half_outer_angle,
                                  .color = light.color,
                                  .cone_inner_param =
                                     1.0f / (light.cos_half_inner_angle -
                                             light.cos_half_outer_angle)};

      std::memcpy(spot_light_upload + i, &gpu_light, sizeof(gpu_light));
   }

   dc.Unmap(_spot_lights_buffer.get(), 0);
}

}