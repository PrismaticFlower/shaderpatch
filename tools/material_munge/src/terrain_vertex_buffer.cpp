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

auto select_textures(const Terrain_map& terrain, const std::array<glm::ivec2, 3> tri)
   -> std::pair<std::array<std::uint8_t, 3>, std::array<std::array<float, 2>, 3>>
{
   Expects(terrain.length > 1);

   using Weights = std::array<std::array<std::array<float, 16>, 5>, 3>;

   const Weights texture_weights = [&] {
      Weights texture_weights{};

      for (auto v = 0; v < tri.size(); ++v) {
         const auto clamp = [max = terrain.length - 1](const auto v) {
            return std::clamp(v, 0, max);
         };

         const auto i0 = clamp(tri[v].x) + (clamp(tri[v].y) * terrain.length);
         const auto i1 = clamp(tri[v].x - 1) + (clamp(tri[v].y) * terrain.length);
         const auto i2 = clamp(tri[v].x + 1) + (clamp(tri[v].y) * terrain.length);
         const auto i3 = clamp(tri[v].x) + (clamp(tri[v].y - 1) * terrain.length);
         const auto i4 = clamp(tri[v].x) + (clamp(tri[v].y + 1) * terrain.length);

         texture_weights[v][0] = terrain.texture_weights[i0];
         texture_weights[v][1] = terrain.texture_weights[i1];
         texture_weights[v][2] = terrain.texture_weights[i2];
         texture_weights[v][3] = terrain.texture_weights[i3];
         texture_weights[v][4] = terrain.texture_weights[i4];
      }

      return texture_weights;
   }();

   const Weights premult_weights = [&] {
      Weights premult_weights = texture_weights;

      for (auto& vertex_weights : premult_weights) {
         for (auto& weights : vertex_weights) {
            for (std::ptrdiff_t i = static_cast<std::ptrdiff_t>(weights.size()) - 2;
                 i >= 0; --i) {
               weights[i] *= (1.0f - weights[i + 1]);
            }

            const auto sum = std::accumulate(weights.cbegin(), weights.cend(), 0.0f);

            if (sum > 0.0f) {
               for (auto& weight : weights) {
                  weight /= sum;
               }
            }
         }
      }

      return premult_weights;
   }();

   const std::array<float, 16> summed_weights = [&] {
      std::array<float, 16> summed_weights{};

      for (auto& vertex_weights : premult_weights) {
         for (const auto& weights : vertex_weights) {
            for (std::ptrdiff_t i = 0; i < weights.size(); ++i) {
               summed_weights[i] += weights[i];
            }
         }
      }

      return summed_weights;
   }();

   const std::array<std::uint_fast8_t, 16> sorted_indices = [&] {
      std::array<std::uint_fast8_t, 16> sorted_indices{};

      std::iota(sorted_indices.begin(), sorted_indices.end(), std::uint_fast8_t{0});
      std::sort(sorted_indices.begin(), sorted_indices.end(),
                [&](const std::uint_fast8_t& l, const std::uint_fast8_t& r) {
                   return summed_weights[l] > summed_weights[r];
                });

      return sorted_indices;
   }();

   std::array indices = {static_cast<std::uint8_t>(sorted_indices[0]),
                         static_cast<std::uint8_t>(sorted_indices[1]),
                         static_cast<std::uint8_t>(sorted_indices[2])};

   std::array tri_weights = {std::array{premult_weights[0][0][indices[0]],
                                        premult_weights[0][0][indices[1]],
                                        premult_weights[0][0][indices[2]]},
                             std::array{premult_weights[1][0][indices[0]],
                                        premult_weights[1][0][indices[1]],
                                        premult_weights[1][0][indices[2]]},
                             std::array{premult_weights[2][0][indices[0]],
                                        premult_weights[2][0][indices[1]],
                                        premult_weights[2][0][indices[2]]}};

   return {indices, std::array{std::array{tri_weights[0][0], tri_weights[0][1]},
                               std::array{tri_weights[1][0], tri_weights[1][1]},
                               std::array{tri_weights[2][0], tri_weights[2][1]}}};
}

auto create_terrain_triangles(const Terrain_map& terrain) -> Terrain_triangle_list
{
   Terrain_triangle_list tris;
   tris.reserve((terrain.length - 1) * (terrain.length - 1) * 2);

   for (auto y = 0; y < (terrain.length - 1); ++y) {
      for (auto x = 0; x < (terrain.length - 1); ++x) {
         const glm::ivec2 xy0{x, y};
         const glm::ivec2 xy1{x, y + 1};
         const glm::ivec2 xy2{x + 1, y};
         const glm::ivec2 xy3{x + 1, y + 1};

         const auto i0 = xy0.x + (xy0.y * terrain.length);
         const auto i1 = xy1.x + (xy1.y * terrain.length);
         const auto i2 = xy2.x + (xy2.y * terrain.length);
         const auto i3 = xy3.x + (xy3.y * terrain.length);

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

         {
            auto& tri0 = tris.emplace_back(std::array{v0, v2, v3});

            auto [tri0_tex_indices, tri0_tex_weights] =
               select_textures(terrain, {xy0, xy2, xy3});

            tri0[0].texture_indices = tri0[1].texture_indices =
               tri0[2].texture_indices = tri0_tex_indices;

            tri0[0].texture_blend = tri0_tex_weights[0];
            tri0[1].texture_blend = tri0_tex_weights[1];
            tri0[2].texture_blend = tri0_tex_weights[2];
         }

         {
            auto& tri1 = tris.emplace_back(std::array{v0, v3, v1});

            auto [tri1_tex_indices, tri1_tex_weights] =
               select_textures(terrain, {xy0, xy3, xy1});

            tri1[0].texture_indices = tri1[1].texture_indices =
               tri1[2].texture_indices = tri1_tex_indices;

            tri1[0].texture_blend = tri1_tex_weights[0];
            tri1[1].texture_blend = tri1_tex_weights[1];
            tri1[2].texture_blend = tri1_tex_weights[2];
         }
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

   auto triangles = create_terrain_triangles(terrain);

   for (const auto& cut : terrain.cuts) {
      cut.apply(triangles);
   }

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
      writer.write(pack_vertex(vertex, pos_compress, pack_lighting));
   }

   writer.write(std::uint64_t{}); // trailing unused texcoords & binormal
}
}
