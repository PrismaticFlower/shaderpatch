
#include "terrain_model_segment.hpp"
#include "optimize_mesh.hpp"
#include "weld_vertex_list.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace sp {

namespace {
constexpr auto segment_grid_length = 8;

using Sorted_segments =
   std::array<std::array<Terrain_triangle_list, segment_grid_length>, segment_grid_length>;

auto sort_into_segments(const Terrain_triangle_list& triangles,
                        const float terrain_length) noexcept -> Sorted_segments
{
   Sorted_segments sorted;

   for (auto& row : sorted) {
      for (auto& list : row) {
         list.reserve(triangles.size() / (segment_grid_length * segment_grid_length));
      }
   }

   for (auto& tri : triangles) {
      const glm::vec2 centre =
         (tri[0].position.xz + tri[1].position.xz + tri[2].position.xz) / 3.0f;

      glm::ivec2 index =
         glm::trunc(glm::clamp(centre / terrain_length + 0.5f, 0.0f, 1.0f) *
                    float{segment_grid_length});
      index = glm::clamp(index, 0, segment_grid_length - 1);

      sorted[index.x][index.y].emplace_back(tri);
   }

   return sorted;
}

auto get_bbox(const Terrain_vertex_buffer& vertices) noexcept
   -> std::array<glm::vec3, 2>
{
   if (vertices.empty()) return std::array{glm::vec3{}, glm::vec3{}};

   std::array<glm::vec3, 2> bbox{vertices[0].position, vertices[0].position};

   std::for_each(vertices.cbegin() + 1, vertices.cend(),
                 [&](const Terrain_vertex& vertex) {
                    bbox[0] = glm::min(bbox[0], vertex.position);
                    bbox[1] = glm::max(bbox[1], vertex.position);
                 });

   return bbox;
}

auto get_max_xy_length(const Terrain_triangle_list& triangles) noexcept -> float
{
   float min = std::numeric_limits<float>::max();
   float max = std::numeric_limits<float>::min();

   for (const auto& tri : triangles) {
      for (const auto& v : tri) {
         min = std::min({min, v.position.x, v.position.z});
         max = std::max({max, v.position.x, v.position.z});
      }
   }

   return glm::distance(min, max);
}

}

auto create_terrain_model_segments(const Terrain_triangle_list& triangles)
   -> std::vector<Terrain_model_segment>
{
   const auto sorted_tris =
      sort_into_segments(triangles, get_max_xy_length(triangles));

   std::vector<Terrain_model_segment> segments;
   segments.reserve(segment_grid_length * segment_grid_length);

   for (auto& row : sorted_tris) {
      for (auto& tris : row) {
         auto indexed_tris = weld_vertex_list(tris);

         if (indexed_tris.second.size() > std::numeric_limits<std::uint16_t>::max()) {
            throw std::runtime_error{
               "Terrain has too many vertices to handle!"};
         }

         const auto bbox = get_bbox(indexed_tris.second);

         segments.push_back({shrink_index_buffer(indexed_tris.first),
                             std::move(indexed_tris.second), bbox});
      }
   }

   return segments;
}

auto create_terrain_low_detail_model_segment(const Terrain_triangle_list& triangles)
   -> Terrain_model_segment
{
   Terrain_model_segment segment;

   auto indexed_tris = weld_vertex_list(triangles);

   if (indexed_tris.second.size() > std::numeric_limits<std::uint16_t>::max()) {
      throw std::runtime_error{"Terrain has too many vertices to handle!"};
   }

   segment.bbox = get_bbox(indexed_tris.second);
   segment.indices = shrink_index_buffer(indexed_tris.first);
   segment.vertices = std::move(indexed_tris.second);

   return segment;
}

auto optimize_terrain_model_segments(std::vector<Terrain_model_segment> segments) noexcept
   -> std::vector<Terrain_model_segment>
{
   for (auto& segment : segments) {
      std::tie(segment.indices, segment.vertices) =
         optimize_mesh(std::move(segment.indices), std::move(segment.vertices));
   }

   return segments;
}

auto calculate_terrain_model_segments_aabb(const std::vector<Terrain_model_segment>& segments) noexcept
   -> std::array<glm::vec3, 2>
{
   if (segments.empty()) return {};

   std::array<glm::vec3, 2> aabb = segments[0].bbox;

   std::for_each(segments.cbegin() + 1, segments.cend(),
                 [&](const Terrain_model_segment& segment) {
                    aabb[0] = glm::min(aabb[0], segment.bbox[0]);
                    aabb[1] = glm::max(aabb[1], segment.bbox[1]);
                 });

   return aabb;
}
}
