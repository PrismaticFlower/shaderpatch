#pragma once

#include "index_buffer.hpp"
#include "terrain_vertex_buffer.hpp"

namespace sp {

struct Terrain_model_shadow {
   Index_buffer_16 indices;
   Terrain_vertex_buffer vertices;
};

auto create_terrain_model_shadows(const Terrain_triangle_list& triangles)
   -> std::vector<Terrain_model_shadow>;

}
