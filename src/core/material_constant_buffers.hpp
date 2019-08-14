#pragma once

#include "com_ptr.hpp"
#include "patch_material_io.hpp"

#include <d3d11_4.h>

namespace sp::core {

auto create_material_constant_buffer(ID3D11Device5& device,
                                     const std::string_view cb_name,
                                     const std::vector<Material_property>& properties)
   -> Com_ptr<ID3D11Buffer>;
}
