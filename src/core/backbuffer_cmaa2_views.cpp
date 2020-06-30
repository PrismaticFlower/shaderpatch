#include "backbuffer_cmaa2_views.hpp"

namespace sp::core {

Backbuffer_cmaa2_views::Backbuffer_cmaa2_views(ID3D11Device1& device,
                                               ID3D11Texture2D& texture,
                                               const DXGI_FORMAT srv_format,
                                               const DXGI_FORMAT uav_format) noexcept
{
   const CD3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{D3D11_SRV_DIMENSION_TEXTURE2D,
                                                   srv_format};
   device.CreateShaderResourceView(&texture, &srv_desc, srv.clear_and_assign());

   const CD3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc{D3D11_UAV_DIMENSION_TEXTURE2D,
                                                    uav_format};
   device.CreateUnorderedAccessView(&texture, &uav_desc, uav.clear_and_assign());
}
}
