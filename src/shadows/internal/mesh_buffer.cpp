#include "mesh_buffer.hpp"

#include "../../logger.hpp"

namespace sp::shadows {

Mesh_buffer::Mesh_buffer(ID3D11Device& device, D3D11_BIND_FLAG bind_flag, UINT byte_size)
   : _size{byte_size}
{
   D3D11_BUFFER_DESC desc{.ByteWidth = _size,
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

auto Mesh_buffer::allocate(UINT size, UINT alignment, UINT& out_offset) noexcept -> HRESULT
{
   if (const UINT remainder = _allocated % alignment; remainder != 0) {
      const UINT padding = alignment - remainder;

      if (_size - _allocated < padding) return E_OUTOFMEMORY;

      _allocated += padding;
   }

   if (_size - _allocated < size) return E_OUTOFMEMORY;

   out_offset = _allocated;

   _allocated += size;

   return S_OK;
}

auto Mesh_buffer::allocated_bytes() const noexcept -> UINT
{
   return _allocated;
}

}