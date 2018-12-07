#pragma once

#include <d3d11_1.h>

namespace sp::core {

struct Game_input_layout {
   Com_ptr<ID3D11InputLayout> layout;
   bool compressed;
};

}
