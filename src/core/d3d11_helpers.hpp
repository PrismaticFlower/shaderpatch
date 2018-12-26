#pragma once

#include "com_ptr.hpp"

#include <utility>

#include <d3d11_1.h>

#include <gsl/gsl>

namespace sp::core {

inline auto rect_to_box(const RECT& rect) noexcept -> D3D11_BOX
{
   return {static_cast<UINT>(rect.left),  static_cast<UINT>(rect.top),    0,
           static_cast<UINT>(rect.right), static_cast<UINT>(rect.bottom), 1};
}

constexpr bool boxes_same_size(const D3D11_BOX& test_box,
                               const D3D11_BOX& containing_box) noexcept
{
   if ((containing_box.right - containing_box.left) !=
       (test_box.right - test_box.left))
      return false;

   if ((containing_box.bottom - containing_box.top) !=
       (test_box.bottom - test_box.top))
      return false;

   if ((containing_box.back - containing_box.front) != (test_box.back - test_box.front))
      return false;

   return true;
}

auto create_dynamic_constant_buffer(ID3D11Device1& device, const UINT size) noexcept
   -> Com_ptr<ID3D11Buffer>;

auto create_dynamic_texture_buffer(ID3D11Device1& device, const UINT size) noexcept
   -> Com_ptr<ID3D11Buffer>;

template<typename Buffer_struct>
void update_dynamic_buffer(ID3D11DeviceContext1& dc, ID3D11Buffer& buffer,
                           const Buffer_struct& buffer_struct) noexcept
{
#ifndef NDEBUG
   D3D11_BUFFER_DESC desc;

   buffer.GetDesc(&desc);

   assert(desc.Usage & D3D11_USAGE_DYNAMIC);
   assert(desc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE);
   assert(desc.ByteWidth >= sizeof(Buffer_struct));
#endif

   D3D11_MAPPED_SUBRESOURCE map;

   dc.Map(&buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);

   std::memcpy(map.pData, &buffer_struct, sizeof(Buffer_struct));

   dc.Unmap(&buffer, 0);
}
}
