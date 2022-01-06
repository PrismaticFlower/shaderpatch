
#include "mask_nan.hpp"

namespace sp::effects {

namespace {

auto make_blend_state(ID3D11Device5& device) noexcept -> Com_ptr<ID3D11BlendState>
{
   const D3D11_RENDER_TARGET_BLEND_DESC rt_blend_desc{.BlendEnable = true,
                                                      .SrcBlend = D3D11_BLEND_ONE,
                                                      .DestBlend = D3D11_BLEND_ONE,
                                                      .BlendOp = D3D11_BLEND_OP_MAX,
                                                      .SrcBlendAlpha = D3D11_BLEND_ONE,
                                                      .DestBlendAlpha = D3D11_BLEND_ONE,
                                                      .BlendOpAlpha = D3D11_BLEND_OP_MAX,
                                                      .RenderTargetWriteMask = 0b1111};

   const D3D11_BLEND_DESC desc{.AlphaToCoverageEnable = false,
                               .IndependentBlendEnable = false,
                               .RenderTarget = {rt_blend_desc}};

   Com_ptr<ID3D11BlendState> blend;

   device.CreateBlendState(&desc, blend.clear_and_assign());

   return blend;
}

}

Mask_nan::Mask_nan(Com_ptr<ID3D11Device5> device, shader::Database& shaders) noexcept
   : _vs{std::get<0>(shaders.vertex("postprocess"sv).entrypoint("main_vs"sv))},
     _ps{shaders.pixel("mask_nan"sv).entrypoint("main_ps"sv)},
     _blend_state{make_blend_state(*device)}
{
}

void Mask_nan::apply(ID3D11DeviceContext4& dc, const Mask_nan_input input,
                     Profiler& profiler) noexcept
{
   Profile profile{profiler, dc, "Bugged Cloth Workaround - Mask NaNs"};

   dc.ClearState();

   auto* rtv = &input.rtv;

   const D3D11_VIEWPORT viewport = {.TopLeftX = 0.0f,
                                    .TopLeftY = 0.0f,
                                    .Width = static_cast<float>(input.width),
                                    .Height = static_cast<float>(input.height),
                                    .MinDepth = 0.0f,
                                    .MaxDepth = 1.0f};

   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   dc.VSSetShader(_vs.get(), nullptr, 0);
   dc.RSSetViewports(1, &viewport);
   dc.PSSetShader(_ps.get(), nullptr, 0);
   dc.OMSetBlendState(_blend_state.get(), nullptr, 0xffffffffu);
   dc.OMSetRenderTargets(1, &rtv, nullptr);
   dc.Draw(3, 0);
}

}