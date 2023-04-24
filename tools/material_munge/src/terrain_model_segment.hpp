#pragma once

#include "index_buffer.hpp"
#include "terrain_map.hpp"
#include "ucfb_reader.hpp"
#include "vertex_buffer.hpp"

#include <vector>

#include <glm/glm.hpp>

namespace sp {

struct Terrain_info {
   float grid_size = 0.0f;
   float height_scale = 0.0f;
   float height_min = 0.0f;
   float height_max = 0.0f;
   std::uint16_t terrain_length = 0;
   std::uint16_t patch_length = 0;
};

struct Terrain_vertex {
   glm::i16vec3 position;
   glm::uint16 texture_indices;
   glm::uint32 normal;
   glm::uint32 tangent;
};

static_assert(sizeof(Terrain_vertex) == 16);

using Terrain_vertex_buffer = std::vector<Terrain_vertex>;

constexpr auto terrain_vbuf_flags =
   Vbuf_flags::position | Vbuf_flags::normal | Vbuf_flags::tangents |
   Vbuf_flags::texcoords | Vbuf_flags::position_compressed |
   Vbuf_flags::normal_compressed | Vbuf_flags::texcoord_compressed;

constexpr auto terrain_vbuf_static_lighting_flags =
   Vbuf_flags::position | Vbuf_flags::normal | Vbuf_flags::tangents |
   Vbuf_flags::static_lighting | Vbuf_flags::texcoords |
   Vbuf_flags::position_compressed | Vbuf_flags::normal_compressed |
   Vbuf_flags::texcoord_compressed;

struct Terrain_model_segment {
   Index_buffer_16 indices;
   Terrain_vertex_buffer vertices;
   std::array<glm::vec3, 2> bbox;
};

auto create_terrain_model_segments(ucfb::Reader_strict<"PCHS"_mn> pchs,
                                   const Terrain_info info,
                                   const std::array<glm::vec3, 2> world_bbox)
   -> std::vector<Terrain_model_segment>;

}
