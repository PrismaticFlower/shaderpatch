
#include "terrain_model_segment.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <absl/container/flat_hash_set.h>
#include <absl/container/inlined_vector.h>

namespace sp {

namespace {

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

struct Stock_terrain_compressed_vertex {
   glm::vec3 position;
   glm::vec3 normal;
   glm::uint32 color;
};

static_assert(sizeof(Stock_terrain_compressed_vertex) == 28);

struct Texture_info {
   std::array<std::uint8_t, 3> indices;
   std::array<std::uint8_t, 3> weights;
};

struct Vbuf_result {
   Terrain_vertex_buffer vertices;
   std::vector<glm::vec3> uncompressed_positions;
};

auto clamp(const int v, const Terrain_map& map) -> int
{
   return std::clamp(v, 0, map.length - 1);
}

#if 0

// This turned out to be unneeded but is kept here for reference just in case.

auto sample_weights(const glm::vec3 position, const Terrain_map& map)
   -> std::array<float, 16>
{
   const float length_half = (map.length / 2.0f);
   const float x = ((position.x / map.grid_scale) + length_half);
   const float z = ((-position.z / map.grid_scale) + length_half - 1.0f);

   const int x_int = static_cast<int>(x);
   const int z_int = static_cast<int>(z);

   std::array<std::uint8_t, 16> uint_weights_x0_z0 =
      map.texture_weights[clamp(z_int + 0, map) * map.length + clamp(x_int + 0, map)];
   std::array<std::uint8_t, 16> uint_weights_x1_z0 =
      map.texture_weights[clamp(z_int + 0, map) * map.length + clamp(x_int + 1, map)];
   std::array<std::uint8_t, 16> uint_weights_x0_z1 =
      map.texture_weights[clamp(z_int + 1, map) * map.length + clamp(x_int + 0, map)];
   std::array<std::uint8_t, 16> uint_weights_x1_z1 =
      map.texture_weights[clamp(z_int + 1, map) * map.length + clamp(x_int + 1, map)];

   const auto unorm_weights = [](std::array<std::uint8_t, 16> uint_weights) {
      std::array<float, 16> weights{};

      for (std::ptrdiff_t i = 0; i < uint_weights.size(); ++i) {
         weights[i] = uint_weights[i] / 255.0f;
      }

      return weights;
   };

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

#endif

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

auto read_vbuf(ucfb::Reader_strict<"VBUF"_mn> vbuf,
               ucfb::Reader_strict<"VBUF"_mn> vbuf_uncomressed,
               const glm::vec3 patch_offset, const std::array<glm::vec3, 2> terrain_bbox,
               const std::array<std::uint8_t, 3> texture_indices) -> Vbuf_result
{
   const auto [count, stride, flags] =
      vbuf.read_multi<std::uint32_t, std::uint32_t, Vbuf_flags>();
   const auto [count_uncompressed, stride_uncompressed, flags_uncompressed] =
      vbuf_uncomressed.read_multi<std::uint32_t, std::uint32_t, Vbuf_flags>();

   if (stride != 16 ||
       flags != (Vbuf_flags::position | Vbuf_flags::normal | Vbuf_flags::static_lighting |
                 Vbuf_flags::position_compressed | Vbuf_flags::normal_compressed)) {
      throw std::runtime_error{"Terrain VBUF has unexpected stride or flags! "
                               "It is quite amazing you managed to hit this."};
   }

   if (count != count_uncompressed) {
      throw std::runtime_error{"Terrain VBUFs have mismatched counts. If "
                               "you're seeing this it is quite amazing."};
   }

   Vbuf_result result;

   result.vertices.reserve(count);
   result.uncompressed_positions.reserve(count);

   Vertex_position_compress compress{terrain_bbox};

   for (std::size_t i = 0; i < count; ++i) {
      const Stock_terrain_vertex input = vbuf.read<Stock_terrain_vertex>();
      const Stock_terrain_compressed_vertex input_uncompressed =
         vbuf_uncomressed.read<Stock_terrain_compressed_vertex>();

      const glm::vec3 position = input_uncompressed.position + patch_offset;

      const auto pack_unorm = [](const float v) -> std::uint32_t {
         return static_cast<std::uint8_t>(glm::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
      };

      std::uint32_t normal_x = pack_unorm(input_uncompressed.normal.x * 0.5f + 0.5f);
      std::uint32_t normal_z = pack_unorm(input_uncompressed.normal.z * 0.5f + 0.5f);

      std::uint32_t texture_weight_0 = (input.color >> 24u) & 0xffu;
      std::uint32_t texture_weight_1 = (input.normal >> 24u) & 0xffu;

      std::uint32_t packed_normal = 0;

      packed_normal |= (normal_x << 0u);
      packed_normal |= (normal_z << 24u);
      packed_normal |= (texture_weight_0 << 16u);
      packed_normal |= (texture_weight_1 << 8u);

      std::uint16_t packed_texture_indices = 0;

      packed_texture_indices |= (texture_indices[0] & 0xf);
      packed_texture_indices |= (texture_indices[1] & 0xf) << 4;
      packed_texture_indices |= (texture_indices[2] & 0xf) << 8;

      result.vertices.push_back(Terrain_vertex{
         .position = compress(position),
         .texture_indices = packed_texture_indices,
         .normal = packed_normal,
         .tangent = input.color | 0xff'00'00'00u,
      });

      result.uncompressed_positions.push_back(position);
   }

   return result;
}

auto make_bbox(std::span<const glm::vec3> uncompressed_positions)
   -> std::array<glm::vec3, 2>
{
   std::array<glm::vec3, 2> bbox = {glm::vec3{std::numeric_limits<float>::max()},
                                    glm::vec3{std::numeric_limits<float>::lowest()}};

   for (auto& v : uncompressed_positions) {
      bbox[0] = glm::min(bbox[0], v);
      bbox[1] = glm::max(bbox[1], v);
   }

   return bbox;
}

auto combine_segments(const std::vector<Terrain_model_segment>& segments,
                      const std::size_t patches_length)
   -> std::vector<Terrain_model_segment>
{
   if (patches_length % 8 != 0) {
      throw std::runtime_error{"Can not convert terrain to use custom "
                               "materials. Has unusual dimensions."};
   }

   std::vector<Terrain_model_segment> combined_segments;

   for (std::size_t start_z = 0; start_z < patches_length; start_z += 8) {
      for (std::size_t start_x = 0; start_x < patches_length; start_x += 8) {
         auto& combined = combined_segments.emplace_back();

         combined.bbox = {glm::vec3{std::numeric_limits<float>::max()},
                          glm::vec3{std::numeric_limits<float>::lowest()}};

         for (std::size_t z_local = 0; z_local < 8; ++z_local) {
            for (std::size_t x_local = 0; x_local < 8; ++x_local) {
               const std::size_t z = start_z + z_local;
               const std::size_t x = start_x + x_local;

               const auto& separate = segments[z * patches_length + x];

               const std::size_t index_offset = combined.vertices.size();

               combined.bbox = {glm::min(separate.bbox[0], combined.bbox[0]),
                                glm::max(separate.bbox[1], combined.bbox[1])};
               combined.vertices.insert(combined.vertices.end(),
                                        separate.vertices.begin(),
                                        separate.vertices.end());

               for (std::size_t i = 0; i < separate.indices.size(); ++i) {
                  combined.indices.push_back(
                     {static_cast<std::uint16_t>(separate.indices[i][0] + index_offset),
                      static_cast<std::uint16_t>(separate.indices[i][1] + index_offset),
                      static_cast<std::uint16_t>(separate.indices[i][2] + index_offset)});
               }
            }
         }

         if (combined.vertices.size() > std::numeric_limits<std::uint16_t>::max()) {
            throw std::runtime_error{
               "Segment has too many vertices. Isn't that nice and vague?"};
         }
      }
   }

   return combined_segments;
}

}

auto create_terrain_model_segments(ucfb::Reader_strict<"PCHS"_mn> pchs,
                                   const Terrain_info info)
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

   const float world_min_x = -half_world_length;
   const float world_max_x = half_world_length;

   const float world_min_y = info.height_min;
   const float world_max_y = info.height_max;

   const float world_min_z = -half_world_length + info.grid_size;
   const float world_max_z = half_world_length + info.grid_size;

   for (std::size_t z = 0; z < patches_length; ++z) {
      for (std::size_t x = 0; x < patches_length; ++x) {
         auto& segment = segments.emplace_back();

         auto ptch = pchs.read_child_strict<"PTCH"_mn>();

         const Patch_info patch_info =
            read_info(ptch.read_child_strict<"INFO"_mn>());

         const float patch_min_x = (x * patch_length) - half_world_length;
         const float patch_min_z =
            (z * patch_length) - half_world_length + info.grid_size;

         auto vbuf = ptch.read_child_strict<"VBUF"_mn>();
         auto vbuf_compressed = ptch.read_child_strict<"VBUF"_mn>();

         Vbuf_result vbuf_result =
            read_vbuf(vbuf, vbuf_compressed, {patch_min_x, 0.0f, patch_min_z},
                      {glm::vec3{world_min_x, world_min_y, world_min_z},
                       glm::vec3{world_max_x, world_max_y, world_max_z}},
                      patch_info.textures);

         if (auto ibuf = ptch.read_child(std::nothrow);
             ibuf and ibuf->magic_number() == "IBUF"_mn) {
            segment.indices =
               create_index_buffer(ucfb::Reader_strict<"IBUF"_mn>{*ibuf});
         }
         else {
            segment.indices = default_index_buffer;
         }

         segment.vertices = std::move(vbuf_result.vertices);
         segment.bbox = make_bbox(vbuf_result.uncompressed_positions);
      }
   }

   return combine_segments(segments, patches_length);
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
