#pragma once

#include "com_ptr.hpp"

#include <d3d11.h>

namespace sp::shadows {

struct Mesh_buffer {
   Mesh_buffer(ID3D11Device& device, D3D11_BIND_FLAG bind_flag, UINT byte_size);

   auto get() const noexcept -> ID3D11Buffer*;

   void clear() noexcept;

   auto allocate(UINT size, UINT alignment, UINT& out_offset) noexcept -> HRESULT;

   auto allocated_bytes() const noexcept -> UINT;

private:
   Com_ptr<ID3D11Buffer> _buffer;

   UINT _size = 0;
   UINT _allocated = 0;
};

}