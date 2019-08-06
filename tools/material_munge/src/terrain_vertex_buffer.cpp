#include "terrain_vertex_buffer.hpp"
#include "generate_tangents.hpp"
#include "image_span.hpp"
#include "vertex_buffer.hpp"

#include <algorithm>
#include <execution>
#include <iterator>
#include <memory>
#include <stdexcept>

#include <glm/gtc/color_space.hpp>

namespace sp {

namespace {
struct Packed_output_terrain_vertex {
   glm::i16vec3 position;
   glm::uint16 texture_indices;
   glm::uint32 normal;
   glm::uint32 tangent;
};

static_assert(sizeof(Packed_output_terrain_vertex) == 16);

constexpr auto packed_vertex_flags =
   Vbuf_flags::position | Vbuf_flags::normal | Vbuf_flags::tangents |
   Vbuf_flags::texcoords | Vbuf_flags::position_compressed |
   Vbuf_flags::normal_compressed | Vbuf_flags::texcoord_compressed;

auto select_terrain_tri_textures(const Terrain_map& terrain,
                                 const std::array<int, 3> tri)
   -> std::array<std::uint8_t, 3>
{
   std::array tri_weights{terrain.texture_weights[tri[0]],
                          terrain.texture_weights[tri[1]],
                          terrain.texture_weights[tri[2]]};

   for (auto& weights : tri_weights) {
      for (std::ptrdiff_t i = 0; i < weights.size() - 1; ++i) {
         weights[i] *= (1.0f - weights[i + 1]);
      }
   }

   std::array<float, 16> total_tri_weights{};

   for (auto i = 0; i < 16; ++i) {
      total_tri_weights[i] += tri_weights[0][i];
      total_tri_weights[i] += tri_weights[1][i];
      total_tri_weights[i] += tri_weights[2][i];
   }

   std::array<std::uint8_t, 3> indices;

   const auto select_index = [&](const std::ptrdiff_t start) {
      for (std::ptrdiff_t i = start; i > 0; --i) {
         if (total_tri_weights[i] <= 0.0f) continue;

         return static_cast<std::uint8_t>(i);
      }

      return std::uint8_t{0};
   };

   indices[0] = select_index(total_tri_weights.size() - 1);
   indices[1] = select_index(int{indices[0]} - 1);
   indices[2] = select_index(int{indices[1]} - 1);

   return indices;
}

auto fetch_texture_weights(const std::array<float, 16>& weights,
                           const std::array<std::uint8_t, 3> indices) noexcept
   -> std::array<float, 2>
{
   auto premult_weights = weights;

   for (std::ptrdiff_t i = 0; i < weights.size() - 1; ++i) {
      premult_weights[i] *= (1.0f - premult_weights[i + 1]);
   }

   return {premult_weights[indices[0]], premult_weights[indices[1]]};
}

auto create_terrain_triangles(const Terrain_map& terrain) -> Terrain_triangle_list
{
   Terrain_triangle_list tris;
   tris.reserve((terrain.length - 1) * (terrain.length - 1) * 2);

   for (auto y = 0; y < (terrain.length - 1); ++y) {
      for (auto x = 0; x < (terrain.length - 1); ++x) {
         const auto i0 = x + (y * terrain.length);
         const auto i1 = x + ((y + 1) * terrain.length);
         const auto i2 = (x + 1) + (y * terrain.length);
         const auto i3 = (x + 1) + ((y + 1) * terrain.length);

         Terrain_vertex v0;
         Terrain_vertex v1;
         Terrain_vertex v2;
         Terrain_vertex v3;

         v0.position = terrain.position[i0];
         v0.diffuse_lighting = terrain.diffuse_lighting[i0];
         v0.base_color = terrain.color[i0];

         v1.position = terrain.position[i1];
         v1.diffuse_lighting = terrain.diffuse_lighting[i1];
         v1.base_color = terrain.color[i1];

         v2.position = terrain.position[i2];
         v2.diffuse_lighting = terrain.diffuse_lighting[i2];
         v2.base_color = terrain.color[i2];

         v3.position = terrain.position[i3];
         v3.diffuse_lighting = terrain.diffuse_lighting[i3];
         v3.base_color = terrain.color[i3];

         auto& tri0 = tris.emplace_back(std::array{v0, v2, v3});
         auto& tri1 = tris.emplace_back(std::array{v0, v3, v1});

         tri0[0].texture_indices = tri0[1].texture_indices = tri0[2].texture_indices =
            select_terrain_tri_textures(terrain, {i0, i2, i3});
         tri1[0].texture_indices = tri1[1].texture_indices = tri1[2].texture_indices =
            select_terrain_tri_textures(terrain, {i0, i3, i1});

         tri0[0].texture_blend = fetch_texture_weights(terrain.texture_weights[i0],
                                                       tri0[0].texture_indices);
         tri0[1].texture_blend = fetch_texture_weights(terrain.texture_weights[i2],
                                                       tri0[0].texture_indices);
         tri0[2].texture_blend = fetch_texture_weights(terrain.texture_weights[i3],
                                                       tri0[0].texture_indices);

         tri1[0].texture_blend = fetch_texture_weights(terrain.texture_weights[i0],
                                                       tri1[0].texture_indices);
         tri1[1].texture_blend = fetch_texture_weights(terrain.texture_weights[i3],
                                                       tri1[0].texture_indices);
         tri1[2].texture_blend = fetch_texture_weights(terrain.texture_weights[i1],
                                                       tri1[0].texture_indices);
      }
   }

   return tris;
}

auto pack_vertex(const Terrain_vertex& vertex,
                 const Vertex_position_compress& pos_compress,
                 const bool pack_lighting) noexcept -> Packed_output_terrain_vertex
{
   Packed_output_terrain_vertex packed{};

   packed.position = pos_compress(vertex.position);

   packed.texture_indices |= (vertex.texture_indices[0] & 0xf);
   packed.texture_indices |= (vertex.texture_indices[1] & 0xf) << 4;
   packed.texture_indices |= (vertex.texture_indices[2] & 0xf) << 8;

   const auto pack_unorm = [](const float v) -> glm::uint {
      return static_cast<glm::uint>(glm::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
   };

   packed.normal |= pack_unorm(vertex.texture_blend[0]) << 16;
   packed.normal |= pack_unorm(vertex.texture_blend[1]) << 8;

   const auto srgb_color = glm::convertLinearToSRGB(
      pack_lighting ? vertex.diffuse_lighting : glm::vec3{0.0f});

   packed.tangent |= pack_unorm(srgb_color.r) << 16;
   packed.tangent |= pack_unorm(srgb_color.g) << 8;
   packed.tangent |= pack_unorm(srgb_color.b);

   return packed;
}
}

auto create_terrain_triangle_list(const Terrain_map& terrain) -> Terrain_triangle_list
{
   if (terrain.length < 2) return {};

   return create_terrain_triangles(terrain);
}

void output_vertex_buffer(const Terrain_vertex_buffer& vertex_buffer,
                          ucfb::Editor_data_writer& writer,
                          const std::array<glm::vec3, 2> vert_box,
                          const bool pack_lighting)
{
   writer.write<std::uint32_t, std::uint32_t>(static_cast<std::uint32_t>(
                                                 vertex_buffer.size()),
                                              sizeof(Packed_output_terrain_vertex),
                                              packed_vertex_flags);

   const Vertex_position_compress pos_compress{vert_box};

   for (const auto& vertex : vertex_buffer) {
      writer.write(pack_vertex(vertex, pos_compress, true));
   }

   writer.write(std::uint64_t{}); // trailing unused texcoords & binormal
}
}
