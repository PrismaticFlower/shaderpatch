#pragma once

#include "vertex_buffer.hpp"

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace sp {

auto weld_vertex_list(const Vertex_buffer& vertex_buffer) noexcept
   -> std::pair<std::vector<std::array<std::uint16_t, 3>>, Vertex_buffer>;

}
