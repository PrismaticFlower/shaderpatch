#pragma once

#include "com_ptr.hpp"

#include <cstddef>

#include <d3d11_1.h>

namespace sp::core {

struct Backbuffer_cmaa2_views {
   Backbuffer_cmaa2_views() = default;

   Backbuffer_cmaa2_views(ID3D11Device1& device, ID3D11Texture2D& texture,
                          const DXGI_FORMAT srv_format,
                          const DXGI_FORMAT uav_format) noexcept;

   Com_ptr<ID3D11ShaderResourceView> srv;
   Com_ptr<ID3D11UnorderedAccessView> uav;
};

}
