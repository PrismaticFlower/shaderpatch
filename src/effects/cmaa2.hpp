#pragma once

#include "../shader/database.hpp"
#include "com_ptr.hpp"
#include "profiler.hpp"

#include <glm/glm.hpp>

#include <d3d11_1.h>

namespace sp::effects {

struct CMAA2_input_output {
   ID3D11ShaderResourceView& input;
   ID3D11UnorderedAccessView& output;
   ID3D11ShaderResourceView* luma_srv = nullptr;

   UINT width;
   UINT height;
};

class CMAA2 {
public:
   CMAA2(Com_ptr<ID3D11Device1> device, shader::Database& shaders) noexcept;

   void apply(ID3D11DeviceContext1& dc, Profiler& profiler,
              CMAA2_input_output input_output) noexcept;

   void clear_resources() noexcept;

private:
   void execute(ID3D11DeviceContext1& dc, Profiler& profiler,
                CMAA2_input_output input_output) noexcept;

   void update_resources() noexcept;

   void update_shaders() noexcept;

   const static bool _uav_store_typed = true;
   const static bool _uav_store_convert_to_srgb = true;
   bool _luma_separate_texture = false;

   glm::uvec2 _size{};

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
   shader::Group_compute& _shaders;
};

}
