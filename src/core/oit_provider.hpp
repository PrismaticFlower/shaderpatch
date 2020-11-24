#pragma once

#include "../shader/database.hpp"
#include "com_ptr.hpp"

#include <d3d11_4.h>

namespace sp::core {

class OIT_provider {
public:
   OIT_provider(Com_ptr<ID3D11Device5> device, shader::Database& shaders) noexcept;

   ~OIT_provider() noexcept = default;

   OIT_provider(const OIT_provider&) = delete;
   OIT_provider& operator=(const OIT_provider&) = delete;

   OIT_provider(OIT_provider&&) = delete;
   OIT_provider& operator=(OIT_provider&&) = delete;

   void prepare_resources(ID3D11DeviceContext4& dc, ID3D11Texture2D& opaque_texture,
                          ID3D11RenderTargetView& opaque_rtv) noexcept;

   void clear_resources() noexcept;

   void resolve(ID3D11DeviceContext4& dc) const noexcept;

   auto uavs() const noexcept -> std::array<ID3D11UnorderedAccessView*, 3>;

   bool enabled() const noexcept;

private:
   void update_resources(ID3D11Texture2D& opaque_texture,
                         ID3D11RenderTargetView& opaque_rtv) noexcept;

   void record_resolve_commandlist() noexcept;

   const Com_ptr<ID3D11Device5> _device;
   const Com_ptr<ID3D11VertexShader> _vs;
   const Com_ptr<ID3D11PixelShader> _ps;
   const Com_ptr<ID3D11BlendState1> _composite_blendstate = [this] {
      CD3D11_BLEND_DESC1 desc{CD3D11_DEFAULT{}};

      desc.RenderTarget[0].BlendEnable = true;
      desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
      desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
      desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;
      desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_SRC_ALPHA;
      desc.RenderTarget[0].RenderTargetWriteMask = 0b111;

      Com_ptr<ID3D11BlendState1> blendstate;

      _device->CreateBlendState1(&desc, blendstate.clear_and_assign());

      return blendstate;
   }();

   Com_ptr<ID3D11Texture2D> _opaque_texture;
   Com_ptr<ID3D11RenderTargetView> _opaque_rtv;
   Com_ptr<ID3D11UnorderedAccessView> _clear_uav;
   Com_ptr<ID3D11ShaderResourceView> _clear_srv;
   Com_ptr<ID3D11UnorderedAccessView> _depth_uav;
   Com_ptr<ID3D11ShaderResourceView> _depth_srv;
   Com_ptr<ID3D11UnorderedAccessView> _color_uav;
   Com_ptr<ID3D11ShaderResourceView> _color_srv;

   Com_ptr<ID3D11CommandList> _resolve_commandlist;

   const bool _usable = [this] {
      D3D11_FEATURE_DATA_D3D11_OPTIONS2 data{};

      _device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &data, sizeof(data));

      return data.TypedUAVLoadAdditionalFormats && data.ROVsSupported;
   }();
};
}
