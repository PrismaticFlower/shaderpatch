
#include "d3d11_helpers.hpp"

namespace sp::core {

auto create_dynamic_constant_buffer(ID3D11Device1& device, const UINT size) noexcept
   -> Com_ptr<ID3D11Buffer>
{
   Com_ptr<ID3D11Buffer> buffer;

   const auto desc = CD3D11_BUFFER_DESC{size, D3D11_BIND_CONSTANT_BUFFER,
                                        D3D11_USAGE_DYNAMIC, D3D10_CPU_ACCESS_WRITE};

   device.CreateBuffer(&desc, nullptr, buffer.clear_and_assign());

   return buffer;
}

auto create_dynamic_texture_buffer(ID3D11Device1& device, const UINT size) noexcept
   -> Com_ptr<ID3D11Buffer>
{
   Com_ptr<ID3D11Buffer> buffer;

   const auto desc = CD3D11_BUFFER_DESC{size, D3D11_BIND_SHADER_RESOURCE,
                                        D3D11_USAGE_DYNAMIC, D3D10_CPU_ACCESS_WRITE};

   device.CreateBuffer(&desc, nullptr, buffer.clear_and_assign());

   return buffer;
}

}
