#pragma once

#include "ucfb_editor.hpp"
#include "ucfb_reader.hpp"

#include <array>
#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

namespace sp {

struct Terrain_vertex {
   glm::vec3 position;
   glm::vec3 normal;
   glm::vec3 tangent;
   float bitangent_sign;
   glm::uint32 static_lighting;
   glm::vec2 blend_weights;
   std::array<std::uint8_t, 3> texture_indices;
};

using Terrain_vertex_buffer = std::vector<Terrain_vertex>;
using Terrain_triangle_list = std::vector<std::array<Terrain_vertex, 3>>;

auto create_terrain_vertex_buffer(ucfb::Reader_strict<"VBUF"_mn> vbuf,
                                  const std::array<std::uint8_t, 3> texture_indices,
                                  const std::array<glm::vec3, 2> vert_box)
   -> Terrain_vertex_buffer;

void output_vertex_buffer(const Terrain_vertex_buffer& vertex_buffer,
                          ucfb::Editor_data_writer& writer,
                          const std::array<glm::vec3, 2> vert_box);
}
