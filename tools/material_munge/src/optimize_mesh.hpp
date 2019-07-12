#pragma once

#include "index_buffer.hpp"
#include "terrain_vertex_buffer.hpp"
#include "vertex_buffer.hpp"

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace sp {

auto optimize_mesh(const Index_buffer_16& index_buffer,
                   const Vertex_buffer& vertex_buffer) noexcept
   -> std::pair<Index_buffer_16, Vertex_buffer>;

auto optimize_mesh(Index_buffer_16 index_buffer, Terrain_vertex_buffer vertex_buffer) noexcept
   -> std::pair<Index_buffer_16, Terrain_vertex_buffer>;
}
