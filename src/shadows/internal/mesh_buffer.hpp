#pragma once

#include "com_ptr.hpp"

#include <d3d11.h>

namespace sp::shadows {

struct Mesh_buffer {
   Mesh_buffer(ID3D11Device& device, D3D11_BIND_FLAG bind_flag,
               UINT element_count, UINT element_size);

   auto get() const noexcept -> ID3D11Buffer*;

   void clear() noexcept;

   auto allocate(UINT count, UINT& out_element_offset) noexcept -> HRESULT;

private:
   Com_ptr<ID3D11Buffer> _buffer;

   UINT _element_count = 0;
   UINT _allocated = 0;
};

}