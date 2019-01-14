
#include "image_stretcher.hpp"
#include "d3d11_helpers.hpp"

#include <DirectXTex.h>
#include <gsl/gsl>

namespace sp::core {

Image_stretcher::Image_stretcher(ID3D11Device1& device,
                                 const Shader_database& shader_database) noexcept
   : _x_8_y_8_shader_float{shader_database.groups.at("stretch_texture"s)
                              .compute.at("main float"s)
                              .copy()},
     _x_8_y_8_shader_unorm{
        shader_database.groups.at("stretch_texture"s).compute.at("main unorm"s).copy()},
     _constant_buffer{create_dynamic_constant_buffer(device, sizeof(Input_vars))}
{
}

void Image_stretcher::stretch(ID3D11DeviceContext1& dc, const D3D11_BOX& source_box,
                              ID3D11ShaderResourceView& source,
                              ID3D11UnorderedAccessView& dest,
                              const D3D11_BOX& dest_box) const noexcept
{
   Expects(source_box.front == 0 && source_box.back == 1);
   Expects(dest_box.front == 0 && dest_box.back == 1);

   // Update constant buffer
   Input_vars vars;

   vars.dest_start = {dest_box.left, dest_box.top};
   vars.dest_end = {dest_box.right, dest_box.bottom};
   vars.dest_length = vars.dest_end - vars.dest_start;

   vars.src_start = {source_box.left, source_box.top};
   vars.src_length = glm::uvec2{source_box.right, source_box.bottom} - vars.src_start;

   update_dynamic_buffer(dc, *_constant_buffer, vars);

   // Setup state
   auto* const cb = _constant_buffer.get();
   auto* const srv = &source;
   auto* const uav = &dest;

   dc.CSSetShaderResources(0, 1, &srv);
   dc.CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
   dc.CSSetConstantBuffers(0, 1, &cb);
   dc.CSSetShader(get_correct_shader(dest), nullptr, 0);

   const glm::uvec2 thread_groups = glm::ceil(glm::vec2{vars.dest_length} / 8.f);

   // Dispatch
   dc.Dispatch(thread_groups.x, thread_groups.y, 1);

   // Cleanup State
   ID3D11Buffer* const buffer_null = nullptr;
   ID3D11ShaderResourceView* const srv_null = nullptr;
   ID3D11UnorderedAccessView* const uav_null = nullptr;

   dc.CSSetShaderResources(0, 1, &srv_null);
   dc.CSSetUnorderedAccessViews(0, 1, &uav_null, nullptr);
   dc.CSSetConstantBuffers(0, 1, &buffer_null);
   dc.CSSetShader(nullptr, nullptr, 0);
}

auto Image_stretcher::get_correct_shader(ID3D11UnorderedAccessView& uav) const
   noexcept -> ID3D11ComputeShader*
{
   D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
   uav.GetDesc(&desc);

   switch (desc.Format) {
   case DXGI_FORMAT_R32G32B32A32_FLOAT:
   case DXGI_FORMAT_R16G16B16A16_FLOAT:
      return _x_8_y_8_shader_float.get();
   case DXGI_FORMAT_R16G16B16A16_UNORM:
   case DXGI_FORMAT_R10G10B10A2_UNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM:
   case DXGI_FORMAT_B5G5R5A1_UNORM:
   case DXGI_FORMAT_B8G8R8A8_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM:
   case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
   case DXGI_FORMAT_B4G4R4A4_UNORM:
      return _x_8_y_8_shader_unorm.get();
   }

   log_and_terminate("Attempt to stetch unsupported image format!");
}

}
