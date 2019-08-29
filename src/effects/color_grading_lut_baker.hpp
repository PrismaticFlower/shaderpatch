#pragma once

#include "color_grading_eval_params.hpp"
#include "color_grading_regions_io.hpp"
#include "com_ptr.hpp"
#include "filmic_tonemapper.hpp"
#include "postprocess_params.hpp"

#include <array>
#include <compare>
#include <optional>
#include <typeinfo>

#include <d3d11_1.h>

namespace sp::effects {

class Color_grading_lut_baker {
public:
   constexpr static auto lut_dimension = 32u;
   constexpr static auto lut_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

   Color_grading_lut_baker(Com_ptr<ID3D11Device1> device) noexcept
      : _device{std::move(device)}
   {
   }

   void bake_color_grading_lut(ID3D11DeviceContext1& dc,
                               const Color_grading_eval_params params) noexcept;

   auto srv() noexcept -> ID3D11ShaderResourceView*;

private:
   using Lut_data =
      std::array<std::array<std::array<glm::uint32, Color_grading_lut_baker::lut_dimension>, Color_grading_lut_baker::lut_dimension>,
                 Color_grading_lut_baker::lut_dimension>;

   glm::vec3 apply_color_grading(glm::vec3 color,
                                 const Color_grading_eval_params& params) noexcept;

   glm::vec3 apply_tonemapping(glm::vec3 color,
                               const Color_grading_eval_params& params) noexcept;

   void uploade_lut(ID3D11DeviceContext1& dc, const Lut_data& lut_data) noexcept;

   const Com_ptr<ID3D11Device1> _device;
   Hdr_state _hdr_state = Hdr_state::stock;

   const Com_ptr<ID3D11Texture3D> _texture = [this] {
      Com_ptr<ID3D11Texture3D> texture;

      const auto desc = CD3D11_TEXTURE3D_DESC{lut_format,
                                              lut_dimension,
                                              lut_dimension,
                                              lut_dimension,
                                              1,
                                              D3D11_BIND_SHADER_RESOURCE,
                                              D3D11_USAGE_DEFAULT};

      _device->CreateTexture3D(&desc, nullptr, texture.clear_and_assign());

      return texture;
   }();

   const Com_ptr<ID3D11ShaderResourceView> _srv = [this] {
      Com_ptr<ID3D11ShaderResourceView> srv;

      _device->CreateShaderResourceView(_texture.get(), nullptr,
                                        srv.clear_and_assign());

      return srv;
   }();
};

}
