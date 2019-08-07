#pragma once

#include "index_buffer.hpp"
#include "terrain_map.hpp"
#include "ucfb_editor.hpp"

#include <array>
#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

namespace sp {

struct Terrain_vertex {
   glm::vec3 position;
   glm::vec3 diffuse_lighting;
   glm::vec3 base_color;
   std::array<float, 2> texture_blend;
   std::array<std::uint8_t, 3> texture_indices;
};

using Terrain_triangle = std::array<Terrain_vertex, 3>;
using Terrain_vertex_buffer = std::vector<Terrain_vertex>;
using Terrain_triangle_list = std::vector<Terrain_triangle>;

auto create_terrain_triangle_list(const Terrain_map& terrain) -> Terrain_triangle_list;

void output_vertex_buffer(const Terrain_vertex_buffer& vertex_buffer,
                          ucfb::Editor_data_writer& writer,
                          const std::array<glm::vec3, 2> vert_box,
                          const bool pack_lighting);
}
