
#include "terrain_model_segment.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <absl/container/inlined_vector.h>

namespace sp {

namespace {

const std::size_t segment_split_size = std::numeric_limits<std::uint16_t>::max() - 3;

struct Patch_info {
   std::array<std::uint8_t, 3> textures = {0, 0, 0};
   std::int16_t height_min = 0;
   std::int16_t height_max = 0;
   std::int16_t height_shift = 0;
};

struct Stock_terrain_vertex {
   glm::i16vec3 position;
   glm::uint16 texture_weight;
   glm::uint32 normal;
   glm::uint32 color;
};

static_assert(sizeof(Stock_terrain_vertex) == 16);

struct Stock_terrain_uncompressed_vertex {
   glm::vec3 position;
   glm::vec3 normal;
   glm::uint32 color;
};

static_assert(sizeof(Stock_terrain_uncompressed_vertex) == 28);

struct Terrain_process_vertex {
   glm::vec3 position;
   Terrain_vertex vertex;
};

struct Texture_info {
   std::array<std::uint8_t, 3> indices;
   std::array<std::uint8_t, 3> weights;
};

auto clamp(const int v, const Terrain_map& map) -> int
{
   return std::clamp(v, 0, map.length - 1);
}

auto sample_normal(const glm::vec3& position, const Terrain_map& map) -> glm::vec3
{
   const float length_half = (map.length / 2.0f);
   const float x = ((position.x / map.grid_scale) + length_half);
   const float z = ((position.z / map.grid_scale) + length_half - 1.0f);

   const int x_int = static_cast<int>(x);
   const int z_int = static_cast<int>(z);

   if (std::round(x) == x and std::round(z) == z) {
      return map.normal_map[clamp(z_int, map) * map.length + clamp(x_int, map)];
   }

   glm::vec3 normal_x0_z0 =
      map.normal_map[clamp(z_int + 0, map) * map.length + clamp(x_int + 0, map)];
   glm::vec3 normal_x1_z0 =
      map.normal_map[clamp(z_int + 0, map) * map.length + clamp(x_int + 1, map)];
   glm::vec3 normal_x0_z1 =
      map.normal_map[clamp(z_int + 1, map) * map.length + clamp(x_int + 0, map)];
   glm::vec3 normal_x1_z1 =
      map.normal_map[clamp(z_int + 1, map) * map.length + clamp(x_int + 1, map)];

   const float x0_weight = x - std::floor(x);
   const float x1_weight = 1.0f - x0_weight;
   const float z0_weight = z - std::floor(z);
   const float z1_weight = 1.0f - z0_weight;

   return normal_x0_z0 * x0_weight * z0_weight + normal_x1_z0 * x1_weight * z0_weight +
          normal_x0_z1 * x0_weight * z1_weight + normal_x1_z1 * x1_weight * z1_weight;
}

auto sample_weights(const glm::vec3& position, const Terrain_map& map)
   -> std::array<float, 16>
{
   const float length_half = (map.length / 2.0f);
   const float x = ((position.x / map.grid_scale) + length_half);
   const float z = ((position.z / map.grid_scale) + length_half - 1.0f);

   const auto unorm_weights = [](std::array<std::uint8_t, 16> uint_weights) {
      std::array<float, 16> weights{};

      for (std::ptrdiff_t i = 0; i < uint_weights.size(); ++i) {
         weights[i] = uint_weights[i] / 255.0f;
      }

      return weights;
   };

   const int x_int = static_cast<int>(x);
   const int z_int = static_cast<int>(z);

   if (std::round(x) == x and std::round(z) == z) {
      return unorm_weights(
         map.texture_weights[clamp(z_int, map) * map.length + clamp(x_int, map)]);
   }

   std::array<std::uint8_t, 16> uint_weights_x0_z0 =
      map.texture_weights[clamp(z_int + 0, map) * map.length + clamp(x_int + 0, map)];
   std::array<std::uint8_t, 16> uint_weights_x1_z0 =
      map.texture_weights[clamp(z_int + 0, map) * map.length + clamp(x_int + 1, map)];
   std::array<std::uint8_t, 16> uint_weights_x0_z1 =
      map.texture_weights[clamp(z_int + 1, map) * map.length + clamp(x_int + 0, map)];
   std::array<std::uint8_t, 16> uint_weights_x1_z1 =
      map.texture_weights[clamp(z_int + 1, map) * map.length + clamp(x_int + 1, map)];

   std::array<float, 16> weights_x0_z0 = unorm_weights(uint_weights_x0_z0);
   std::array<float, 16> weights_x1_z0 = unorm_weights(uint_weights_x1_z0);
   std::array<float, 16> weights_x0_z1 = unorm_weights(uint_weights_x0_z1);
   std::array<float, 16> weights_x1_z1 = unorm_weights(uint_weights_x1_z1);

   const float x0_weight = x - std::floor(x);
   const float x1_weight = 1.0f - x0_weight;
   const float z0_weight = z - std::floor(z);
   const float z1_weight = 1.0f - z0_weight;

   std::array<float, 16> weights{};

   for (std::size_t i = 0; i < weights.size(); ++i) {
      weights[i] = weights_x0_z0[i] * x0_weight * z0_weight +
                   weights_x1_z0[i] * x1_weight * z0_weight +
                   weights_x0_z1[i] * x0_weight * z1_weight +
                   weights_x1_z1[i] * x1_weight * z1_weight;
   }

   return weights;
}

auto pick_textures(const std::array<glm::vec3, 3>& positions, const Terrain_map& map)
   -> std::array<std::uint8_t, 3>
{
   std::array<float, 16> total_weights = {};

   for (const glm::vec3& position : positions) {
      for (int z = 0; z < 3; ++z) {
         for (int x = 0; x < 3; ++x) {
            std::array<float, 16> weights =
               sample_weights(position + glm::vec3{map.grid_scale * (x - 1), 0.0f,
                                                   map.grid_scale * (z - 1)},
                              map);

            for (std::ptrdiff_t i = std::ssize(weights) - 2; i >= 0; --i) {
               weights[i] *= (1.0f - weights[i + 1]);
            }

            for (std::size_t i = 0; i < weights.size(); ++i) {
               total_weights[i] += weights[i];
            }
         }
      }
   }

   std::array<std::uint8_t, 16> texture_indices = {0,  1,  2,  3, 4,  5,
                                                   6,  7,  8,  9, 10, 11,
                                                   12, 13, 14, 15};

   std::sort(texture_indices.begin(), texture_indices.end(),
             [&](const std::uint8_t l, const std::uint8_t r) {
                return total_weights[l] > total_weights[r];
             });

   std::array<std::uint8_t, 3> picked = {texture_indices[0], texture_indices[1],
                                         texture_indices[2]};

   std::sort(picked.begin(), picked.end());

   return picked;
}

auto read_info(ucfb::Reader_strict<"INFO"_mn> info) -> Patch_info
{
   Patch_info result;

   std::int8_t texture_count = info.read_unaligned<std::uint8_t>();

   for (std::ptrdiff_t i = 0; i < texture_count; ++i) {
      std::uint8_t texture_index = info.read_unaligned<std::uint8_t>();

      if (i >= result.textures.size()) continue;

      result.textures[i] = texture_index;
   }

   for (std::ptrdiff_t i = 2; i > (texture_count - 1); --i) {
      result.textures[i] = result.textures[0];
   }

   result.height_min = info.read_unaligned<std::int16_t>();
   result.height_max = info.read_unaligned<std::int16_t>();
   result.height_shift = info.read_unaligned<std::int16_t>();

   return result;
}

auto read_vbuf(ucfb::Reader_strict<"VBUF"_mn> vbuf, const glm::vec3& patch_offset)
   -> std::vector<Stock_terrain_uncompressed_vertex>
{
   const auto [count, stride, flags] =
      vbuf.read_multi<std::uint32_t, std::uint32_t, Vbuf_flags>();

   if (stride != 28 || flags != (Vbuf_flags::position | Vbuf_flags::normal |
                                 Vbuf_flags::static_lighting)) {
      throw std::runtime_error{"Terrain VBUF has unexpected stride or flags! "
                               "It is quite amazing you managed to hit this."};
   }

   std::vector<Stock_terrain_uncompressed_vertex> vertices;

   vertices.reserve(count);

   for (std::size_t i = 0; i < count; ++i) {
      Stock_terrain_uncompressed_vertex vertex =
         vbuf.read<Stock_terrain_uncompressed_vertex>();

      vertex.position += patch_offset;

      vertices.push_back(vertex);
   }

   return vertices;
}

auto convert_mesh(std::span<const Stock_terrain_uncompressed_vertex> stock_vertices,
                  std::span<const std::array<std::uint16_t, 3>> triangles,
                  const std::array<glm::vec3, 2> terrain_bbox,
                  const Terrain_map& map, const bool keep_static_lighting)
   -> std::vector<std::array<Terrain_process_vertex, 3>>
{
   const std::size_t triangle_count = triangles.size();

   std::vector<std::array<Terrain_process_vertex, 3>> out_triangles;
   out_triangles.resize(triangle_count);

   Vertex_position_compress compress{terrain_bbox};

   for (std::size_t triangle_index = 0; triangle_index < triangle_count;
        ++triangle_index) {
      const std::array<std::uint16_t, 3> indices = triangles[triangle_index];
      const std::array<glm::vec3, 3> positions = {
         stock_vertices[indices[0]].position,
         stock_vertices[indices[1]].position,
         stock_vertices[indices[2]].position,
      };

      const std::array<std::uint8_t, 3> texture_indices =
         pick_textures(positions, map);

      std::array<Terrain_process_vertex, 3>& out_tri = out_triangles[triangle_index];

      std::uint16_t packed_texture_indices = 0;

      packed_texture_indices |= (texture_indices[0] & 0xf);
      packed_texture_indices |= (texture_indices[1] & 0xf) << 4;
      packed_texture_indices |= (texture_indices[2] & 0xf) << 8;

      for (std::size_t i = 0; i < out_tri.size(); ++i) {
         const Stock_terrain_uncompressed_vertex& input = stock_vertices[indices[i]];

         const auto pack_unorm = [](const float v) -> std::uint32_t {
            return static_cast<std::uint8_t>(glm::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
         };

         const glm::vec3 normal = sample_normal(input.position, map);

         std::uint32_t normal_x = pack_unorm(normal.x * 0.5f + 0.5f);
         std::uint32_t normal_z = pack_unorm(normal.z * 0.5f + 0.5f);

         std::array<float, 16> texture_weights = sample_weights(input.position, map);

         for (std::ptrdiff_t weight_index = std::ssize(texture_weights) - 2;
              weight_index >= 0; --weight_index) {
            texture_weights[weight_index] *=
               (1.0f - texture_weights[weight_index + 1]);
         }

         glm::vec3 used_texture_weights = {
            texture_weights[texture_indices[0]],
            texture_weights[texture_indices[1]],
            texture_weights[texture_indices[2]],
         };

         const float total_texture_weight = used_texture_weights[0] +
                                            used_texture_weights[1] +
                                            used_texture_weights[2];

         if (total_texture_weight != 0.0f) {
            used_texture_weights /= total_texture_weight;
         }

         std::uint32_t texture_weight_0 = pack_unorm(used_texture_weights[0]);
         std::uint32_t texture_weight_1 = pack_unorm(used_texture_weights[1]);

         std::uint32_t packed_normal = 0;

         packed_normal |= (normal_x << 0u);
         packed_normal |= (normal_z << 24u);
         packed_normal |= (texture_weight_0 << 16u);
         packed_normal |= (texture_weight_1 << 8u);

         out_tri[i].position = input.position;
         out_tri[i].vertex = {
            .position = compress(input.position),
            .texture_indices = packed_texture_indices,
            .normal = packed_normal,
            .tangent = keep_static_lighting ? input.color | 0xff'00'00'00u : 0,
         };
      }
   }

   return out_triangles;
}

auto indexify_mesh(std::span<const std::array<Terrain_process_vertex, 3>> triangles)
   -> std::vector<Terrain_model_segment>
{
   absl::flat_hash_map<Terrain_vertex, std::uint16_t> vertex_cache;
   vertex_cache.reserve(triangles.size());

   std::vector<Terrain_model_segment> segments;

   for (const std::array<Terrain_process_vertex, 3>& input_triangle : triangles) {
      if (segments.empty() or segments.back().vertices.size() >= segment_split_size) {
         segments.push_back({.bbox = {glm::vec3{std::numeric_limits<float>::max()},
                                      glm::vec3{std::numeric_limits<float>::min()}}});

         vertex_cache.clear();
      }

      Terrain_model_segment& segment = segments.back();

      std::array<std::uint16_t, 3> out_triangle = {};

      for (std::size_t i = 0; i < out_triangle.size(); ++i) {
         if (auto it = vertex_cache.find(input_triangle[i].vertex);
             it != vertex_cache.end()) {
            out_triangle[i] = it->second;
         }
         else {
            out_triangle[i] = static_cast<std::uint16_t>(segment.vertices.size());

            segment.vertices.push_back(input_triangle[i].vertex);
            segment.bbox[0] = glm::min(segment.bbox[0], input_triangle[i].position);
            segment.bbox[1] = glm::max(segment.bbox[1], input_triangle[i].position);
         }
      }

      segment.indices.push_back(out_triangle);
   }

   return segments;
}

auto combine_segments(std::vector<Terrain_model_segment>& segments,
                      const Terrain_info info) -> std::vector<Terrain_model_segment>
{
   const float length_half = (info.terrain_length / 2.0f);

   const std::size_t bin_dimension_count = 8;
   const std::size_t bin_length = info.terrain_length / bin_dimension_count;

   if (bin_length == 0) return segments;

   std::vector<Terrain_model_segment> combined_segments;
   combined_segments.reserve(bin_dimension_count * bin_dimension_count);

   std::array<std::array<std::vector<std::size_t>, bin_dimension_count>, bin_dimension_count> bins;

   for (Terrain_model_segment& segment : segments) {
      const std::size_t x =
         std::clamp(static_cast<std::uint32_t>(
                       (segment.bbox[0].x / info.grid_size) + length_half) /
                       bin_length,
                    std::size_t{0}, bin_dimension_count - 1);
      const std::size_t z =
         std::clamp(static_cast<std::uint32_t>(
                       (-segment.bbox[0].z / info.grid_size) + length_half - 1.0f) /
                       bin_length,
                    std::size_t{0}, bin_dimension_count - 1);

      std::vector<std::size_t>& bin = bins[z][x];

      if (bin.empty() or
          combined_segments[bin.back()].vertices.size() + segment.vertices.size() >
             std::numeric_limits<std::uint16_t>::max()) {
         bin.emplace_back(combined_segments.size());

         combined_segments.push_back(std::move(segment));

         continue;
      }

      Terrain_model_segment& combined = combined_segments[bin.back()];

      combined.bbox[0] = glm::min(combined.bbox[0], segment.bbox[0]);
      combined.bbox[1] = glm::max(combined.bbox[1], segment.bbox[1]);

      const std::size_t index_offset = combined.vertices.size();

      combined.vertices.insert(combined.vertices.end(),
                               segment.vertices.begin(), segment.vertices.end());

      combined.indices.reserve(combined.indices.size() + segment.indices.size());

      for (const auto& [i0, i1, i2] : segment.indices) {
         combined.indices.push_back({static_cast<std::uint16_t>(i0 + index_offset),
                                     static_cast<std::uint16_t>(i1 + index_offset),
                                     static_cast<std::uint16_t>(i2 + index_offset)});
      }
   }

   return combined_segments;
}
}

auto create_terrain_model_segments(ucfb::Reader_strict<"PCHS"_mn> pchs,
                                   const Terrain_info info,
                                   const Terrain_map& terrain_map,
                                   const bool keep_static_lighting,
                                   const std::array<glm::vec3, 2> world_bbox)
   -> std::vector<Terrain_model_segment>
{
   const Index_buffer_16 default_index_buffer = create_index_buffer(
      pchs.read_child_strict<"COMN"_mn>().read_child_strict<"IBUF"_mn>());

   const std::size_t patches_length = info.terrain_length / info.patch_length;

   std::vector<Terrain_model_segment> segments;

   segments.reserve(patches_length * patches_length);

   const float world_length = info.terrain_length * info.grid_size;
   const float half_world_length = world_length / 2.0f;
   const float patch_length = info.patch_length * info.grid_size;

   for (std::size_t z = 0; z < patches_length; ++z) {
      for (std::size_t x = 0; x < patches_length; ++x) {
         auto ptch = pchs.read_child_strict<"PTCH"_mn>();

         const Patch_info patch_info =
            read_info(ptch.read_child_strict<"INFO"_mn>());

         const float patch_min_x = (x * patch_length) - half_world_length;
         const float patch_min_z =
            (z * patch_length) - half_world_length + info.grid_size;

         [[maybe_unused]] auto vbuf_compressed = ptch.read_child_strict<"VBUF"_mn>();
         auto vbuf = ptch.read_child_strict<"VBUF"_mn>();

         const std::vector<Stock_terrain_uncompressed_vertex> vertices =
            read_vbuf(vbuf, {patch_min_x, 0.0f, patch_min_z});

         sp::Index_buffer_16 index_buffer;

         if (auto ibuf = ptch.read_child(std::nothrow);
             ibuf and ibuf->magic_number() == "IBUF"_mn) {
            index_buffer = create_index_buffer(ucfb::Reader_strict<"IBUF"_mn>{*ibuf});
         }
         else {
            index_buffer = default_index_buffer;
         }

         const std::vector<std::array<Terrain_process_vertex, 3>> segment_triangle_list =
            convert_mesh(vertices, index_buffer, world_bbox, terrain_map,
                         keep_static_lighting);

         for (Terrain_model_segment& segment : indexify_mesh(segment_triangle_list)) {
            segments.push_back(std::move(segment));
         }
      }
   }

   return combine_segments(segments, info);
}

}
