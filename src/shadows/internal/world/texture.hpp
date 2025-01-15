#pragma once

#include "com_ptr.hpp"

#include <d3d11.h>

namespace sp::shadows {

struct Texture {
   Com_ptr<ID3D11ShaderResourceView> shader_resource_view;
};

}