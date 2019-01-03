#pragma once

#include "com_ptr.hpp"

#include <d3d11_1.h>

namespace sp::core {

struct Patch_texture {
   std::shared_ptr<ID3D11ShaderResourceView> srv;
};

}
