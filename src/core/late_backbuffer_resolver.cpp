
#include "late_backbuffer_resolver.hpp"
#include "d3d11_helpers.hpp"

#include <array>

namespace sp::core {

Late_backbuffer_resolver::Late_backbuffer_resolver(ID3D11Device1& device,
                                                   const Shader_database& shader_database) noexcept
   : _vs{std::get<0>(shader_database.groups.at("late_backbuffer_resolve"s)
                        .vertex.at("main_vs"s)
                        .copy())},
     _ps{shader_database.groups.at("late_backbuffer_resolve"s)
            .pixel.at("main_ps"s)
            .copy()},
     _ps_x2{shader_database.groups.at("late_backbuffer_resolve"s)
               .pixel.at("main_x2_ps"s)
               .copy()},
     _ps_x4{shader_database.groups.at("late_backbuffer_resolve"s)
               .pixel.at("main_x4_ps"s)
               .copy()},
     _ps_x8{shader_database.groups.at("late_backbuffer_resolve"s)
               .pixel.at("main_x8_ps"s)
               .copy()},
     _ps_linear{shader_database.groups.at("late_backbuffer_resolve"s)
                   .pixel.at("main_linear_ps"s)
                   .copy()},
     _ps_linear_x2{shader_database.groups.at("late_backbuffer_resolve"s)
                      .pixel.at("main_linear_x2_ps"s)
                      .copy()},
     _ps_linear_x4{shader_database.groups.at("late_backbuffer_resolve"s)
                      .pixel.at("main_linear_x4_ps"s)
                      .copy()},
     _ps_linear_x8{shader_database.groups.at("late_backbuffer_resolve"s)
                      .pixel.at("main_linear_x8_ps"s)
                      .copy()},
     _cb{create_dynamic_constant_buffer(device, sizeof(std::array<std::uint32_t, 4>))}
{
}

void Late_backbuffer_resolver::resolve(ID3D11DeviceContext1& dc,
                                       const Shader_resource_database& textures,
                                       const bool linear_source,
                                       const Game_rendertarget& source,
                                       ID3D11RenderTargetView& target) noexcept
{
   dc.ClearState();
   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   dc.VSSetShader(_vs.get(), nullptr, 0);

   const CD3D11_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(source.width),
                                  static_cast<float>(source.height)};

   dc.RSSetViewports(1, &viewport);

   auto* const rtv = &target;
   dc.OMSetRenderTargets(1, &rtv, nullptr);

   update_blue_noise_srv(textures);
   const std::array srvs{source.srv.get(), _blue_noise_srv.get()};
   dc.PSSetShaderResources(0, srvs.size(), srvs.data());

   const std::array<std::uint32_t, 4> randomness{_rand_dist(_xorshift),
                                                 _rand_dist(_xorshift)};
   update_dynamic_buffer(dc, *_cb, &randomness);
   auto* const cb = _cb.get();
   dc.PSSetConstantBuffers(0, 1, &cb);

   dc.PSSetShader(get_pixel_shader(linear_source, source), nullptr, 0);

   dc.Draw(3, 0);
}

void Late_backbuffer_resolver::update_blue_noise_srv(const Shader_resource_database& textures)
{
   _blue_noise_srv = textures.at_if("_SP_BUILTIN_blue_noise_rgb_"s +
                                    std::to_string(_rand_dist(_xorshift)));
}

auto Late_backbuffer_resolver::get_pixel_shader(const bool linear_source,
                                                const Game_rendertarget& source) const noexcept
   -> ID3D11PixelShader*
{
   if (linear_source) {
      switch (source.sample_count) {
      case 1:
         return _ps_linear.get();
      case 2:
         return _ps_linear_x2.get();
      case 4:
         return _ps_linear_x4.get();
      case 8:
         return _ps_linear_x8.get();
      }
   }

   switch (source.sample_count) {
   case 1:
      return _ps.get();
   case 2:
      return _ps_x2.get();
   case 4:
      return _ps_x4.get();
   case 8:
      return _ps_x8.get();
   default:
      log_and_terminate("Unsupported sample count!");
   }
}
}
