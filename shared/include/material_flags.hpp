#pragma once

#include "enum_flags.hpp"

#include <cstdint>

namespace sp {

enum class Material_flags : std::uint32_t {
   normal = 1,
   hardedged = 2,
   transparent = 4,
   glossmap = 8,
   glow = 16,
   perpixel = 32,
   additive = 64,
   specular = 128,
   env_map = 256,
   vertex_lit = 512,
   tiled_normalmap = 2048,
   doublesided = 65536,

   scrolling = 16777216,
   energy = 33554432,
   animated = 67108864,

   attached_light = 134217728,
};

template<>
struct is_enum_flag<Material_flags> : std::true_type {
};

}
