
#include "weld_vertex_list.hpp"

#include <absl/container/flat_hash_map.h>

#include <bit>

namespace sp {

namespace {

struct Hashable_float {
   Hashable_float() = default;

   Hashable_float(const float v)
   {
      _v = (v == 0.0f or v != v) ? 0 : std::bit_cast<std::uint32_t>(v);
   }

   bool operator==(const Hashable_float&) const noexcept = default;

   template<typename H>
   friend H AbslHashValue(H h, const Hashable_float& cached)
   {
      return H::combine(std::move(h), cached._v);
   }

private:
   uint32_t _v = 0;
};

struct Hashable_vec2 {
   Hashable_vec2() = default;

   Hashable_vec2(const glm::vec2& v)
   {
      _x = (v.x == 0.0f or v.x != v.x) ? 0 : std::bit_cast<std::uint32_t>(v.x);
      _y = (v.y == 0.0f or v.y != v.y) ? 0 : std::bit_cast<std::uint32_t>(v.y);
   }

   bool operator==(const Hashable_vec2&) const noexcept = default;

   template<typename H>
   friend H AbslHashValue(H h, const Hashable_vec2& cached)
   {
      return H::combine(std::move(h), cached._x, cached._y);
   }

private:
   uint32_t _x = 0;
   uint32_t _y = 0;
};

struct Hashable_vec3 {
   Hashable_vec3() = default;

   Hashable_vec3(const glm::vec3& v)
   {
      _x = (v.x == 0.0f or v.x != v.x) ? 0 : std::bit_cast<std::uint32_t>(v.x);
      _y = (v.y == 0.0f or v.y != v.y) ? 0 : std::bit_cast<std::uint32_t>(v.y);
      _z = (v.z == 0.0f or v.z != v.z) ? 0 : std::bit_cast<std::uint32_t>(v.z);
   }

   bool operator==(const Hashable_vec3&) const noexcept = default;

   template<typename H>
   friend H AbslHashValue(H h, const Hashable_vec3& cached)
   {
      return H::combine(std::move(h), cached._x, cached._y, cached._z);
   }

private:
   uint32_t _x = 0;
   uint32_t _y = 0;
   uint32_t _z = 0;
};

struct Vertex {
   Hashable_vec3 position = {};
   std::uint32_t blendindices = 0;
   Hashable_vec3 blendweights = {};
   Hashable_vec3 normal = {};
   Hashable_vec3 tangent = {};
   Hashable_float bitangent_sign = {};
   Hashable_vec3 binormal = {};
   std::uint32_t color = 0;
   std::uint32_t static_lighting_color = 0;
   Hashable_vec2 texcoords = {};

   Vertex(const Vertex_buffer& vbuf, std::size_t index)
   {
      if (index >= vbuf.count) return;

      if (vbuf.positions) position = vbuf.positions[index];

      if (vbuf.blendindices) blendindices = vbuf.blendindices[index];

      if (vbuf.blendweights) blendweights = vbuf.blendweights[index];

      if (vbuf.normals) normal = vbuf.normals[index];

      if (vbuf.tangents) tangent = vbuf.tangents[index];

      if (vbuf.bitangent_signs) bitangent_sign = vbuf.bitangent_signs[index];

      if (vbuf.binormals) binormal = vbuf.binormals[index];

      if (vbuf.colors) color = vbuf.colors[index];

      if (vbuf.static_lighting_colors) {
         static_lighting_color = vbuf.static_lighting_colors[index];
      }

      if (vbuf.texcoords) texcoords = vbuf.texcoords[index];
   }

   bool operator==(const Vertex&) const noexcept = default;

   template<typename H>
   friend H AbslHashValue(H h, const Vertex& v)
   {
      return H::combine(std::move(h), v.position, v.blendindices, v.blendweights,
                        v.normal, v.tangent, v.bitangent_sign, v.binormal,
                        v.color, v.static_lighting_color, v.texcoords);
   }
};

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

auto push_back_vertex(const Vertex_buffer& src_vbuf, const std::size_t src_index,
                      Vertex_buffer& dest_vbuf) noexcept -> std::uint16_t
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

   return static_cast<std::uint16_t>(dest_vbuf.count++);
}

}

auto weld_vertex_list(const Vertex_buffer& vertex_buffer) noexcept
   -> std::pair<Index_buffer_16, Vertex_buffer>
{
   Expects((vertex_buffer.count % 3u) == 0u);

   Index_buffer_16 ibuf;
   auto welded_vbuf = init_vertex_buffer(vertex_buffer);

   absl::flat_hash_map<Vertex, std::uint16_t> vertex_cache;
   vertex_cache.reserve(vertex_buffer.count);

   const auto face_count = vertex_buffer.count / 3u;

   for (std::size_t f = 0; f < face_count; ++f) {
      auto& tri_index = ibuf.emplace_back();

      for (std::size_t v = 0; v < 3; ++v) {
         if (const auto it = vertex_cache.find(Vertex{vertex_buffer, f * 3 + v});
             it != vertex_cache.end()) {
            tri_index[v] = it->second;
         }
         else {
            tri_index[v] = push_back_vertex(vertex_buffer, f * 3 + v, welded_vbuf);

            vertex_cache.emplace(Vertex{vertex_buffer, f * 3 + v}, tri_index[v]);
         }
      }
   }

   return {std::move(ibuf), std::move(welded_vbuf)};
}
}
