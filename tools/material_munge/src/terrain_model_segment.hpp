#pragma once

#include "index_buffer.hpp"
#include "terrain_vertex_buffer.hpp"

#include <vector>

#include <glm/glm.hpp>

namespace sp {

struct Terrain_model_segment {
   Index_buffer_16 indices;
   Terrain_vertex_buffer vertices;
   std::array<glm::vec3, 2> bbox;
};

auto create_terrain_model_segments(const Terrain_triangle_list& triangles,
                                   const float terrain_length)
   -> std::vector<Terrain_model_segment>;

auto optimize_terrain_model_segments(std::vector<Terrain_model_segment> segments) noexcept
   -> std::vector<Terrain_model_segment>;

auto calculate_terrain_model_segments_aabb(const std::vector<Terrain_model_segment>& segments) noexcept
   -> std::array<glm::vec3, 2>;

}
