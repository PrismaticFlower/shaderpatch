
#include "weld_vertex_list.hpp"

#include <execution>
#include <mutex>

#include <gsl/gsl>

namespace sp {

namespace {

constexpr auto pos_max_diff = 0.00025f;
constexpr auto normal_threshold = 1.0f - (1.f / 128.5f);
constexpr auto texcoords_max_diff = 1.f / 2048.f;
constexpr auto terrain_blend_threshold = (1.f / 255.5f);
constexpr auto terrain_color_threshold = (1.f / 255.5f);

auto init_vertex_buffer(const Vertex_buffer& old_vbuf) noexcept -> Vertex_buffer
{
   Vertex_buffer vertex_buffer{};

   if (old_vbuf.positions)
      vertex_buffer.positions = std::make_unique<glm::vec3[]>(old_vbuf.count);

   if (old_vbuf.blendindices)
      vertex_buffer.blendindices = std::make_unique<glm::uint32[]>(old_vbuf.count);

   if (old_vbuf.blendweights)
      vertex_buffer.blendweights = std::make_unique<glm::vec3[]>(old_vbuf.count);

   if (old_vbuf.normals)
      vertex_buffer.normals = std::make_unique<glm::vec3[]>(old_vbuf.count);

   if (old_vbuf.tangents)
      vertex_buffer.tangents = std::make_unique<glm::vec3[]>(old_vbuf.count);

   if (old_vbuf.bitangent_signs)
      vertex_buffer.bitangent_signs = std::make_unique<float[]>(old_vbuf.count);

   if (old_vbuf.binormals)
      vertex_buffer.binormals = std::make_unique<glm::vec3[]>(old_vbuf.count);

   if (old_vbuf.colors)
      vertex_buffer.colors = std::make_unique<glm::uint32[]>(old_vbuf.count);

   if (old_vbuf.static_lighting_colors)
      vertex_buffer.static_lighting_colors =
         std::make_unique<glm::uint32[]>(old_vbuf.count);

   if (old_vbuf.texcoords)
      vertex_buffer.texcoords = std::make_unique<glm::vec2[]>(old_vbuf.count);

   return vertex_buffer;
}

auto push_back_vertex(const Vertex_buffer& src_vbuf, const int src_index,
                      Vertex_buffer& dest_vbuf) noexcept -> int
{
   const auto index = dest_vbuf.count;

   if (src_vbuf.positions)
      dest_vbuf.positions[index] = src_vbuf.positions[src_index];

   if (src_vbuf.blendindices)
      dest_vbuf.blendindices[index] = src_vbuf.blendindices[src_index];

   if (src_vbuf.blendweights)
      dest_vbuf.blendweights[index] = src_vbuf.blendweights[src_index];

   if (src_vbuf.normals) dest_vbuf.normals[index] = src_vbuf.normals[src_index];

   if (src_vbuf.tangents)
      dest_vbuf.tangents[index] = src_vbuf.tangents[src_index];

   if (src_vbuf.bitangent_signs)
      dest_vbuf.bitangent_signs[index] = src_vbuf.bitangent_signs[src_index];

   if (src_vbuf.binormals)
      dest_vbuf.binormals[index] = src_vbuf.binormals[src_index];

   if (src_vbuf.colors) dest_vbuf.colors[index] = src_vbuf.colors[src_index];

   if (src_vbuf.static_lighting_colors)
      dest_vbuf.static_lighting_colors[index] =
         src_vbuf.static_lighting_colors[src_index];

   if (src_vbuf.texcoords)
      dest_vbuf.texcoords[index] = src_vbuf.texcoords[src_index];

   return gsl::narrow_cast<int>(dest_vbuf.count++);
}

bool is_vertex_similar(const Vertex_buffer& left_vbuf, const int left_index,
                       const Vertex_buffer& right_vbuf, const int right_index) noexcept
{
   if (left_vbuf.positions) {
      if (glm::distance(left_vbuf.positions[left_index],
                        right_vbuf.positions[right_index]) > pos_max_diff)
         return false;
   }

   if (left_vbuf.blendindices) {
      if (left_vbuf.blendindices[left_index] != right_vbuf.blendindices[right_index])
         return false;
   }

   if (left_vbuf.blendweights) {
      if (left_vbuf.blendweights[left_index] != right_vbuf.blendweights[right_index])
         return false;
   }

   if (left_vbuf.normals) {
      if (glm::dot(left_vbuf.normals[left_index],
                   right_vbuf.normals[right_index]) < normal_threshold)
         return false;
   }

   if (left_vbuf.tangents) {
      if (glm::dot(left_vbuf.tangents[left_index],
                   right_vbuf.tangents[right_index]) < normal_threshold)
         return false;
   }

   if (left_vbuf.bitangent_signs) {
      if (left_vbuf.bitangent_signs[left_index] > right_vbuf.bitangent_signs[right_index])
         return false;
   }

   if (left_vbuf.binormals) {
      if (glm::dot(left_vbuf.binormals[left_index],
                   right_vbuf.binormals[right_index]) < normal_threshold)
         return false;
   }

   if (left_vbuf.colors) {
      if (left_vbuf.colors[left_index] != right_vbuf.colors[right_index])
         return false;
   }

   if (left_vbuf.static_lighting_colors) {
      if (left_vbuf.static_lighting_colors[left_index] !=
          right_vbuf.static_lighting_colors[right_index])
         return false;
   }

   if (left_vbuf.texcoords) {
      if (glm::distance(left_vbuf.texcoords[left_index],
                        right_vbuf.texcoords[right_index]) > texcoords_max_diff)
         return false;
   }

   return true;
}

bool is_vertex_similar(const Terrain_vertex& left, const Terrain_vertex& right) noexcept
{
   if (left.texture_indices != right.texture_indices) return false;

   if (glm::distance(left.position, right.position) > pos_max_diff)
      return false;

   if (glm::any(glm::greaterThan(glm::vec2{glm::distance(left.texture_blend[0],
                                                         right.texture_blend[0]),
                                           glm::distance(left.texture_blend[1],
                                                         right.texture_blend[1])},
                                 glm::vec2{terrain_blend_threshold})))
      return false;

   if (glm::any(glm::greaterThan(glm::abs(right.diffuse_lighting - left.diffuse_lighting),
                                 glm::vec3{terrain_color_threshold})))
      return false;

   if (glm::any(glm::greaterThan(glm::abs(right.base_color - left.base_color),
                                 glm::vec3{terrain_color_threshold})))
      return false;

   return false;
}

auto find_similar_vertex(const Vertex_buffer& ref_vbuf, const int ref_index,
                         const Vertex_buffer& in_vbuf) noexcept -> int
{
   for (auto v = 0; v < in_vbuf.count; ++v) {
      if (is_vertex_similar(ref_vbuf, ref_index, in_vbuf, v)) return v;
   }

   return -1;
}

auto find_similar_vertex(const Terrain_vertex& ref_vertex,
                         const Terrain_vertex_buffer& vertices) noexcept -> int
{
   for (auto v = 0; v < vertices.size(); ++v) {
      if (is_vertex_similar(ref_vertex, vertices[v])) return v;
   }

   return -1;
}

auto add_terrain_vertex(Terrain_vertex_buffer& buffer, const Terrain_vertex& vertex)
   -> std::uint32_t
{
   if (const auto index = find_similar_vertex(vertex, buffer); index != -1) {
      return static_cast<std::uint32_t>(index);
   }
   else {
      buffer.push_back(vertex);

      return static_cast<std::uint32_t>(buffer.size() - 1);
   }
}
}

auto weld_vertex_list(const Vertex_buffer& vertex_buffer) noexcept
   -> std::pair<Index_buffer_16, Vertex_buffer>
{
   Expects((vertex_buffer.count % 3u) == 0u);

   Index_buffer_16 ibuf;
   auto welded_vbuf = init_vertex_buffer(vertex_buffer);

   const auto face_count = vertex_buffer.count / 3u;

   for (auto f = 0; f < face_count; ++f) {
      auto& tri_index = ibuf.emplace_back();

      for (auto v = 0; v < 3; ++v) {
         if (const auto index =
                find_similar_vertex(vertex_buffer, f * 3 + v, welded_vbuf);
             index != -1) {
            tri_index[v] = static_cast<std::uint16_t>(index);
         }
         else {
            tri_index[v] = static_cast<std::uint16_t>(
               push_back_vertex(vertex_buffer, f * 3 + v, welded_vbuf));
         }
      }
   }

   return {std::move(ibuf), std::move(welded_vbuf)};
}

auto weld_vertex_list(const Terrain_triangle_list& triangles) noexcept
   -> std::pair<Index_buffer_32, Terrain_vertex_buffer>
{
   Expects((triangles.size() * 3) < std::numeric_limits<std::int32_t>::max());

   std::pair<Index_buffer_32, Terrain_vertex_buffer> result;
   auto& [indices, vertices] = result;

   vertices.reserve(triangles.size() * 3);
   indices.reserve(triangles.size());

   for (auto& tri : triangles) {
      indices.push_back({add_terrain_vertex(vertices, tri[0]),
                         add_terrain_vertex(vertices, tri[1]),
                         add_terrain_vertex(vertices, tri[2])});
   }

   return result;
}
}
