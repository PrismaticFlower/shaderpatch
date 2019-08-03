#pragma once

#include "terrain_map.hpp"

namespace sp {

auto terrain_downsample(const Terrain_map& input, const std::uint16_t new_length)
   -> Terrain_map;

}
