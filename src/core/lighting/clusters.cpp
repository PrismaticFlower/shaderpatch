#include "clusters.hpp"

#include "../../shadows/bounding_box.hpp"

#include "../../imgui/imgui.h"
#include "../../logger.hpp"

#include <source_location>

#include <DirectXMath.h>

namespace sp::core {

namespace {

constexpr float near_cluster_z = 5.0f;
constexpr float far_cluster_z = 1000.0f;

#if 0
   if (ImGui::CollapsingHeader("Matrices")) {

      ImGui::SeparatorText("proj matrix");

      for (int i = 0; i < 4; ++i)
         ImGui::Text("%f %f %f %f", matrices.proj_matrix[i][0],
                     matrices.proj_matrix[i][1], matrices.proj_matrix[i][2],
                     matrices.proj_matrix[i][3]);

      ImGui::SeparatorText("view matrix");

      for (int i = 0; i < 4; ++i)
         ImGui::Text("%f %f %f %f", matrices.view_matrix[i][0],
                     matrices.view_matrix[i][1], matrices.view_matrix[i][2],
                     matrices.view_matrix[i][3]);

      ImGui::SeparatorText("view proj matrix");

      for (int i = 0; i < 4; ++i)
         ImGui::Text("%f %f %f %f", matrices.view_proj_matrix[i][0],
                     matrices.view_proj_matrix[i][1],
                     matrices.view_proj_matrix[i][2],
                     matrices.view_proj_matrix[i][3]);

      glm::mat4 view_proj_alt = matrices.view_matrix * matrices.proj_matrix;

      ImGui::SeparatorText("view proj alt");

      for (int i = 0; i < 4; ++i)
         ImGui::Text("%f %f %f %f", view_proj_alt[i][0], view_proj_alt[i][1],
                     view_proj_alt[i][2], view_proj_alt[i][3]);
   }

   ImGui::SeparatorText("lights");

   const float cluster_count = _z_clusters;

   for (int i = 0; i < _z_clusters; ++i) {
      const float split = near_cluster_z * std::pow(far_cluster_z / near_cluster_z,
                                                    i / cluster_count);

      ImGui::Text("split %f", split);

      const float reverse_split = log2f(split / near_cluster_z) /
                                  log2f(far_cluster_z / near_cluster_z) * cluster_count;

      ImGui::Text("reverse_split %f", (reverse_split));
   }

      ImGui::Text("positionVS %f %f %f", positionVS.x, positionVS.y, positionVS.z);
      ImGui::Text("min_zVS %f max_zVS %f", min_zVS, max_zVS);
      ImGui::Text("first %f last %f", first_cluster, last_cluster);

      ImGui::Separator();

#endif

struct GPU_cluster_index {
   std::uint32_t point_lights_start = 0;
   std::uint32_t point_lights_end = 0;
   std::uint32_t spot_lights_start = 0;
   std::uint32_t spot_lights_end = 0;
};

struct Time {
   Time(std::source_location loc = std::source_location::current())
      : label{loc.function_name()}
   {
      QueryPerformanceCounter(&start);
   }

   ~Time()
   {
      LARGE_INTEGER end;
      QueryPerformanceCounter(&end);

      LARGE_INTEGER freq;
      QueryPerformanceFrequency(&freq);

      ImGui::Begin("Slim Profile");

      ImGui::Text("%s time %f", label,
                  (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000.0f);

      ImGui::End();
   }

   const char* label = "";
   LARGE_INTEGER start;
};

auto cone_bbox(const glm::vec3& position, const glm::vec3& direction,
               const float length, const float cone_radius) noexcept -> shadows::Bounding_box
{
   const glm::vec3 cap_centre = position + direction * length;
   const glm::vec3 e = cone_radius * glm::sqrt(1.0f - direction * direction);

   return {
      .min = glm::min(position, cap_centre - e),
      .max = glm::max(position, cap_centre + e),
   };
}

auto log2_packed(const float a, const float b) noexcept -> std::array<float, 2>
{
   const DirectX::XMFLOAT2 vec2 = {a, b};

   const DirectX::XMVECTOR log2_results =
      DirectX::XMVectorLog2(DirectX::XMLoadFloat2(&vec2));

   return {DirectX::XMVectorGetX(log2_results), DirectX::XMVectorGetY(log2_results)};
}

auto ceil_sse4_1(float v) noexcept -> float
{
   __m128 m128_value = _mm_load_ss(&v);

   float result;

   _mm_store_ss(&result, _mm_ceil_ss(m128_value, m128_value));

   return result;
}

auto sqrt(const float v) noexcept -> float
{
   __m128 m128_v = _mm_load_ss(&v);

   float sqrt_v;

   _mm_store_ss(&sqrt_v, _mm_sqrt_ss(m128_v));

   return sqrt_v;
}

auto length(const glm::vec3& v) noexcept -> float
{
   return sqrt(dot(v, v));
}

// Cull that cone! Improved cone/spotlight visibility tests for tiled and clustered lighting - Bart Wronski
// https://bartwronski.com/2017/04/13/cull-that-cone/
bool cone_intersects(const glm::vec3& cone_position,
                     const glm::vec3& cone_direction, const float cone_length,
                     const float cone_sin_angle, const float cone_cos_angle,
                     const glm::vec3& sphere_position, const float sphere_radius) noexcept
{
   const glm::vec3 v = sphere_position - cone_position;
   const float v_length_sq = glm::dot(v, v);
   const float v1_length = glm::dot(v, cone_direction);
   const float distance_closest_point =
      cone_cos_angle * sqrt(v_length_sq - v1_length * v1_length) -
      v1_length * cone_sin_angle;

   const bool angle_cull = distance_closest_point > sphere_radius;
   const bool front_cull = v1_length > sphere_radius + cone_length;
   const bool back_cull = v1_length < -sphere_radius;

   return !(angle_cull || front_cull || back_cull);
}

}

Light_clusters::Light_clusters(ID3D11Device5& device) noexcept
{
   const float slice_count = _z_clusters;

   _z_splits[0] = 0.0f;

   for (int i = 1; i <= _z_clusters; ++i) {
      _z_splits[i] = near_cluster_z *
                     std::pow(far_cluster_z / near_cluster_z, i / slice_count);
   }

   const std::uint32_t cluster_count = _x_clusters * _y_clusters * _z_clusters;

   const D3D11_BUFFER_DESC index_desc = {
      .ByteWidth = sizeof(GPU_cluster_index) * cluster_count,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
      .StructureByteStride = sizeof(GPU_cluster_index),
   };

   if (FAILED(device.CreateBuffer(&index_desc, nullptr,
                                  _cluster_index_buffer.clear_and_assign()))) {
      log_and_terminate("Failed to create cluster index!");
   }

   if (FAILED(device.CreateShaderResourceView(_cluster_index_buffer.get(), nullptr,
                                              cluster_index_srv.clear_and_assign()))) {
      log_and_terminate("Failed to create cluster index SRV!");
   }

   const D3D11_BUFFER_DESC list_desc = {
      .ByteWidth = sizeof(std::uint32_t) * cluster_max_lights * cluster_count,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
      .StructureByteStride = sizeof(std::uint32_t),
   };

   if (FAILED(device.CreateBuffer(&list_desc, nullptr,
                                  _cluster_light_lists_buffer.clear_and_assign()))) {
      log_and_terminate("Failed to create cluster light list!");
   }

   if (FAILED(device.CreateShaderResourceView(_cluster_light_lists_buffer.get(), nullptr,
                                              cluster_light_lists_srv.clear_and_assign()))) {
      log_and_terminate("Failed to create cluster light list SRV!");
   }
}

void Light_clusters::build(const Light_cluster_matrices& matrices,
                           std::span<const game_support::Point_light> point_lights,
                           std::span<const game_support::Spot_light> spot_lights) noexcept
{
   Time timer;

   build_spot_light_bounds(matrices, spot_lights); // ASYNC OPPORTUNITY - Could preallocate memory and process each light in paralell

   z_bin_lights(matrices, point_lights,
                spot_lights); // ASYNC OPPORTUNITY - Could preallocate memory and use atomics to process each light in paralell

   build_tile_directions(matrices); // ASYNC OPPORTUNITY - Could run same time as Z binning.
   build_cluster_bounds(); // ASYNC OPPORTUNITY - Could run same time as Z binning, after build_tile_directions.

   build_xy_clusters(matrices, point_lights, spot_lights);

#if 0
   if (ImGui::CollapsingHeader("Z Bins")) {
      for (int i = 0; i < _z_clusters; ++i) {
         if (ImGui::TreeNodeEx((void*)i, ImGuiTreeNodeFlags_CollapsingHeader,
                               "Z %i (lights: %u)", i,
                               _z_bins[i].point_lights.size())) {
            for (int light : _z_bins[i].point_lights) ImGui::Text("%i", light);
         }
      }
   }

   if (ImGui::CollapsingHeader("Tile Directions")) {
      const std::size_t x_directions = _x_clusters + 1;
      const std::size_t y_directions = _y_clusters + 1;

      if (ImGui::BeginTable("Table", x_directions)) {
         for (std::size_t y = 0; y < y_directions; ++y) {
            ImGui::TableNextRow();

            for (std::size_t x = 0; x < x_directions; ++x) {
               const glm::vec3& directionVS = _tile_corner_directions[y][x];

               ImGui::PushID(x);
               ImGui::PushID(y);

               ImGui::TableNextColumn();

               const ImVec2 cursor = ImGui::GetCursorPos();

               ImGui::Selectable("##button");

               if (ImGui::IsItemHovered() && ImGui::BeginTooltip()) {
                  ImGui::Text("X: %f Y: %f Z: %f", directionVS.x, directionVS.y,
                              directionVS.z);

                  ImGui::EndTooltip();
               }

               ImGui::SetCursorPos(cursor);

               ImGui::Text("(%u, %u)", x, y);

               ImGui::PopID();
               ImGui::PopID();
            }
         }
      }

      ImGui::EndTable();
   }

   if (ImGui::CollapsingHeader("Clusters")) {
      static int z_slice = 0;

      ImGui::SliderInt("Z Slice", &z_slice, 0, _z_clusters - 1, "%d",
                       ImGuiSliderFlags_AlwaysClamp);

      const float cluster_startZ = _z_splits[z_slice];
      const float cluster_endZ = _z_splits[z_slice + 1];

      ImGui::Text("Cluster Z Range [%f, %f]", cluster_startZ, cluster_endZ);

      std::array<std::array<Cluster, _x_clusters>, _y_clusters>& clusters =
         _xy_clusters[z_slice];

      if (ImGui::BeginTable("Table", _x_clusters)) {
         for (std::size_t y = 0; y < _y_clusters; ++y) {
            ImGui::TableNextRow();

            for (std::size_t x = 0; x < _x_clusters; ++x) {
               Cluster& cluster = clusters[y][x];

               ImGui::PushID(x);
               ImGui::PushID(y);

               ImGui::TableNextColumn();

               const ImVec2 cursor = ImGui::GetCursorPos();

               ImGui::Selectable("##button");

               if (ImGui::IsItemHovered() && ImGui::BeginTooltip()) {
                  ImGui::Text("(%u, %u)", x, y);

                  ImGui::EndTooltip();
               }

               ImGui::SetCursorPos(cursor);

               ImGui::Text("(%u, %u)", cluster.info.point_light_count,
                           cluster.info.spot_light_count);

               ImGui::PopID();
               ImGui::PopID();
            }
         }
      }

      ImGui::EndTable();
   }
#endif
}

void Light_clusters::upload(ID3D11DeviceContext4& dc) noexcept
{
   Time timer;

   D3D11_MAPPED_SUBRESOURCE index_mapped;

   if (FAILED(dc.Map(_cluster_index_buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                     &index_mapped))) {
      log_and_terminate("Failed to map cluster index for update.");
   }

   D3D11_MAPPED_SUBRESOURCE lists_mapped;

   if (FAILED(dc.Map(_cluster_light_lists_buffer.get(), 0,
                     D3D11_MAP_WRITE_DISCARD, 0, &lists_mapped))) {
      log_and_terminate("Failed to map cluster light lists for update.");
   }

   GPU_cluster_index* index_upload =
      reinterpret_cast<GPU_cluster_index*>(index_mapped.pData);
   std::uint32_t* lists_upload = reinterpret_cast<std::uint32_t*>(lists_mapped.pData);

   std::uint32_t allocated_list_lights = 0;

   const std::uint32_t slice_size = _x_clusters * _y_clusters;

   for (int z = 0; z < _z_clusters; ++z) {
      for (int y = 0; y < _y_clusters; ++y) {
         for (int x = 0; x < _x_clusters; ++x) {
            const Cluster& cluster = _xy_clusters[z][y][x];

            GPU_cluster_index index{};

            const std::uint32_t cluster_light_count =
               cluster.info.point_light_count + cluster.info.spot_light_count;

            if (cluster_light_count > 0) {
               index.point_lights_start = allocated_list_lights;
               index.point_lights_end =
                  index.point_lights_start + cluster.info.point_light_count;

               index.spot_lights_start = index.point_lights_end;
               index.spot_lights_end =
                  index.spot_lights_start + cluster.info.spot_light_count;

               std::memcpy(lists_upload + allocated_list_lights,
                           cluster.lights.data(),
                           cluster_light_count * sizeof(std::uint32_t));

               allocated_list_lights += cluster_light_count;
            }

            const std::uint32_t cluster_index = z * slice_size + y * _x_clusters + x;

            std::memcpy(index_upload + cluster_index, &index,
                        sizeof(GPU_cluster_index));
         }
      }
   }

   dc.Unmap(_cluster_index_buffer.get(), 0);
   dc.Unmap(_cluster_light_lists_buffer.get(), 0);
}

void Light_clusters::build_spot_light_bounds(
   const Light_cluster_matrices& matrices,
   std::span<const game_support::Spot_light> spot_lights) noexcept
{
   Time timer;

   _spot_light_bounds.clear();
   _spot_light_bounds.reserve(spot_lights.size());

   for (const game_support::Spot_light& light : spot_lights) {
      Spot_light_bounds& bounds = _spot_light_bounds.emplace_back();

      bounds.positionVS = glm::vec4{light.positionWS, 1.0f} * matrices.view_matrix;

      bounds.range = light.range;

      bounds.directionVS =
         glm::normalize(light.directionWS * glm::mat3{matrices.view_matrix});

      DirectX::XMScalarSinCos(&bounds.sin_angle, &bounds.cos_angle, light.outer_angle);

      const shadows::Bounding_box bboxVS =
         cone_bbox(bounds.positionVS, bounds.directionVS, light.range, light.cone_radius);

      bounds.minVS = bboxVS.min;
      bounds.maxVS = bboxVS.max;
   }
}

void Light_clusters::z_bin_lights(const Light_cluster_matrices& matrices,
                                  std::span<const game_support::Point_light> point_lights,
                                  std::span<const game_support::Spot_light> spot_lights) noexcept
{
   Time timer;

   for (Z_bin& bin : _z_bins) {
      bin.point_lights.clear();
      bin.spot_lights.clear();
   }

   const float log2_far_z_over_near_z = log2f(far_cluster_z / near_cluster_z);
   const float z_clusters = _z_clusters;
   const float max_z_cluster = _z_clusters - 1.0f;

   for (std::size_t light_index = 0; light_index < point_lights.size(); ++light_index) {
      const game_support::Point_light& light = point_lights[light_index];

      const glm::vec3 positionVS =
         glm::vec4{light.positionWS, 1.0f} * matrices.view_matrix;

      const float min_zVS = positionVS.z - light.range;
      const float max_zVS = positionVS.z + light.range;

      if (min_zVS >= far_cluster_z || max_zVS < 0.0f) continue;

      const std::array<float, 2> min_max_over_near_log2 =
         log2_packed(min_zVS / near_cluster_z, max_zVS / near_cluster_z);

      float first_cluster =
         min_max_over_near_log2[0] / log2_far_z_over_near_z * z_clusters;
      float last_cluster =
         min_max_over_near_log2[1] / log2_far_z_over_near_z * z_clusters;

      first_cluster = std::max(0.0f, first_cluster);
      last_cluster = std::max(0.0f, last_cluster);
      first_cluster = std::min(max_z_cluster, first_cluster);
      last_cluster = std::min(max_z_cluster, last_cluster);

      const int z_start = static_cast<int>(first_cluster);
      const int z_end = static_cast<int>(last_cluster);

      for (int z = z_start; z <= z_end; ++z) {
         _z_bins[z].point_lights.push_back(static_cast<std::uint16_t>(light_index));
      }
   }

   for (std::size_t light_index = 0; light_index < spot_lights.size(); ++light_index) {
      const game_support::Spot_light& light = spot_lights[light_index];

      const shadows::Bounding_box bboxVS =
         cone_bbox(glm::vec4{light.positionWS, 1.0f} * matrices.view_matrix,
                   glm::normalize(light.directionWS * glm::mat3{matrices.view_matrix}),
                   light.range, light.cone_radius);

      const glm::vec3 positionVS =
         glm::vec4{light.positionWS, 1.0f} * matrices.view_matrix;

      const float min_zVS = positionVS.z - light.range;
      const float max_zVS = positionVS.z + light.range;

      if (min_zVS >= far_cluster_z || max_zVS < 0.0f) continue;

      const std::array<float, 2> min_max_over_near_log2 =
         log2_packed(min_zVS / near_cluster_z, max_zVS / near_cluster_z);

      float first_cluster =
         min_max_over_near_log2[0] / log2_far_z_over_near_z * z_clusters;
      float last_cluster =
         min_max_over_near_log2[1] / log2_far_z_over_near_z * z_clusters;

      first_cluster = std::max(0.0f, first_cluster);
      last_cluster = std::max(0.0f, last_cluster);
      first_cluster = std::min(max_z_cluster, first_cluster);
      last_cluster = std::min(max_z_cluster, last_cluster);

      const int z_start = static_cast<int>(first_cluster);
      const int z_end = static_cast<int>(last_cluster);

      for (int z = z_start; z <= z_end; ++z) {
         _z_bins[z].spot_lights.push_back(static_cast<std::uint16_t>(light_index));
      }
   }
}

void Light_clusters::build_tile_directions(const Light_cluster_matrices& matrices) noexcept
{
   Time timer;

   const std::size_t x_directions = _x_clusters + 1;
   const std::size_t y_directions = _y_clusters + 1;

   const float x_clusters = static_cast<float>(_x_clusters);
   const float y_clusters = static_cast<float>(_y_clusters);

   const glm::mat4 inv_proj_matrix = glm::inverse(matrices.proj_matrix);

   // ASYNC OPPORTUNITY - Could process each row independently (but may not be worth it)

   for (std::size_t y = 0; y < y_directions; ++y) {
      for (std::size_t x = 0; x < x_directions; ++x) {
         const float xNDC = -1.0f + (x / x_clusters) * 2.0f;
         const float yNDC = 1.0f + (y / y_clusters) * -2.0f;

         const glm::vec4 posNDC = glm::vec4{xNDC, yNDC, 0.0f, 1.0f};

         const glm::vec4 posPS = glm::vec4{xNDC, yNDC, 0.0f, 1.0f} * inv_proj_matrix;

         const glm::vec3 posVS = glm::vec3{posPS} / posPS.w;

         const glm::vec3 directionVS = posVS / length(posVS);

         _tile_corner_directions[y][x] = directionVS;
      }
   }
}

void Light_clusters::build_cluster_bounds() noexcept
{
   Time timer;

   // ASYNC OPPORTUNITY - Could run each Z slice in paralell.
   for (int z = 0; z < _z_clusters; ++z) {
      const float cluster_startZ = _z_splits[z];
      const float cluster_endZ = _z_splits[z + 1];

      for (int y = 0; y < _y_clusters; ++y) {
         const std::array<glm::vec3, 2> start_directionsVS = {
            _tile_corner_directions[y][0],
            _tile_corner_directions[y + 1][0],
         };

         const std::array<float, 2> start_z_factors = {
            1.0f / start_directionsVS[0].z,
            1.0f / start_directionsVS[1].z,
         };

         std::array<glm::vec3, 4> cluster_cornersVS = {
            start_directionsVS[0] * cluster_startZ * start_z_factors[0],
            start_directionsVS[1] * cluster_startZ * start_z_factors[1],

            start_directionsVS[0] * cluster_endZ * start_z_factors[0],
            start_directionsVS[1] * cluster_endZ * start_z_factors[1],
         };

         for (int x = 0; x < _x_clusters; ++x) {
            glm::vec3 cluster_minVS = cluster_cornersVS[0];
            glm::vec3 cluster_maxVS = cluster_cornersVS[0];

            for (int corner = 1; corner < 4; ++corner) {
               cluster_minVS = glm::min(cluster_minVS, cluster_cornersVS[corner]);
               cluster_maxVS = glm::max(cluster_maxVS, cluster_cornersVS[corner]);
            }

            const std::array<glm::vec3, 2> directionsVS = {
               _tile_corner_directions[y][x + 1],
               _tile_corner_directions[y + 1][x + 1],
            };

            const std::array<float, 2> z_factors = {
               1.0f / directionsVS[0].z,
               1.0f / directionsVS[1].z,
            };

            cluster_cornersVS = {
               directionsVS[0] * cluster_startZ * z_factors[0],
               directionsVS[1] * cluster_startZ * z_factors[1],

               directionsVS[0] * cluster_endZ * z_factors[0],
               directionsVS[1] * cluster_endZ * z_factors[1],
            };

            for (const glm::vec3& cornerVS : cluster_cornersVS) {
               cluster_minVS = glm::min(cluster_minVS, cornerVS);
               cluster_maxVS = glm::max(cluster_maxVS, cornerVS);
            }

            _cluster_bounds[z][y][x] = {
               .centreVS = (cluster_minVS + cluster_maxVS) * 0.5f,
               .sizeVS = abs(cluster_maxVS - cluster_minVS) * 0.5f,
            };
         }
      }
   }
}

void Light_clusters::build_xy_clusters(const Light_cluster_matrices& matrices,
                                       std::span<const game_support::Point_light> point_lights,
                                       std::span<const game_support::Spot_light> spot_lights) noexcept
{
   Time timer;

   for (std::array<std::array<Cluster, _x_clusters>, _y_clusters>& slice : _xy_clusters) {
      for (std::array<Cluster, _x_clusters>& row : slice) {
         for (Cluster& cluster : row) cluster.info = {};
      }
   }

   // ASYNC OPPORTUNITY - Can process each Z cluster independently
#if 1
   (void)spot_lights;

   for (int z = 0; z < _z_clusters; ++z) {
      const float cluster_endZ = _z_splits[z + 1];

      float cluster_width = 0.0f;
      float cluster_height = 0.0f;

      {
         const int x = 0;
         const int y = 0;

         std::array<glm::vec3, 2> directionsVS = {
            _tile_corner_directions[y][x],
            _tile_corner_directions[(y + 1)][x + 1],
         };

         std::array<float, 2> z_factors = {
            1.0f / directionsVS[0].z,
            1.0f / directionsVS[1].z,
         };

         std::array<glm::vec3, 4> cluster_far_cornersVS = {
            directionsVS[0] * cluster_endZ * z_factors[0],
            directionsVS[1] * cluster_endZ * z_factors[1],
         };

         cluster_width =
            std::abs(cluster_far_cornersVS[0].x - cluster_far_cornersVS[1].x);
         cluster_height =
            std::abs(cluster_far_cornersVS[0].y - cluster_far_cornersVS[1].y);
      }

      const float half_x_clusters = _x_clusters / 2.0f;
      const float half_y_clusters = _y_clusters / 2.0f;

      for (std::uint16_t light_index : _z_bins[z].point_lights) {
         const game_support::Point_light& light = point_lights[light_index];

         const glm::vec3 positionVS =
            glm::vec4{light.positionWS, 1.0f} * matrices.view_matrix;

         const float min_xVS = positionVS.x - light.range * 1.4142135623730950488f;
         const float max_xVS = positionVS.x + light.range * 1.4142135623730950488f;

         const float min_yVS = positionVS.y - light.range * 1.4142135623730950488f;
         const float max_yVS = positionVS.y + light.range * 1.4142135623730950488f;

         const int min_cluster_x =
            std::clamp((int)(min_xVS / cluster_width + half_x_clusters - 0.5f),
                       0, _x_clusters - 1);
         const int max_cluster_x =
            std::clamp((int)ceil_sse4_1(max_xVS / cluster_width + half_x_clusters),
                       0, _x_clusters - 1);

         const int min_cluster_y =
            std::clamp((int)(-max_yVS / cluster_height + half_y_clusters - 0.5f),
                       0, _y_clusters - 1);
         const int max_cluster_y =
            std::clamp((int)(ceil_sse4_1(-min_yVS / cluster_height + half_y_clusters)),
                       0, _y_clusters - 1);

         assert(min_cluster_x <= max_cluster_x);
         assert(min_cluster_y <= max_cluster_y);

         for (int y = min_cluster_y; y <= max_cluster_y; ++y) {
            for (int x = min_cluster_x; x <= max_cluster_x; ++x) {
               const Cluster_bounds& cluster_bounds = _cluster_bounds[z][y][x];

               const glm::vec3 d = abs(positionVS - cluster_bounds.centreVS) -
                                   cluster_bounds.sizeVS;

               const float distance = length(glm::max(d, 0.0f));

               if (distance <= light.range) {
                  Cluster& cluster = _xy_clusters[z][y][x];

                  if (cluster.info.point_light_count != cluster.lights.size()) {
                     cluster.lights[cluster.info.point_light_count++] = light_index;
                  }
               }
            }
         }
      }

      for (std::uint16_t light_index : _z_bins[z].spot_lights) {
         const Spot_light_bounds& light = _spot_light_bounds[light_index];

         const glm::vec3& positionVS = light.positionVS;

         const float min_xVS = positionVS.x - light.range * 1.4142135623730950488f;
         const float max_xVS = positionVS.x + light.range * 1.4142135623730950488f;

         const float min_yVS = positionVS.y - light.range * 1.4142135623730950488f;
         const float max_yVS = positionVS.y + light.range * 1.4142135623730950488f;

         const int min_cluster_x =
            std::clamp((int)(min_xVS / cluster_width + half_x_clusters - 0.5f),
                       0, _x_clusters - 1);
         const int max_cluster_x =
            std::clamp((int)ceil_sse4_1(max_xVS / cluster_width + half_x_clusters),
                       0, _x_clusters - 1);

         const int min_cluster_y =
            std::clamp((int)(-max_yVS / cluster_height + half_y_clusters - 0.5f),
                       0, _y_clusters - 1);
         const int max_cluster_y =
            std::clamp((int)(ceil_sse4_1(-min_yVS / cluster_height + half_y_clusters)),
                       0, _y_clusters - 1);

         assert(min_cluster_x <= max_cluster_x);
         assert(min_cluster_y <= max_cluster_y);

         for (int y = min_cluster_y; y <= max_cluster_y; ++y) {
            for (int x = min_cluster_x; x <= max_cluster_x; ++x) {
               const Cluster_bounds& cluster_bounds = _cluster_bounds[z][y][x];

               {
                  const glm::vec3 d = abs(positionVS - cluster_bounds.centreVS) -
                                      cluster_bounds.sizeVS;

                  const float distance = length(glm::max(d, 0.0f));

                  if (distance > light.range) continue;
               }

               if (not cone_intersects(positionVS, light.directionVS,
                                       light.range, light.sin_angle,
                                       light.cos_angle, cluster_bounds.centreVS,
                                       length(cluster_bounds.sizeVS))) {
                  continue;
               }

               {
                  const shadows::Bounding_box cluster_bboxVS = {
                     .min = cluster_bounds.centreVS - cluster_bounds.sizeVS,
                     .max = cluster_bounds.centreVS + cluster_bounds.sizeVS,
                  };

                  bool box_outside = false;

                  box_outside |= (light.minVS.x >= cluster_bboxVS.max.x) |
                                 (light.maxVS.x <= cluster_bboxVS.min.x);
                  box_outside |= (light.minVS.y >= cluster_bboxVS.max.y) |
                                 (light.maxVS.y <= cluster_bboxVS.min.y);
                  box_outside |= (light.minVS.z >= cluster_bboxVS.max.z) |
                                 (light.maxVS.z <= cluster_bboxVS.min.z);

                  if (box_outside) continue;
               }

               Cluster& cluster = _xy_clusters[z][y][x];

               const std::size_t spot_lights_offset = cluster.info.point_light_count;

               if (cluster.info.spot_light_count + spot_lights_offset !=
                   cluster.lights.size()) {
                  cluster.lights[cluster.info.spot_light_count + spot_lights_offset] =
                     light_index;
                  cluster.info.spot_light_count += 1;
               }
            }
         }
      }
   }

#elif 0
   static bool cull_cone = true;
   static bool cull_sphere = true;
   static bool cull_bbox = true;

   for (int z = 0; z < _z_clusters; ++z) {
      const float cluster_endZ = _z_splits[z + 1];

      float cluster_width = 0.0f;
      float cluster_height = 0.0f;

      {
         const int x = 0;
         const int y = 0;

         std::array<glm::vec3, 2> directionsVS = {
            _tile_corner_directions[y][x],
            _tile_corner_directions[(y + 1)][x + 1],
         };

         std::array<float, 2> z_factors = {
            1.0f / directionsVS[0].z,
            1.0f / directionsVS[1].z,
         };

         std::array<glm::vec3, 4> cluster_far_cornersVS = {
            directionsVS[0] * cluster_endZ * z_factors[0],
            directionsVS[1] * cluster_endZ * z_factors[1],
         };

         cluster_width =
            std::abs(cluster_far_cornersVS[0].x - cluster_far_cornersVS[1].x);
         cluster_height =
            std::abs(cluster_far_cornersVS[0].y - cluster_far_cornersVS[1].y);
      }

      const float half_x_clusters = _x_clusters / 2.0f;
      const float half_y_clusters = _y_clusters / 2.0f;

      for (std::uint16_t light_index : _z_bins[z].point_lights) {
         const game_support::Point_light& light = point_lights[light_index];

         const glm::vec3 positionVS =
            glm::vec4{light.positionWS, 1.0f} * matrices.view_matrix;

         const float min_xVS = positionVS.x - light.range * 1.4142135623730950488f;
         const float max_xVS = positionVS.x + light.range * 1.4142135623730950488f;

         const float min_yVS = positionVS.y - light.range * 1.4142135623730950488f;
         const float max_yVS = positionVS.y + light.range * 1.4142135623730950488f;

         const int min_cluster_x =
            std::clamp((int)(min_xVS / cluster_width + half_x_clusters - 0.5f),
                       0, _x_clusters - 1);
         const int max_cluster_x =
            std::clamp((int)ceil_sse4_1(max_xVS / cluster_width + half_x_clusters),
                       0, _x_clusters - 1);

         const int min_cluster_y =
            std::clamp((int)(-max_yVS / cluster_height + half_y_clusters - 0.5f),
                       0, _y_clusters - 1);
         const int max_cluster_y =
            std::clamp((int)(ceil_sse4_1(-min_yVS / cluster_height + half_y_clusters)),
                       0, _y_clusters - 1);

         assert(min_cluster_x <= max_cluster_x);
         assert(min_cluster_y <= max_cluster_y);

         for (int y = min_cluster_y; y <= max_cluster_y; ++y) {
            for (int x = min_cluster_x; x <= max_cluster_x; ++x) {
               const Cluster_bounds& cluster_bounds = _cluster_bounds[z][y][x];

               const glm::vec3 d = abs(positionVS - cluster_bounds.centreVS) -
                                   cluster_bounds.sizeVS;

               const float distance = length(glm::max(d, 0.0f));

               if (distance <= light.range) {
                  Cluster& cluster = _xy_clusters[z][y][x];

                  if (cluster.info.point_light_count != cluster.lights.size()) {
                     cluster.lights[cluster.info.point_light_count++] = light_index;
                  }
               }
            }
         }
      }

      for (std::uint16_t light_index : _z_bins[z].spot_lights) {
         const game_support::Spot_light& light = spot_lights[light_index];

         const glm::vec3 positionVS =
            glm::vec4{light.positionWS, 1.0f} * matrices.view_matrix;

         const float min_xVS = positionVS.x - light.range * 1.4142135623730950488f;
         const float max_xVS = positionVS.x + light.range * 1.4142135623730950488f;

         const float min_yVS = positionVS.y - light.range * 1.4142135623730950488f;
         const float max_yVS = positionVS.y + light.range * 1.4142135623730950488f;

         const int min_cluster_x =
            std::clamp((int)(min_xVS / cluster_width + half_x_clusters - 0.5f),
                       0, _x_clusters - 1);
         const int max_cluster_x =
            std::clamp((int)ceil_sse4_1(max_xVS / cluster_width + half_x_clusters),
                       0, _x_clusters - 1);

         const int min_cluster_y =
            std::clamp((int)(-max_yVS / cluster_height + half_y_clusters - 0.5f),
                       0, _y_clusters - 1);
         const int max_cluster_y =
            std::clamp((int)(ceil_sse4_1(-min_yVS / cluster_height + half_y_clusters)),
                       0, _y_clusters - 1);

         assert(min_cluster_x <= max_cluster_x);
         assert(min_cluster_y <= max_cluster_y);

         for (int y = min_cluster_y; y <= max_cluster_y; ++y) {
            for (int x = min_cluster_x; x <= max_cluster_x; ++x) {
               const Cluster_bounds& cluster_bounds = _cluster_bounds[z][y][x];

               bool intersects = true;

               if (cull_cone)
                  intersects &=
                     cone_intersects(positionVS,
                                     glm::normalize(light.directionWS *
                                                    glm::mat3{matrices.view_matrix}),
                                     light.range, sinf(light.outer_angle),
                                     cosf(light.outer_angle), cluster_bounds.centreVS,
                                     length(cluster_bounds.sizeVS));

               if (cull_sphere) {
                  const glm::vec3 d = abs(positionVS - cluster_bounds.centreVS) -
                                      cluster_bounds.sizeVS;

                  const float distance = length(glm::max(d, 0.0f));

                  intersects &= (distance <= light.range);
               }

               if (cull_bbox) {
                  const shadows::Bounding_box bboxVS =
                     cone_bbox(positionVS,
                               glm::normalize(light.directionWS *
                                              glm::mat3{matrices.view_matrix}),
                               light.range, light.cone_radius);

                  const shadows::Bounding_box cluster_bboxVS = {
                     .min = cluster_bounds.centreVS - cluster_bounds.sizeVS,
                     .max = cluster_bounds.centreVS + cluster_bounds.sizeVS,
                  };

                  bool box_outside = false;

                  box_outside |= (bboxVS.min.x >= cluster_bboxVS.max.x) |
                                 (bboxVS.max.x <= cluster_bboxVS.min.x);
                  box_outside |= (bboxVS.min.y >= cluster_bboxVS.max.y) |
                                 (bboxVS.max.y <= cluster_bboxVS.min.y);
                  box_outside |= (bboxVS.min.z >= cluster_bboxVS.max.z) |
                                 (bboxVS.max.z <= cluster_bboxVS.min.z);

                  intersects &= not box_outside;
               }

               if (intersects) {
                  Cluster& cluster = _xy_clusters[z][y][x];

                  const std::size_t spot_lights_offset = cluster.info.point_light_count;

                  if (cluster.info.spot_light_count + spot_lights_offset !=
                      cluster.lights.size()) {
                     cluster.lights[cluster.info.spot_light_count + spot_lights_offset] =
                        light_index;
                     cluster.info.spot_light_count += 1;
                  }
               }
            }
         }
      }
   }
#else
   for (int z = 0; z < _z_clusters; ++z) {
      const float cluster_startZ = _z_splits[z];
      const float cluster_endZ = _z_splits[z + 1];

      for (int y = 0; y < _y_clusters; ++y) {
         for (int x = 0; x < _x_clusters; ++x) {
            std::array<glm::vec3, 4> directionsVS = {
               _tile_corner_directions[y * x_directions + x],
               _tile_corner_directions[y * x_directions + (x + 1)],
               _tile_corner_directions[(y + 1) * x_directions + x],
               _tile_corner_directions[(y + 1) * x_directions + (x + 1)],
            };

            std::array<float, 4> z_factors = {
               1.0f / directionsVS[0].z,
               1.0f / directionsVS[1].z,
               1.0f / directionsVS[2].z,
               1.0f / directionsVS[3].z,
            };

            std::array<glm::vec3, 8> cluster_cornersVS = {
               directionsVS[0] * cluster_startZ * z_factors[0],
               directionsVS[1] * cluster_startZ * z_factors[1],
               directionsVS[2] * cluster_startZ * z_factors[2],
               directionsVS[3] * cluster_startZ * z_factors[3],

               directionsVS[0] * cluster_endZ * z_factors[0],
               directionsVS[1] * cluster_endZ * z_factors[1],
               directionsVS[2] * cluster_endZ * z_factors[2],
               directionsVS[3] * cluster_endZ * z_factors[3],
            };

            glm::vec3 cluster_minVS = cluster_cornersVS[0];
            glm::vec3 cluster_maxVS = cluster_cornersVS[0];

            for (int corner = 1; corner < 8; ++corner) {
               cluster_minVS = glm::min(cluster_minVS, cluster_cornersVS[corner]);
               cluster_maxVS = glm::max(cluster_maxVS, cluster_cornersVS[corner]);
            }

            const glm::vec3 cluster_centreVS = (cluster_minVS + cluster_maxVS) * 0.5f;
            const glm::vec3 cluster_sizeVS = abs(cluster_maxVS - cluster_minVS) * 0.5f;

            for (std::uint16_t light_index : _z_bins[z]) {
               const game_support::Point_light& light = point_lights[light_index];

               const glm::vec3 positionVS =
                  glm::vec4{light.positionWS, 1.0f} * matrices.view_matrix;

               const glm::vec3 d = abs(positionVS - cluster_centreVS) - cluster_sizeVS;

               const float distance =
                  glm::length(glm::max(d, 0.0f)) +
                  std::min(std::max(std::max(d.x, d.y), d.z), 0.0f);

               if (distance <= light.range) {
                  Cluster& cluster = _xy_clusters[z][y * _x_clusters + x];

                  cluster.lights[cluster.light_count++] = light_index;

                  if (cluster.light_count == cluster.lights.size()) {
                     break;
                  }
               }
            }
         }
      }
   }
#endif
}
}