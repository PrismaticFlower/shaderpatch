
#include "d3d11_helpers.hpp"
#include "utility.hpp"

namespace sp::core {

auto create_immutable_constant_buffer(ID3D11Device1& device,
                                      const std::span<const std::byte> data) noexcept
   -> Com_ptr<ID3D11Buffer>
{
   Expects(is_multiple_of<16u>(data.size()));

   Com_ptr<ID3D11Buffer> buffer;

   const auto size = static_cast<UINT>(data.size());
   const auto desc = CD3D11_BUFFER_DESC{size, D3D11_BIND_CONSTANT_BUFFER,
                                        D3D11_USAGE_IMMUTABLE, 0};
   const auto init_data = D3D11_SUBRESOURCE_DATA{data.data(), static_cast<UINT>(size),
                                                 static_cast<UINT>(size)};

   device.CreateBuffer(&desc, &init_data, buffer.clear_and_assign());

   return buffer;
}

auto create_dynamic_constant_buffer(ID3D11Device1& device, const UINT size) noexcept
   -> Com_ptr<ID3D11Buffer>
{
   Expects(is_multiple_of<16u>(size));

   Com_ptr<ID3D11Buffer> buffer;

   const auto desc = CD3D11_BUFFER_DESC{size, D3D11_BIND_CONSTANT_BUFFER,
                                        D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE};

   device.CreateBuffer(&desc, nullptr, buffer.clear_and_assign());

   return buffer;
}

auto create_dynamic_texture_buffer(ID3D11Device1& device, const UINT size) noexcept
   -> Com_ptr<ID3D11Buffer>
{
   Expects(is_multiple_of<16u>(size));

   Com_ptr<ID3D11Buffer> buffer;

   const auto desc = CD3D11_BUFFER_DESC{size, D3D11_BIND_SHADER_RESOURCE,
                                        D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE};

   device.CreateBuffer(&desc, nullptr, buffer.clear_and_assign());

   return buffer;
}

}
