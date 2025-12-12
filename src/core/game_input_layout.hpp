#pragma once

#include <d3d11_1.h>

namespace sp::core {

struct Game_input_layout {
   UINT16 layout_index;
   bool compressed_position;
   bool compressed_texcoords;
   bool has_vertex_weights;
};

}
