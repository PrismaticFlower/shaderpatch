#pragma once

#include <d3d11_1.h>

namespace sp::core {

struct Game_input_layout {
   std::uint16_t layout_index;
   bool compressed;
   bool particle_texture_scale;
   bool has_vertex_weights;
};

}
