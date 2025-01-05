#include "mesh_buffer.hpp"

#include "../../logger.hpp"

namespace sp::shadows {

Mesh_buffer::Mesh_buffer(ID3D11Device& device, D3D11_BIND_FLAG bind_flag,
                         UINT element_count, UINT element_size)
   : _element_count{element_count}
{
   D3D11_BUFFER_DESC desc{.ByteWidth = _element_count * element_size,
                          .Usage = D3D11_USAGE_DEFAULT,
                          .BindFlags = static_cast<UINT>(bind_flag)};

   if (FAILED(device.CreateBuffer(&desc, nullptr, _buffer.clear_and_assign()))) {
      log_and_terminate("Failed to create Shader World mesh buffer!");
   }
}

auto Mesh_buffer::get() const noexcept -> ID3D11Buffer*
{
   return _buffer.get();
}

void Mesh_buffer::clear() noexcept
{
   _allocated = 0;
}

auto Mesh_buffer::allocate(UINT count, UINT& out_element_offset) noexcept -> HRESULT
{
   if (_element_count - _allocated < count) return E_OUTOFMEMORY;

   out_element_offset = _allocated;

   _allocated += count;

   return S_OK;
}

}