#pragma once

#include "com_ptr.hpp"
#include "filmic_tonemapper.hpp"
#include "postprocess_params.hpp"

#include <array>
#include <optional>
#include <typeinfo>

#include <d3d11_1.h>

namespace sp::effects {

class Color_grading_lut_baker {
public:
   constexpr static auto lut_dimension = 32u;
   constexpr static auto lut_format = DXGI_FORMAT_R8G8B8A8_UNORM;

   Color_grading_lut_baker(Com_ptr<ID3D11Device1> device) noexcept
      : _device{std::move(device)}
   {
   }

   void update_params(const Color_grading_params& params) noexcept;

   void hdr_state(const Hdr_state state) noexcept;

   auto srv(ID3D11DeviceContext1& dc) noexcept -> ID3D11ShaderResourceView*;

private:
   using Lut_data =
      std::array<std::array<std::array<glm::uint32, Color_grading_lut_baker::lut_dimension>, Color_grading_lut_baker::lut_dimension>,
                 Color_grading_lut_baker::lut_dimension>;

   void bake_color_grading_lut(ID3D11DeviceContext1& dc) noexcept;

   glm::vec3 apply_color_grading(glm::vec3 color) noexcept;

   void uploade_lut(ID3D11DeviceContext1& dc, const Lut_data& lut_data) noexcept;

   struct Eval_params {
      float contrast;
      float saturation;

      glm::vec3 color_filter;
      glm::vec3 hsv_adjust;

      glm::vec3 channel_mix_red;
      glm::vec3 channel_mix_green;
      glm::vec3 channel_mix_blue;

      glm::vec3 lift_adjust;
      glm::vec3 inv_gamma_adjust;
      glm::vec3 gain_adjust;

      filmic::Curve filmic_curve;
   };

   static_assert(std::is_trivially_destructible_v<Eval_params>);

   const Com_ptr<ID3D11Device1> _device;
   Eval_params _eval_params;
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

   bool _rebake = true;
};

}
