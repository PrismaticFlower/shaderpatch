#pragma once

#include "index_buffer.hpp"
#include "terrain_vertex_buffer.hpp"
#include "vertex_buffer.hpp"

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace sp {

auto weld_vertex_list(const Vertex_buffer& vertex_buffer) noexcept
   -> std::pair<Index_buffer_16, Vertex_buffer>;

auto weld_vertex_list(const Terrain_triangle_list& triangles) noexcept
   -> std::pair<Index_buffer_32, Terrain_vertex_buffer>;

}
