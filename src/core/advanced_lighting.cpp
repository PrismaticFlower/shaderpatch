#include "advanced_lighting.hpp"

#include "lighting/clusters.hpp"

#include "../game_support/texture_table.hpp"

#include "../shadows/frustum.hpp"

#include "../logger.hpp"

#include <algorithm>
#include <bit>

namespace sp::core {

struct Spot_light_bounds {};

struct Sorting_texture {
   std::uint32_t texture_name_hash = 0;
   float nearest_light = FLT_MAX;
};

namespace {

struct GPU_point_light {
   glm::vec3 positionWS;
   float inv_range_sq;
   glm::vec3 color;
   std::uint32_t options;
};

struct GPU_spot_light {
   glm::vec3 positionWS;
   float inv_range_sq;
   glm::vec3 directionWS;
   float cone_outer_param;
   glm::vec3 color;
   float cone_inner_param;
   std::uint32_t options;
   std::array<std::uint32_t, 3> padding;
};

const static std::uint32_t light_options_static_bit = 0u;
const static std::uint32_t light_options_has_texture_bit = 1u;
const static std::uint32_t light_options_has_shadow_bit = 2u;

const static std::uint32_t light_options_texture_transform_offset = 3u;
const static std::uint32_t light_options_texture_transform_bits = 13u;
const static std::uint32_t light_options_texture_transform_mask =
   (1u << light_options_texture_transform_bits) - 1u;

const static std::uint32_t light_options_texture_index_offset = 16u;
const static std::uint32_t light_options_texture_index_bits = 2u;
const static std::uint32_t light_options_texture_index_mask =
   (1u << light_options_texture_index_bits) - 1u;

const static std::uint32_t light_options_shadow_data_offset = 18u;
const static std::uint32_t light_options_shadow_data_bits = 13u;
const static std::uint32_t light_options_shadow_data_mask =
   (1u << light_options_texture_transform_bits) - 1u;

[[msvc::forceinline]] auto pack_options(game_support::Light_options options) noexcept
   -> std::uint32_t
{
   std::uint32_t packed = 0;

   if (options.has_texture) {
      packed |= (1u << light_options_has_texture_bit);
      packed |= ((options.texture_index & light_options_texture_transform_mask)
                 << light_options_texture_transform_offset);
      packed |= ((options.texture_srv_index & light_options_texture_index_mask)
                 << light_options_texture_index_offset);
   }

   return packed;
}

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
   game_support::acquire_light_list(point_lights, spot_lights, textures);

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

   process_textures(frustum, false);
   upload_light_buffers(dc);

   // ASYNC OPPORTUNITY - Could run at the same time as upload_light buffers + process_textures. (clusters.upload(dc) however can not).
   _clusters->build(matrices, visible_point_lights, spot_lights);
   _clusters->upload(dc);
}

auto Advanced_lighting::get_srvs() const noexcept
   -> std::array<ID3D11ShaderResourceView*, 11>
{
   return {
      _clusters->cluster_index_srv.get(),
      _clusters->cluster_light_lists_srv.get(),
      _point_lights_srv.get(),
      _spot_lights_srv.get(),
      _textures_srv.get(),

      _active_point_light_textures[0].srv.get(),
      _active_point_light_textures[1].srv.get(),
      _active_point_light_textures[2].srv.get(),

      _active_spot_light_textures[0].srv.get(),
      _active_spot_light_textures[1].srv.get(),
      _active_spot_light_textures[2].srv.get(),
   };
}

template<typename T>
void Advanced_lighting::apply_textures(
   std::vector<T>& visible_lights,
   const std::array<Light_texture, texture_count_per_type>& active_light_textures) noexcept
{
   for (T& light : visible_lights) {
      if (light.options.has_texture) {
         const game_support::Light_texture& source_texture =
            textures[light.options.texture_index];

         light.options.has_texture = false;
         light.options.texture_index = 0;

         for (std::uint32_t i = 0; i < active_light_textures.size(); ++i) {
            if (active_light_textures[i].texture_name_hash == source_texture.name_hash) {
               light.options.has_texture = true;
               light.options.texture_srv_index = i;

               if constexpr (std::is_same_v<T, game_support::Spot_light>) {
                  light.options.texture_index = visible_textures.size();

                  visible_textures.push_back(source_texture.matrix);
               }

               break;
            }
         }
      }
   }
}

void Advanced_lighting::process_textures(const shadows::Frustum& frustum,
                                         const bool srgb_textures) noexcept
{
   // ASYNC OPPORTUNITY - Could have seperate scratch space for point and spot lights and process them at the same time.

   visible_textures.clear();

   _light_textures_scratch.clear();

   for (const game_support::Point_light& light : visible_point_lights) {
      if (light.options.has_texture) {
         const game_support::Light_texture& texture =
            textures[light.options.texture_index];

         const float light_planar_distance =
            glm::dot(frustum.planes[shadows::frustum_plane_near],
                     glm::vec4{light.positionWS, 1.0f

                     }) -
            light.range;

         if (auto it = std::lower_bound(_light_textures_scratch.begin(),
                                        _light_textures_scratch.end(), texture.name_hash,
                                        [&](const Sorting_texture& other,
                                            const std::uint32_t hash) {
                                           return other.texture_name_hash < hash;
                                        });
             it != _light_textures_scratch.end() and
             it->texture_name_hash == texture.name_hash) {
            it->nearest_light = std::min(it->nearest_light, light_planar_distance);
         }
         else {
            _light_textures_scratch.insert(it, Sorting_texture{
                                                  .texture_name_hash = texture.name_hash,
                                                  .nearest_light = light_planar_distance,
                                               });
         }
      }
   }

   _active_point_light_textures =
      select_textures(_light_textures_scratch, srgb_textures);

   apply_textures(visible_point_lights, _active_point_light_textures);

   _light_textures_scratch.clear();

   for (const game_support::Spot_light& light : visible_spot_lights) {
      if (light.options.has_texture) {
         const game_support::Light_texture& texture =
            textures[light.options.texture_index];

         const float light_planar_distance =
            glm::dot(frustum.planes[shadows::frustum_plane_near],
                     glm::vec4{light.positionWS, 1.0f

                     }) -
            light.range;

         if (auto it = std::lower_bound(_light_textures_scratch.begin(),
                                        _light_textures_scratch.end(), texture.name_hash,
                                        [&](const Sorting_texture& other,
                                            const std::uint32_t hash) {
                                           return other.texture_name_hash < hash;
                                        });
             it != _light_textures_scratch.end() and
             it->texture_name_hash == texture.name_hash) {
            it->nearest_light = std::min(it->nearest_light, light_planar_distance);
         }
         else {
            _light_textures_scratch.insert(it, Sorting_texture{
                                                  .texture_name_hash = texture.name_hash,
                                                  .nearest_light = light_planar_distance,
                                               });
         }
      }
   }

   _active_spot_light_textures =
      select_textures(_light_textures_scratch, srgb_textures);

   apply_textures(visible_spot_lights, _active_spot_light_textures);
}

auto Advanced_lighting::select_textures(std::span<const Sorting_texture> light_textures,
                                        const bool srgb_textures) noexcept
   -> std::array<Light_texture, texture_count_per_type>
{
   std::array<Sorting_texture, texture_count_per_type> selected_textures = {{
      {0, FLT_MAX},
      {0, FLT_MAX},
      {0, FLT_MAX},
   }};

   for (const Sorting_texture& texture : light_textures) {
      if (texture.nearest_light < selected_textures[0].nearest_light) {
         selected_textures[2] = selected_textures[1];
         selected_textures[1] = selected_textures[0];
         selected_textures[0] = texture;
      }
      else if (texture.nearest_light < selected_textures[1].nearest_light) {
         selected_textures[2] = selected_textures[1];
         selected_textures[1] = texture;
      }
      else if (texture.nearest_light < selected_textures[2].nearest_light) {
         selected_textures[2] = texture;
      }
   }

   std::array<Light_texture, texture_count_per_type> active_textures;

   for (std::size_t i = 0; i < selected_textures.size(); ++i) {
      if (selected_textures[i].texture_name_hash == 0) {
         active_textures[i] = {};
      }
      else {
         ID3D11ShaderResourceView* srv =
            game_support::lookup_texture(selected_textures[i].texture_name_hash,
                                         srgb_textures);

         if (srv) {
            active_textures[i] = {
               .texture_name_hash = selected_textures[i].texture_name_hash,
               .srv = copy_raw_com_ptr(srv),
            };
         }
         else {
            active_textures[i] = {};
         }
      }
   }

   return active_textures;
}

void Advanced_lighting::upload_light_buffers(ID3D11DeviceContext4& dc) noexcept
{
   if (not visible_point_lights.empty()) {
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

      if (FAILED(dc.Map(_point_lights_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD,
                        0, &mapped_point_lights))) {
         log_and_terminate("Failed to map point lights buffer!");
      }

      GPU_point_light* point_light_upload =
         reinterpret_cast<GPU_point_light*>(mapped_point_lights.pData);

      for (std::size_t i = 0; i < visible_point_lights.size(); ++i) {
         const game_support::Point_light& light = visible_point_lights[i];

         GPU_point_light gpu_light = {.positionWS = light.positionWS,
                                      .inv_range_sq = light.inv_range_sq,
                                      .color = light.color,
                                      .options = pack_options(light.options)};

         std::memcpy(point_light_upload + i, &gpu_light, sizeof(gpu_light));
      }

      dc.Unmap(_point_lights_buffer.get(), 0);
   }

   if (not visible_spot_lights.empty()) {
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

      if (FAILED(dc.Map(_spot_lights_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD,
                        0, &mapped_spot_lights))) {
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
                                                light.cos_half_outer_angle),
                                     .options = pack_options(light.options)};

         std::memcpy(spot_light_upload + i, &gpu_light, sizeof(gpu_light));
      }

      dc.Unmap(_spot_lights_buffer.get(), 0);
   }

   if (not visible_textures.empty()) {
      if (visible_textures.size() > _textures_capacity) {
         _textures_capacity = std::bit_ceil(visible_textures.size());

         const D3D11_BUFFER_DESC textures_desc = {
            .ByteWidth = sizeof(glm::mat4) * _textures_capacity,
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
            .StructureByteStride = sizeof(glm::mat4),
         };

         if (FAILED(_device->CreateBuffer(&textures_desc, nullptr,
                                          _textures_buffer.clear_and_assign()))) {
            log_and_terminate("Failed to create textures buffer!");
         }

         if (FAILED(_device->CreateShaderResourceView(_textures_buffer.get(), nullptr,
                                                      _textures_srv.clear_and_assign()))) {
            log_and_terminate("Failed to create textures SRV!");
         }
      }

      D3D11_MAPPED_SUBRESOURCE mapped_textures;

      if (FAILED(dc.Map(_textures_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                        &mapped_textures))) {
         log_and_terminate("Failed to map textures buffer!");
      }

      glm::mat4* spot_light_upload =
         reinterpret_cast<glm::mat4*>(mapped_textures.pData);

      for (std::size_t i = 0; i < visible_textures.size(); ++i) {
         std::memcpy(spot_light_upload + i, &visible_textures[i],
                     sizeof(visible_textures[i]));
      }

      dc.Unmap(_textures_buffer.get(), 0);
   }
}

}