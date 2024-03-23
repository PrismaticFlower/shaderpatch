
#include "ssao.hpp"
#include "../user_config.hpp"

#pragma warning(disable : 4201)

#include "assao/ASSAO.h"

namespace sp::effects {

using namespace std::literals;

namespace {
auto create_assao(ID3D11Device4& device, shader::Database& shaders)
   -> std::unique_ptr<ASSAO_Effect, std::add_pointer_t<void(ASSAO_Effect*)>>
{
   const ASSAO_CreateDescDX11 desc{&device, &shaders};

   return {ASSAO_Effect::CreateInstance(&desc), &ASSAO_Effect::DestroyInstance};
}
}

SSAO::SSAO(Com_ptr<ID3D11Device4> device, shader::Database& shaders) noexcept
   : _device{std::move(device)}, _assao_effect{create_assao(*_device, shaders)}
{
}

void SSAO::params(const SSAO_params params) noexcept
{
   _params = params;
   _output_rtv = nullptr;
   _depth_srv = nullptr;
}

auto SSAO::params() const noexcept -> SSAO_params
{
   return _params;
}

bool SSAO::enabled() const noexcept
{
   return _params.enabled && user_config.effects.ssao;
}

void SSAO::apply(effects::Profiler& profiler, ID3D11DeviceContext4& dc,
                 ID3D11ShaderResourceView& depth_input,
                 ID3D11RenderTargetView& output, const glm::mat4& proj_matrix) noexcept
{
   if (&depth_input != _depth_srv || &output != _output_rtv ||
       update_quality_level() || update_proj_matrix(proj_matrix)) {
      update_resources(depth_input, output);
      record_commandlist();
   }

   Profile profile{profiler, dc, "ASSAO"sv};

   dc.ExecuteCommandList(_commandlist.get(), false);
}

void SSAO::clear_resources() noexcept
{
   _depth_srv = nullptr;
   _output_rtv = nullptr;
   _commandlist = nullptr;
   _assao_effect->DeleteAllocatedVideoMemory();
}

void SSAO::update_resources(ID3D11ShaderResourceView& depth_input,
                            ID3D11RenderTargetView& output) noexcept
{
   _depth_srv = copy_raw_com_ptr(depth_input);
   _output_rtv = copy_raw_com_ptr(output);
}

bool SSAO::update_quality_level() noexcept
{
   int new_quality = -1;

   switch (user_config.effects.ssao_quality) {
   case SSAO_quality::fastest:
      new_quality = -1;
      break;
   case SSAO_quality::fast:
      new_quality = 0;
      break;
   case SSAO_quality::medium:
      new_quality = 1;
      break;
   case SSAO_quality::high:
      new_quality = 2;
      break;
   case SSAO_quality::highest:
      new_quality = 3;
      break;
   }

   if (std::exchange(_quality_level, new_quality) == new_quality) return false;

   return true;
}

bool SSAO::update_proj_matrix(const glm::mat4& new_proj_matrix) noexcept
{
   const bool changed = _proj_matrix != new_proj_matrix;

   if (changed) _proj_matrix = new_proj_matrix;

   return changed;
}

void SSAO::record_commandlist() noexcept
{
   Com_ptr<ID3D11DeviceContext3> dc;
   _device->CreateDeferredContext3(0, dc.clear_and_assign());

   const auto input_desc = [&] {
      Com_ptr<ID3D11Resource> resource;
      _depth_srv->GetResource(resource.clear_and_assign());

      Com_ptr<ID3D11Texture2D> texture;

      if (const auto hr = resource->QueryInterface(texture.clear_and_assign());
          FAILED(hr)) {
         std::terminate();
      }

      D3D11_TEXTURE2D_DESC desc;

      texture->GetDesc(&desc);

      return desc;
   }();

   ASSAO_Settings settings{};
   ASSAO_InputsDX11 inputs{};

   settings.QualityLevel = _quality_level;
   settings.Radius = std::clamp(_params.radius, 0.1f, 2.0f);
   settings.ShadowMultiplier = std::clamp(_params.shadow_multiplier, 0.0f, 5.0f);
   settings.ShadowPower = std::clamp(_params.shadow_power, 0.0f, 5.0f);
   settings.DetailShadowStrength =
      std::clamp(_params.detail_shadow_strength, 0.0f, 5.0f);
   settings.BlurPassCount = std::clamp(_params.blur_pass_count, 0, 6);
   settings.Sharpness = std::clamp(_params.sharpness, 0.0f, 1.0f);

   inputs.ScissorLeft = 0;
   inputs.ScissorTop = 0;
   inputs.ScissorRight = input_desc.Width;
   inputs.ScissorBottom = input_desc.Height;

   inputs.ViewportX = 0;
   inputs.ViewportY = 0;
   inputs.ViewportWidth = input_desc.Width;
   inputs.ViewportHeight = input_desc.Height;

   static_assert(sizeof(inputs.ProjectionMatrix) == sizeof(_proj_matrix));
   std::memcpy(&inputs.ProjectionMatrix, &_proj_matrix, sizeof(_proj_matrix));

   inputs.MatricesRowMajorOrder = true;
   inputs.NormalsUnpackMul = 1.0f;
   inputs.NormalsUnpackAdd = 0.0f;
   inputs.DrawOpaque = false;

   inputs.DeviceContext = dc.get();
   inputs.DepthSRV = _depth_srv.get();
   inputs.OverrideOutputRTV = _output_rtv.get();
   inputs.NormalSRV = nullptr;

   _assao_effect->Draw(settings, &inputs);

   dc->FinishCommandList(false, _commandlist.clear_and_assign());
}
}
