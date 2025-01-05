#pragma once

#include "com_ptr.hpp"

#include <d3d11.h>

namespace sp::shadows {

struct Mesh_buffer {
   Mesh_buffer(Com_ptr<ID3D11Device> device, UINT element_count, UINT32 element_size);

   auto get() noexcept -> ID3D11Buffer*;

private:
   Com_ptr<ID3D11Device> _device;
};

}