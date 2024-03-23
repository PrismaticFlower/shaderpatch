#pragma once

#include "../shader/database.hpp"
#include "com_ptr.hpp"
#include "postprocess_params.hpp"
#include "profiler.hpp"

#include <memory>
#include <type_traits>

#include <d3d11_4.h>

class ASSAO_Effect;

namespace sp::effects {

class SSAO {
public:
   SSAO(Com_ptr<ID3D11Device4> device, shader::Database& shaders) noexcept;

   void params(const SSAO_params params) noexcept;

   auto params() const noexcept -> SSAO_params;

   bool enabled() const noexcept;

   void apply(effects::Profiler& profiler, ID3D11DeviceContext4& dc,
              ID3D11ShaderResourceView& depth_input, ID3D11RenderTargetView& output,
              const glm::mat4& proj_matrix) noexcept;

   void clear_resources() noexcept;

private:
   void update_resources(ID3D11ShaderResourceView& depth_input,
                         ID3D11RenderTargetView& output) noexcept;

   bool update_quality_level() noexcept;

   bool update_proj_matrix(const glm::mat4& new_proj_matrix) noexcept;

   void record_commandlist() noexcept;

   const Com_ptr<ID3D11Device4> _device;
   const std::unique_ptr<ASSAO_Effect, std::add_pointer_t<void(ASSAO_Effect*)>> _assao_effect;

   int _quality_level = 2;

   Com_ptr<ID3D11ShaderResourceView> _depth_srv;
   Com_ptr<ID3D11RenderTargetView> _output_rtv;
   Com_ptr<ID3D11CommandList> _commandlist;

   glm::mat4 _proj_matrix;

   SSAO_params _params{};
};
}
