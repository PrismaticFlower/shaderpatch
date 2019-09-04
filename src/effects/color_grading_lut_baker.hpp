#pragma once

#include "../core/d3d11_helpers.hpp"
#include "../core/shader_database.hpp"
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

   Color_grading_lut_baker(Com_ptr<ID3D11Device1> device,
                           const core::Shader_group_collection& shader_groups) noexcept;

   void bake_color_grading_lut(ID3D11DeviceContext1& dc,
                               const Color_grading_params& params) noexcept;

   auto srv() noexcept -> ID3D11ShaderResourceView*;

private:
   constexpr static auto group_size = 8u;

   void update_cb(ID3D11DeviceContext1& dc, const Color_grading_params& params) noexcept;

   auto pick_shader(const Tonemapper tonemapper) noexcept -> ID3D11ComputeShader*;

   const Com_ptr<ID3D11Device1> _device;

   const Com_ptr<ID3D11Texture3D> _texture = [this] {
      Com_ptr<ID3D11Texture3D> texture;

      const auto desc = CD3D11_TEXTURE3D_DESC{DXGI_FORMAT_R8G8B8A8_TYPELESS,
                                              lut_dimension,
                                              lut_dimension,
                                              lut_dimension,
                                              1,
                                              D3D11_BIND_SHADER_RESOURCE |
                                                 D3D11_BIND_UNORDERED_ACCESS,
                                              D3D11_USAGE_DEFAULT};

      _device->CreateTexture3D(&desc, nullptr, texture.clear_and_assign());

      return texture;
   }();

   const Com_ptr<ID3D11UnorderedAccessView> _uav = [this] {
      Com_ptr<ID3D11UnorderedAccessView> uav;

      const auto desc =
         CD3D11_UNORDERED_ACCESS_VIEW_DESC{D3D11_UAV_DIMENSION_TEXTURE3D,
                                           DXGI_FORMAT_R8G8B8A8_UNORM};

      _device->CreateUnorderedAccessView(_texture.get(), &desc,
                                         uav.clear_and_assign());

      return uav;
   }();

   const Com_ptr<ID3D11ShaderResourceView> _srv = [this] {
      Com_ptr<ID3D11ShaderResourceView> srv;

      const auto desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE3D,
                                          DXGI_FORMAT_R8G8B8A8_UNORM_SRGB};

      _device->CreateShaderResourceView(_texture.get(), &desc, srv.clear_and_assign());

      return srv;
   }();

   const Com_ptr<ID3D11ComputeShader> _cs_main_filmic;
   const Com_ptr<ID3D11ComputeShader> _cs_main_aces;
   const Com_ptr<ID3D11ComputeShader> _cs_main_heji2015;
   const Com_ptr<ID3D11ComputeShader> _cs_main_reinhard;
   const Com_ptr<ID3D11ComputeShader> _cs_main;

   struct Color_grading_lut_cb {
      Color_grading_lut_cb(const Color_grading_params& params) noexcept;

      glm::vec3 color_filter;
      float contrast;
      glm::vec3 hsv_adjust;
      float saturation;

      glm::vec3 channel_mix_red;
      std::uint32_t _padding0;
      glm::vec3 channel_mix_green;
      std::uint32_t _padding1{};
      glm::vec3 channel_mix_blue;
      std::uint32_t _padding2;

      glm::vec3 lift_adjust;
      std::uint32_t _padding3;
      glm::vec3 inv_gamma_adjust;
      std::uint32_t _padding4;
      glm::vec3 gain_adjust;
      float heji_whitepoint;

      struct Filmic_curve {
         Filmic_curve(const filmic::Curve& curve) noexcept;

         float w;
         float inv_w;

         float x0;
         float y0;
         float x1;
         float y1;

         std::array<std::uint32_t, 2> _padding{};

         struct Curve_segment {
            float offset_x;
            float offset_y;
            float scale_x;
            float scale_y;
            float ln_a;
            float b;
            std::array<std::uint32_t, 2> _padding{};
         };

         std::array<Curve_segment, 3> segments{};
      };

      Filmic_curve filmic_curve;
   };

   static_assert(sizeof(Color_grading_lut_cb) == 256);
   static_assert(std::is_trivially_destructible_v<Color_grading_lut_cb>);

   const Com_ptr<ID3D11Buffer> _cb =
      core::create_dynamic_constant_buffer(*_device, sizeof(Color_grading_lut_cb));
};

}
