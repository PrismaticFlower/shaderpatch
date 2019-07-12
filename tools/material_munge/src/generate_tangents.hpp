#pragma once

#include "terrain_texture_transform.hpp"
#include "terrain_vertex_buffer.hpp"
#include "vertex_buffer.hpp"

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace sp {

auto generate_tangents(const std::vector<std::array<std::uint16_t, 3>>& index_buffer,
                       const Vertex_buffer& vertex_buffer) noexcept
   -> std::pair<std::vector<std::array<std::uint16_t, 3>>, Vertex_buffer>;

auto generate_tangents(Terrain_triangle_list triangles,
                       const std::array<Terrain_texture_transform, 16>& texture_transforms) noexcept
   -> Terrain_triangle_list;
}
