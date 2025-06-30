#pragma once

#include "../core/game_rendertarget.hpp"
#include "../core/texture_database.hpp"
#include "../shader/database.hpp"
#include "cmaa2.hpp"
#include "color_grading_regions_io.hpp"
#include "com_ptr.hpp"
#include "ffx_cas.hpp"
#include "helpers.hpp"
#include "postprocess_params.hpp"
#include "profiler.hpp"
#include "rendertarget_allocator.hpp"

#include <memory>

#include <d3d11_4.h>

namespace sp::effects {

struct Postprocess_options {
   bool apply_cmaa2;
};

struct Postprocess_input {
   ID3D11ShaderResourceView& srv;
   ID3D11ShaderResourceView& depth_srv;

   DXGI_FORMAT format;
   UINT width;
   UINT height;
   UINT sample_count;

   glm::mat4 projection_from_view;
};

struct Postprocess_output {
   ID3D11RenderTargetView& rtv;

   UINT width;
   UINT height;
};

class Postprocess {
public:
   Postprocess(Com_ptr<ID3D11Device5> device, shader::Database& shaders);

   ~Postprocess();

   void bloom_params(const Bloom_params& params) noexcept;

   auto bloom_params() const noexcept -> const Bloom_params&;

   void vignette_params(const Vignette_params& params) noexcept;

   auto vignette_params() const noexcept -> const Vignette_params&;

   void color_grading_params(const Color_grading_params& params) noexcept;

   auto color_grading_params() const noexcept -> const Color_grading_params&;

   void film_grain_params(const Film_grain_params& params) noexcept;

   auto film_grain_params() const noexcept -> const Film_grain_params&;

   void dof_params(const DOF_params& params) noexcept;

   auto dof_params() const noexcept -> const DOF_params&;

   void color_grading_regions(const Color_grading_regions& colorgrading_regions) noexcept;

   void show_color_grading_regions_imgui(
      const HWND game_window,
      Small_function<Color_grading_params(Color_grading_params) noexcept> show_cg_params_imgui,
      Small_function<Bloom_params(Bloom_params) noexcept> show_bloom_params_imgui) noexcept;

   void apply(ID3D11DeviceContext1& dc, Rendertarget_allocator& rt_allocator,
              Profiler& profiler, const core::Shader_resource_database& textures,
              const glm::vec3 camera_position, FFX_cas& ffx_cas, CMAA2& cmaa2,
              const Postprocess_input input, const Postprocess_output output,
              const Postprocess_options options) noexcept;

   void hdr_state(Hdr_state state) noexcept;

private:
   class Impl;

   std::unique_ptr<Impl> _impl;
};

}
