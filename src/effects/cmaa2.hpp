#pragma once

#include "../core/shader_database.hpp"
#include "com_ptr.hpp"
#include "profiler.hpp"

#include <glm/glm.hpp>

#include <d3d11_1.h>

namespace sp::effects {

class CMAA2 {
public:
   CMAA2(Com_ptr<ID3D11Device1> device, const core::Shader_group_collection& shader_groups)
   noexcept;

   void apply(ID3D11DeviceContext1& dc, Profiler& profiler,
              ID3D11Texture2D& input_output) noexcept;

   void apply(ID3D11DeviceContext1& dc, Profiler& profiler,
              ID3D11Texture2D& input_output, ID3D11ShaderResourceView& luma_srv) noexcept;

   void clear_resources() noexcept;

private:
   void apply_impl(ID3D11DeviceContext1& dc, Profiler& profiler,
                   ID3D11Texture2D& input_output,
                   ID3D11ShaderResourceView* luma_srv) noexcept;

   void execute(ID3D11DeviceContext1& dc, Profiler& profiler,
                ID3D11ShaderResourceView* luma_srv) noexcept;

   void update_resources(ID3D11Texture2D& input_output) noexcept;

   void update_shaders() noexcept;

   bool _uav_store_typed = false;
   bool _uav_store_convert_to_srgb = false;
   bool _luma_separate_texture = false;

   glm::uvec2 _size{};

   Com_ptr<ID3D11Texture2D> _input_output_texture;
   Com_ptr<ID3D11ShaderResourceView> _input_srv;
   Com_ptr<ID3D11UnorderedAccessView> _output_uav;

   Com_ptr<ID3D11UnorderedAccessView> _working_edges_uav;
   Com_ptr<ID3D11UnorderedAccessView> _working_shape_candidates_uav;
   Com_ptr<ID3D11UnorderedAccessView> _working_deferred_blend_location_list_uav;
   Com_ptr<ID3D11UnorderedAccessView> _working_deferred_blend_item_list_uav;
   Com_ptr<ID3D11UnorderedAccessView> _working_deferred_blend_item_list_heads_uav;

   Com_ptr<ID3D11Buffer> _working_control_buffer;
   Com_ptr<ID3D11UnorderedAccessView> _working_control_uav;

   Com_ptr<ID3D11ComputeShader> _edges_color2x2;
   Com_ptr<ID3D11ComputeShader> _compute_dispatch_args;
   Com_ptr<ID3D11ComputeShader> _process_candidates;
   Com_ptr<ID3D11ComputeShader> _deferred_color_apply2x2;

   const Com_ptr<ID3D11SamplerState> _point_sampler;
   const Com_ptr<ID3D11Device1> _device;
   const core::Shader_entrypoints<core::Compute_shader_entrypoint> _shader_entrypoints;
};

}
