#pragma once

#include "com_ptr.hpp"

#include <d3d11_1.h>

namespace sp::core {

struct Game_texture {
   Com_ptr<ID3D11ShaderResourceView> srv;
   Com_ptr<ID3D11ShaderResourceView> srgb_srv;
};

inline const Game_texture nullgametex = {nullptr, nullptr};

}
