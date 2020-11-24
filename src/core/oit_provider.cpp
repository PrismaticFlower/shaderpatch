
#include "oit_provider.hpp"
#include "../logger.hpp"
#include "../user_config.hpp"

#include <string_view>

namespace sp::core {

using namespace std::literals;

OIT_provider::OIT_provider(Com_ptr<ID3D11Device5> device, shader::Database& shaders) noexcept
   : _device{device},
     _vs{std::get<Com_ptr<ID3D11VertexShader>>(
        shaders.vertex("adaptive oit resolve"sv).entrypoint("main_vs"))},
     _ps{shaders.pixel("adaptive oit resolve"sv).entrypoint("main_ps")}
{
   if (_usable) {
      log(Log_level::info,
          "Adaptive Order-Independent Transparency is supported.");
   }
   else {
      D3D11_FEATURE_DATA_D3D11_OPTIONS2 data{};

      _device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &data, sizeof(data));

      std::string_view missing;

      if (!data.ROVsSupported && !data.TypedUAVLoadAdditionalFormats) {
         missing = "Rasterizer Ordered Views and typed Unordered Access View loads."sv;
      }
      else if (!data.ROVsSupported) {
         missing = "Rasterizer Ordered Views."sv;
      }
      else {
         missing = "typed Unordered Access View loads."sv;
      }

      log(Log_level::info, "Adaptive Order-Independent Transparency is unsupported. GPU is missing support for ",
          missing);
   }
}

void OIT_provider::prepare_resources(ID3D11DeviceContext4& dc,
                                     ID3D11Texture2D& opaque_texture,
                                     ID3D11RenderTargetView& opaque_rtv) noexcept
{
   if (&opaque_texture != _opaque_texture) {
      update_resources(opaque_texture, opaque_rtv);
      record_resolve_commandlist();
   }

   dc.ClearUnorderedAccessViewUint(_clear_uav.get(),
                                   std::array<UINT, 4>{0x1, 0x1, 0x1, 0x1}.data());
}

void OIT_provider::clear_resources() noexcept
{
   _opaque_texture = nullptr;
   _clear_uav = nullptr;
   _clear_srv = nullptr;
   _depth_uav = nullptr;
   _depth_srv = nullptr;
   _color_uav = nullptr;
   _color_srv = nullptr;
}

void OIT_provider::resolve(ID3D11DeviceContext4& dc) const noexcept
{
   dc.ExecuteCommandList(_resolve_commandlist.get(), true);
}

auto OIT_provider::uavs() const noexcept -> std::array<ID3D11UnorderedAccessView*, 3>
{
   return {_clear_uav.get(), _depth_uav.get(), _color_uav.get()};
}

bool OIT_provider::enabled() const noexcept
{
   return _usable && user_config.graphics.enable_oit;
}

bool OIT_provider::usable(ID3D11Device5& device) noexcept
{
   D3D11_FEATURE_DATA_D3D11_OPTIONS2 data{};

   device.CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &data, sizeof(data));

   return data.TypedUAVLoadAdditionalFormats && data.ROVsSupported;
}

void OIT_provider::update_resources(ID3D11Texture2D& opaque_texture,
                                    ID3D11RenderTargetView& opaque_rtv) noexcept
{
   _opaque_texture = copy_raw_com_ptr(opaque_texture);
   _opaque_rtv = copy_raw_com_ptr(opaque_rtv);

   const auto [width, height] = [&] {
      D3D11_TEXTURE2D_DESC desc{};

      opaque_texture.GetDesc(&desc);

      return std::make_tuple(desc.Width, desc.Height);
   }();

   // Create clear texture.
   {
      D3D11_TEXTURE2D_DESC desc{};
      desc.Width = width;
      desc.Height = height;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.SampleDesc = {1, 0};
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
      desc.Format = DXGI_FORMAT_R8_UINT;

      Com_ptr<ID3D11Texture2D> texture;

      _device->CreateTexture2D(&desc, nullptr, texture.clear_and_assign());
      _device->CreateUnorderedAccessView(texture.get(), nullptr,
                                         _clear_uav.clear_and_assign());
      _device->CreateShaderResourceView(texture.get(), nullptr,
                                        _clear_srv.clear_and_assign());
   }

   // Create depth + color buffers.
   {
      D3D11_TEXTURE2D_DESC desc{};
      desc.Width = width;
      desc.Height = height;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.SampleDesc = {1, 0};
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

      // Create depth buffer.
      {
         desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;

         Com_ptr<ID3D11Texture2D> texture;

         _device->CreateTexture2D(&desc, nullptr, texture.clear_and_assign());
         _device->CreateUnorderedAccessView(texture.get(), nullptr,
                                            _depth_uav.clear_and_assign());
         _device->CreateShaderResourceView(texture.get(), nullptr,
                                           _depth_srv.clear_and_assign());
      }

      // Create color buffer.
      {
         desc.Format = DXGI_FORMAT_R32G32B32A32_UINT;

         Com_ptr<ID3D11Texture2D> texture;

         _device->CreateTexture2D(&desc, nullptr, texture.clear_and_assign());
         _device->CreateUnorderedAccessView(texture.get(), nullptr,
                                            _color_uav.clear_and_assign());
         _device->CreateShaderResourceView(texture.get(), nullptr,
                                           _color_srv.clear_and_assign());
      }
   }
}

void OIT_provider::record_resolve_commandlist() noexcept
{
   Com_ptr<ID3D11DeviceContext3> dc;

   _device->CreateDeferredContext3(0, dc.clear_and_assign());

   D3D11_TEXTURE2D_DESC texture_desc{};
   _opaque_texture->GetDesc(&texture_desc);

   dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   dc->VSSetShader(_vs.get(), nullptr, 0);
   dc->PSSetShader(_ps.get(), nullptr, 0);

   const std::array srvs{_clear_srv.get(), _depth_srv.get(), _color_srv.get()};
   dc->PSSetShaderResources(0, srvs.size(), srvs.data());

   const CD3D11_VIEWPORT viewport{0.0f, 0.0f, static_cast<float>(texture_desc.Width),
                                  static_cast<float>(texture_desc.Height)};

   dc->RSSetViewports(1, &viewport);
   dc->OMSetBlendState(_composite_blendstate.get(), nullptr, 0xffffffff);

   auto* const rtv = _opaque_rtv.get();
   dc->OMSetRenderTargets(1, &rtv, nullptr);

   dc->Draw(3, 0);

   dc->FinishCommandList(false, _resolve_commandlist.clear_and_assign());
}
}
