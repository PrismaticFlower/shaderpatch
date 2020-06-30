
#include "postprocess.hpp"
#include "enum_flags.hpp"
#include "utility.hpp"

namespace sp::effects {

namespace {
enum class Postprocess_flags {
   bloom_active = 0b1,
   bloom_use_dirt = 0b10,
   vignette_active = 0b100,
   film_grain_active = 0b1000,
   film_grain_colored = 0b10000
};

enum class Postprocess_cmaa2_post_flags {
   film_grain_active = 0b1,
   film_grain_colored = 0b10
};

constexpr bool marked_as_enum_flag(Postprocess_flags) noexcept
{
   return true;
}

constexpr bool marked_as_enum_flag(Postprocess_cmaa2_post_flags) noexcept
{
   return true;
}

void do_pass(ID3D11DeviceContext1& dc, ID3D11ShaderResourceView& input,
             ID3D11RenderTargetView& output) noexcept
{
   ID3D11ShaderResourceView* const null_srv = nullptr;
   dc.PSSetShaderResources(0, 1, &null_srv);

   auto* const rtv = &output;
   dc.OMSetRenderTargets(1, &rtv, nullptr);

   auto* const srv = &input;
   dc.PSSetShaderResources(0, 1, &srv);

   dc.Draw(3, 0);
}

void do_pass(ID3D11DeviceContext1& dc, std::array<ID3D11ShaderResourceView*, 5> inputs,
             ID3D11RenderTargetView& output) noexcept
{
   auto* const rtv = &output;
   dc.OMSetRenderTargets(1, &rtv, nullptr);

   dc.PSSetShaderResources(0, inputs.size(), inputs.data());

   dc.Draw(3, 0);
}

void do_pass(ID3D11DeviceContext1& dc, std::array<ID3D11ShaderResourceView*, 5> inputs,
             std::array<ID3D11RenderTargetView*, 2> outputs) noexcept
{
   dc.OMSetRenderTargets(outputs.size(), outputs.data(), nullptr);

   dc.PSSetShaderResources(0, inputs.size(), inputs.data());

   dc.Draw(3, 0);
}

}

using namespace std::literals;

Postprocess::Postprocess(Com_ptr<ID3D11Device1> device,
                         const core::Shader_group_collection& shader_groups)
   : _device{device},
     _fullscreen_vs{
        std::get<0>(shader_groups.at("postprocess"s).vertex.at("main_vs"s).copy())},
     _postprocess_ps_ep{shader_groups.at("postprocess"s).pixel.at("main_ps"s)},
     _postprocess_cmaa2_pre_ps_ep{
        shader_groups.at("postprocess"s).pixel.at("main_cmaa2_pre_ps"s)},
     _postprocess_cmaa2_post_ps_ep{
        shader_groups.at("postprocess"s).pixel.at("main_cmaa2_post_ps"s)},
     _stock_hdr_to_linear_ps{
        shader_groups.at("postprocess"s).pixel.at("stock_hdr_to_linear_ps"s).copy()},
     _msaa_hdr_resolve_x2_ps{
        shader_groups.at("postprocess_msaa_resolve"s).pixel.at("main_x2_ps"s).copy()},
     _msaa_hdr_resolve_x4_ps{
        shader_groups.at("postprocess_msaa_resolve"s).pixel.at("main_x4_ps"s).copy()},
     _msaa_hdr_resolve_x8_ps{
        shader_groups.at("postprocess_msaa_resolve"s).pixel.at("main_x8_ps"s).copy()},
     _msaa_stock_hdr_resolve_x2_ps{shader_groups.at("postprocess_msaa_resolve"s)
                                      .pixel.at("main_stock_hdr_x2_ps"s)
                                      .copy()},
     _msaa_stock_hdr_resolve_x4_ps{shader_groups.at("postprocess_msaa_resolve"s)
                                      .pixel.at("main_stock_hdr_x4_ps"s)
                                      .copy()},
     _msaa_stock_hdr_resolve_x8_ps{shader_groups.at("postprocess_msaa_resolve"s)
                                      .pixel.at("main_stock_hdr_x8_ps"s)
                                      .copy()},
     _bloom_downsample_ps{
        shader_groups.at("postprocess_bloom"s).pixel.at("downsample_ps"s).copy()},
     _bloom_upsample_ps{
        shader_groups.at("postprocess_bloom"s).pixel.at("upsample_ps"s).copy()},
     _bloom_threshold_ps{
        shader_groups.at("postprocess_bloom"s).pixel.at("threshold_ps"s).copy()},
     _color_grading_lut_baker{_device, shader_groups}
{
   bloom_params(Bloom_params{});
   vignette_params(Vignette_params{});
   color_grading_params(Color_grading_params{});
   film_grain_params(Film_grain_params{});
}

void Postprocess::bloom_params(const Bloom_params& params) noexcept
{
   _color_grading_regions_blender.global_bloom_params(params);

   _bloom_enabled = user_config.effects.bloom && params.enabled;
   _bloom_use_dirt = params.use_dirt;
   _bloom_dirt_texture_name = params.dirt_texture_name;

   _config_changed = true;
}

void Postprocess::vignette_params(const Vignette_params& params) noexcept
{
   _vignette_params = params;

   _global_constants.vignette_end = _vignette_params.end;
   _global_constants.vignette_start = _vignette_params.start;

   _vignette_enabled = user_config.effects.vignette && _vignette_params.enabled;

   _config_changed = true;
}

void Postprocess::color_grading_params(const Color_grading_params& params) noexcept
{
   _color_grading_regions_blender.global_cg_params(params);

   _config_changed = true;
}

void Postprocess::film_grain_params(const Film_grain_params& params) noexcept
{
   _film_grain_params = params;

   _global_constants.film_grain_amount = params.amount;
   _global_constants.film_grain_color_amount = params.color_amount;
   _global_constants.film_grain_luma_amount = params.luma_amount;

   _film_grain_enabled = user_config.effects.film_grain && _film_grain_params.enabled;
   _colored_film_grain_enabled =
      user_config.effects.colored_film_grain && _film_grain_params.colored;

   _config_changed = true;
}

void Postprocess::color_grading_regions(const Color_grading_regions& colorgrading_regions) noexcept
{
   _color_grading_regions_blender.regions(colorgrading_regions);

   _config_changed = true;
}

void Postprocess::hdr_state(Hdr_state state) noexcept
{
   _hdr_state = state;

   _config_changed = true;
}

void Postprocess::apply(ID3D11DeviceContext1& dc,
                        Rendertarget_allocator& rt_allocator, Profiler& profiler,
                        const core::Shader_resource_database& textures,
                        const glm::vec3 camera_position, FFX_cas& ffx_cas,
                        const Postprocess_input input,
                        const Postprocess_output output) noexcept
{
   dc.ClearState();
   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   dc.VSSetShader(_fullscreen_vs.get(), nullptr, 0);

   auto* const sampler = _linear_clamp_sampler.get();
   dc.PSSetSamplers(0, 1, &sampler);

   update_colorgrading_bloom(dc, camera_position);
   update_shaders();
   update_and_bind_cb(dc, input.width, input.height);

   const auto process_format = _hdr_state == Hdr_state::hdr
                                  ? DXGI_FORMAT_R16G16B16A16_FLOAT
                                  : DXGI_FORMAT_R11G11B10_FLOAT;

   const bool stock_hdr = _hdr_state == Hdr_state::stock;
   const bool msaa_input = input.sample_count != 1;

   auto linear_target = (stock_hdr | msaa_input)
                           ? std::optional{rt_allocator.allocate(
                                {.format = process_format,
                                 .width = input.width,
                                 .height = input.height,
                                 .bind_flags = rendertarget_bind_srv_rtv})}
                           : std::nullopt;

   const Postprocess_input linear_input = [&] {
      if (msaa_input) {
         msaa_resolve_input(dc, input, *linear_target, profiler);

         return Postprocess_input{linear_target->srv(), process_format,
                                  input.width, input.height, 1};
      }
      else if (stock_hdr) {
         linearize_input(dc, input, *linear_target, profiler);

         return Postprocess_input{linear_target->srv(), process_format,
                                  input.width, input.height, 1};
      }

      return input;
   }();

   if (ffx_cas.params().enabled) {
      auto intermediate_target =
         rt_allocator.allocate({.format = DXGI_FORMAT_R8G8B8A8_TYPELESS,
                                .width = input.width,
                                .height = input.height,
                                .bind_flags = rendertarget_bind_srv_rtv,
                                .srv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                .rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                .uav_format = DXGI_FORMAT_R8G8B8A8_UNORM});

      const Postprocess_output intermediate_output{intermediate_target.rtv(),
                                                   input.width, input.height};

      // Apply main postprocessing.
      if (_bloom_enabled) {
         do_bloom_and_color_grading(dc, rt_allocator, textures, linear_input,
                                    intermediate_output, profiler,
                                    *_postprocess_cmaa2_pre_ps);
      }
      else {
         do_color_grading(dc, textures, linear_input, intermediate_output,
                          profiler, *_postprocess_cmaa2_pre_ps);
      }

      auto cas_target =
         rt_allocator.allocate({.format = DXGI_FORMAT_R8G8B8A8_TYPELESS,
                                .width = input.width,
                                .height = input.height,
                                .bind_flags = rendertarget_bind_srv_rtv_uav,
                                .srv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                .rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                .uav_format = DXGI_FORMAT_R8G8B8A8_UNORM});

      ffx_cas.apply(dc, profiler,
                    {.width = input.width,
                     .height = input.height,
                     .input = intermediate_target.srv(),
                     .output = cas_target.uav()});

      // Copy the sharpened scene into the backbuffer, apply dithering and film grain.
      {
         Profile profile{profiler, dc, "Postprocessing - Finalize Output"};

         dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
         dc.VSSetShader(_fullscreen_vs.get(), nullptr, 0);

         auto* const cb = _global_constant_buffer.get();
         dc.PSSetSamplers(0, 1, &sampler);
         dc.PSSetConstantBuffers(global_cb_slot, 1, &cb);
         dc.PSSetShader(_postprocess_cmaa2_post_ps.get(), nullptr, 0);

         set_viewport(dc, output.width, output.height);
         do_pass(dc, cas_target.srv(), output.rtv);
      }
   }
   else {
      if (_bloom_enabled)
         do_bloom_and_color_grading(dc, rt_allocator, textures, linear_input,
                                    output, profiler, *_postprocess_ps);
      else
         do_color_grading(dc, textures, linear_input, output, profiler, *_postprocess_ps);
   }
}

void Postprocess::apply(ID3D11DeviceContext1& dc,
                        Rendertarget_allocator& rt_allocator, Profiler& profiler,
                        const core::Shader_resource_database& textures,
                        const glm::vec3 camera_position, FFX_cas& ffx_cas,
                        CMAA2& cmaa2, const Postprocess_input input,
                        const Postprocess_output output) noexcept
{
   dc.ClearState();
   dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
   dc.VSSetShader(_fullscreen_vs.get(), nullptr, 0);

   auto* const sampler = _linear_clamp_sampler.get();
   dc.PSSetSamplers(0, 1, &sampler);

   update_colorgrading_bloom(dc, camera_position);
   update_shaders();
   update_and_bind_cb(dc, input.width, input.height);

   const auto process_format = _hdr_state == Hdr_state::hdr
                                  ? DXGI_FORMAT_R16G16B16A16_FLOAT
                                  : DXGI_FORMAT_R11G11B10_FLOAT;
   const bool stock_hdr = _hdr_state == Hdr_state::stock;

   auto linear_target = stock_hdr ? std::optional{rt_allocator.allocate(
                                       {.format = process_format,
                                        .width = input.width,
                                        .height = input.height,
                                        .bind_flags = rendertarget_bind_srv_rtv})}
                                  : std::nullopt;

   if (stock_hdr) linearize_input(dc, input, *linear_target, profiler);

   const auto linear_input = stock_hdr
                                ? Postprocess_input{.srv = linear_target->srv(),
                                                    .format = process_format,
                                                    .width = input.width,
                                                    .height = input.height,
                                                    .sample_count = 1}
                                : input;

   const bool cas_enabled = ffx_cas.params().enabled;

   auto intermediate_target = rt_allocator.allocate(
      {.format = DXGI_FORMAT_R8G8B8A8_TYPELESS,
       .width = input.width,
       .height = input.height,
       .bind_flags = cas_enabled ? rendertarget_bind_srv_rtv : rendertarget_bind_srv_rtv_uav,
       .srv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
       .rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
       .uav_format = DXGI_FORMAT_R8G8B8A8_UNORM});

   const Postprocess_output intermediate_output{intermediate_target.rtv(),
                                                input.width, input.height};

   auto luma_target =
      rt_allocator.allocate({.format = DXGI_FORMAT_R8_UNORM,
                             .width = input.width,
                             .height = input.height,
                             .bind_flags = rendertarget_bind_srv_rtv});

   // Apply main postprocessing.
   if (_bloom_enabled) {
      do_bloom_and_color_grading(dc, rt_allocator, textures, linear_input,
                                 intermediate_output, profiler,
                                 *_postprocess_cmaa2_pre_ps, &luma_target.rtv());
   }
   else {
      do_color_grading(dc, textures, linear_input, intermediate_output,
                       profiler, *_postprocess_cmaa2_pre_ps, &luma_target.rtv());
   }

   auto cas_target = cas_enabled
                        ? std::optional{rt_allocator.allocate(
                             {.format = DXGI_FORMAT_R8G8B8A8_TYPELESS,
                              .width = input.width,
                              .height = input.height,
                              .bind_flags = rendertarget_bind_srv_rtv_uav,
                              .srv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                              .rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                              .uav_format = DXGI_FORMAT_R8G8B8A8_UNORM})}
                        : std::nullopt;

   if (cas_enabled) {
      ffx_cas.apply(dc, profiler,
                    {.width = input.width,
                     .height = input.height,
                     .input = intermediate_target.srv(),
                     .output = cas_target->uav()});
   }

   auto& final_target = cas_enabled ? *cas_target : intermediate_target;

   // Apply anti-aliasing.
   cmaa2.apply(dc, profiler,
               {.input = final_target.srv(),
                .output = final_target.uav(),
                .luma_srv = &luma_target.srv(),
                .width = input.width,
                .height = input.height});

   // Copy the anti-aliased scene into the backbuffer, apply dithering and film grain.
   {
      Profile profile{profiler, dc, "Postprocessing - Finalize Output"};

      dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      dc.VSSetShader(_fullscreen_vs.get(), nullptr, 0);

      auto* const cb = _global_constant_buffer.get();
      dc.PSSetSamplers(0, 1, &sampler);
      dc.PSSetConstantBuffers(global_cb_slot, 1, &cb);
      dc.PSSetShader(_postprocess_cmaa2_post_ps.get(), nullptr, 0);

      set_viewport(dc, output.width, output.height);
      do_pass(dc, final_target.srv(), output.rtv);
   }
}

void Postprocess::msaa_resolve_input(ID3D11DeviceContext1& dc,
                                     const Postprocess_input& input,
                                     Rendertarget_allocator::Handle& output,
                                     Profiler& profiler) noexcept
{
   Expects(input.sample_count > 1);

   Profile profile{profiler, dc, "Postprocessing - AA Resolve"};

   // Update constants buffer.
   const Resolve_constants constants{{input.width - 1, input.height - 1},
                                     _global_constants.exposure};

   core::update_dynamic_buffer(dc, *_msaa_resolve_constant_buffer, constants);

   auto* const srv = &input.srv;
   auto* const rtv = &output.rtv();
   auto* const shader = select_msaa_resolve_shader(input);

   dc.OMSetBlendState(nullptr, nullptr, 0xffffffff);
   set_viewport(dc, input.width, input.height);

   dc.PSSetShaderResources(0, 1, &srv);
   dc.PSSetShader(shader, nullptr, 0);
   dc.OMSetRenderTargets(1, &rtv, nullptr);

   dc.Draw(3, 0);
}

void Postprocess::linearize_input(ID3D11DeviceContext1& dc,
                                  const Postprocess_input& input,
                                  Rendertarget_allocator::Handle& output,
                                  Profiler& profiler) noexcept
{
   Expects(input.sample_count == 1);

   Profile profile{profiler, dc, "Postprocessing - Linearize Input"};

   auto* const srv = &input.srv;
   auto* const rtv = &output.rtv();
   set_viewport(dc, input.width, input.height);

   dc.PSSetShaderResources(0, 1, &srv);
   dc.PSSetShader(_stock_hdr_to_linear_ps.get(), nullptr, 0);
   dc.OMSetRenderTargets(1, &rtv, nullptr);

   dc.Draw(3, 0);
}

void Postprocess::do_bloom_and_color_grading(
   ID3D11DeviceContext1& dc, Rendertarget_allocator& allocator,
   const core::Shader_resource_database& textures, const Postprocess_input& input,
   const Postprocess_output& output, Profiler& profiler,
   ID3D11PixelShader& postprocess_shader, ID3D11RenderTargetView* luma_rtv) noexcept
{
   Expects(input.sample_count == 1);

   auto rt_a = allocator.allocate({.format = input.format,
                                   .width = input.width / 2,
                                   .height = input.height / 2,
                                   .bind_flags = rendertarget_bind_srv_rtv});
   auto rt_b = allocator.allocate({.format = input.format,
                                   .width = input.width / 4,
                                   .height = input.height / 4,
                                   .bind_flags = rendertarget_bind_srv_rtv});
   auto rt_c = allocator.allocate({.format = input.format,
                                   .width = input.width / 8,
                                   .height = input.height / 8,
                                   .bind_flags = rendertarget_bind_srv_rtv});
   auto rt_d = allocator.allocate({.format = input.format,
                                   .width = input.width / 16,
                                   .height = input.height / 16,
                                   .bind_flags = rendertarget_bind_srv_rtv});
   auto rt_e = allocator.allocate({.format = input.format,
                                   .width = input.width / 32,
                                   .height = input.height / 32,
                                   .bind_flags = rendertarget_bind_srv_rtv});

   // Bloom Threshold
   {
      Profile profile{profiler, dc, "Postprocessing - Bloom Threshold"};

      dc.PSSetShader(_bloom_threshold_ps.get(), nullptr, 0);

      bind_bloom_cb(dc, 0);
      set_viewport(dc, input.width / 2, input.height / 2);
      do_pass(dc, input.srv, rt_a.rtv());
   }

   // Bloom Downsample
   {
      Profile profile{profiler, dc, "Postprocessing - Bloom Downsample"};

      dc.PSSetShader(_bloom_downsample_ps.get(), nullptr, 0);

      bind_bloom_cb(dc, 1);
      set_viewport(dc, input.width / 4, input.height / 4);
      do_pass(dc, rt_a.srv(), rt_b.rtv());

      bind_bloom_cb(dc, 2);
      set_viewport(dc, input.width / 8, input.height / 8);
      do_pass(dc, rt_b.srv(), rt_c.rtv());

      bind_bloom_cb(dc, 3);
      set_viewport(dc, input.width / 16, input.height / 16);
      do_pass(dc, rt_c.srv(), rt_d.rtv());

      bind_bloom_cb(dc, 4);
      set_viewport(dc, input.width / 32, input.height / 32);
      do_pass(dc, rt_d.srv(), rt_e.rtv());
   }

   // Bloom Upsample
   {
      Profile profile{profiler, dc, "Postprocessing - Bloom Upsample"};

      dc.PSSetShader(_bloom_upsample_ps.get(), nullptr, 0);
      dc.OMSetBlendState(_additive_blend_state.get(), nullptr, 0xffffffff);

      bind_bloom_cb(dc, 5);
      set_viewport(dc, input.width / 16, input.height / 16);
      do_pass(dc, rt_e.srv(), rt_d.rtv());

      bind_bloom_cb(dc, 4);
      set_viewport(dc, input.width / 8, input.height / 8);
      do_pass(dc, rt_d.srv(), rt_c.rtv());

      bind_bloom_cb(dc, 3);
      set_viewport(dc, input.width / 4, input.height / 4);
      do_pass(dc, rt_c.srv(), rt_b.rtv());

      bind_bloom_cb(dc, 2);
      set_viewport(dc, input.width / 2, input.height / 2);
      do_pass(dc, rt_b.srv(), rt_a.rtv());
   }

   Profile profile{profiler, dc, "Postprocessing - Final"};

   std::array<ID3D11ShaderResourceView*, 5> srvs{};

   srvs[scene_texture_slot] = &input.srv;
   srvs[bloom_texture_slot] = &rt_a.srv();
   srvs[dirt_texture_slot] =
      _bloom_use_dirt ? textures.at_if(_bloom_dirt_texture_name).get() : nullptr;
   srvs[lut_texture_slot] = _color_grading_lut_baker.srv();
   srvs[blue_noise_texture_slot] = blue_noise_srv(textures);

   dc.PSSetShader(&postprocess_shader, nullptr, 0);
   bind_bloom_cb(dc, 1);
   dc.OMSetBlendState(nullptr, nullptr, 0xffffffff);
   set_viewport(dc, output.width, output.height);

   if (luma_rtv)
      do_pass(dc, srvs, {&output.rtv, luma_rtv});
   else
      do_pass(dc, srvs, output.rtv);
}

void Postprocess::do_color_grading(ID3D11DeviceContext1& dc,
                                   const core::Shader_resource_database& textures,
                                   const Postprocess_input& input,
                                   const Postprocess_output& output, Profiler& profiler,
                                   ID3D11PixelShader& postprocess_shader,
                                   ID3D11RenderTargetView* luma_rtv) noexcept
{
   Expects(input.sample_count == 1);

   Profile profile{profiler, dc, "Postprocessing - Final"};

   std::array<ID3D11ShaderResourceView*, 5> srvs{};
   set_viewport(dc, output.width, output.height);

   srvs[scene_texture_slot] = &input.srv;
   srvs[lut_texture_slot] = _color_grading_lut_baker.srv();
   srvs[blue_noise_texture_slot] = blue_noise_srv(textures);

   dc.PSSetShader(&postprocess_shader, nullptr, 0);

   if (luma_rtv)
      do_pass(dc, srvs, {&output.rtv, luma_rtv});
   else
      do_pass(dc, srvs, output.rtv);
}

auto Postprocess::select_msaa_resolve_shader(const Postprocess_input& input) const noexcept
   -> ID3D11PixelShader*
{
   Expects(input.sample_count > 1);

   if (_hdr_state == Hdr_state::hdr) {
      if (input.sample_count == 2) return _msaa_hdr_resolve_x2_ps.get();
      if (input.sample_count == 4) return _msaa_hdr_resolve_x4_ps.get();
      if (input.sample_count == 8) return _msaa_hdr_resolve_x8_ps.get();
   }
   else {
      if (input.sample_count == 2) return _msaa_stock_hdr_resolve_x2_ps.get();
      if (input.sample_count == 4) return _msaa_stock_hdr_resolve_x4_ps.get();
      if (input.sample_count == 8) return _msaa_stock_hdr_resolve_x8_ps.get();
   }

   std::terminate();
}

void Postprocess::update_and_bind_cb(ID3D11DeviceContext1& dc, const UINT width,
                                     const UINT height) noexcept
{
   update_randomness();

   const auto res = glm::uvec2{width, height};
   const auto flt_res = static_cast<glm::vec2>(res);

   _global_constants.scene_pixel_size = 1.0f / flt_res;
   _global_constants.film_grain_aspect = flt_res.x / flt_res.y;
   _global_constants.film_grain_size = flt_res / _film_grain_params.size;

   _bloom_constants[0].bloom_texel_size = 1.0f / static_cast<glm::vec2>(res);
   _bloom_constants[1].bloom_texel_size = 1.0f / static_cast<glm::vec2>(res / 2u);
   _bloom_constants[2].bloom_texel_size = 1.0f / static_cast<glm::vec2>(res / 4u);
   _bloom_constants[3].bloom_texel_size = 1.0f / static_cast<glm::vec2>(res / 8u);
   _bloom_constants[4].bloom_texel_size = 1.0f / static_cast<glm::vec2>(res / 16u);
   _bloom_constants[5].bloom_texel_size = 1.0f / static_cast<glm::vec2>(res / 32u);

   core::update_dynamic_buffer(dc, *_global_constant_buffer, _global_constants);

   for (auto i = 0; i < _bloom_constants.size(); ++i) {
      core::update_dynamic_buffer(dc, *_bloom_constant_buffers[i],
                                  _bloom_constants[i]);
   }

   auto* const cb = _global_constant_buffer.get();
   dc.PSSetConstantBuffers(global_cb_slot, 1, &cb);
}

void Postprocess::bind_bloom_cb(ID3D11DeviceContext1& dc, const UINT index) noexcept
{
   auto* const cb = _bloom_constant_buffers[index].get();

   dc.PSSetConstantBuffers(bloom_cb_slot, 1, &cb);
}

auto Postprocess::blue_noise_srv(const core::Shader_resource_database& textures) noexcept
   -> ID3D11ShaderResourceView*
{
   if (_blue_noise_srvs.empty()) {
      for (int i = 0; i < 64; ++i) {
         _blue_noise_srvs.emplace_back(
            textures.at_if("_SP_BUILTIN_blue_noise_rgb_"s + std::to_string(i)));
      }
   }

   return _blue_noise_srvs[_random_int_dist(_random_engine)].get();
}

void Postprocess::update_colorgrading_bloom(ID3D11DeviceContext1& dc,
                                            const glm::vec3 camera_position) noexcept
{
   const auto [cg_params, bloom_params] =
      _color_grading_regions_blender.blend(camera_position);

   _global_constants.exposure = glm::exp2(cg_params.exposure) * cg_params.brightness;
   _global_constants.bloom_threshold = bloom_params.threshold;
   _global_constants.bloom_global_scale = bloom_params.tint * bloom_params.intensity;
   _global_constants.bloom_dirt_scale = bloom_params.dirt_scale * bloom_params.dirt_tint;

   _bloom_constants[0].bloom_local_scale = glm::vec3{1.0f, 1.0f, 1.0f};
   _bloom_constants[1].bloom_local_scale =
      bloom_params.inner_tint * bloom_params.inner_scale;
   _bloom_constants[2].bloom_local_scale =
      bloom_params.inner_mid_scale * bloom_params.inner_mid_tint;
   _bloom_constants[3].bloom_local_scale =
      bloom_params.mid_scale * bloom_params.mid_tint;
   _bloom_constants[4].bloom_local_scale =
      bloom_params.outer_mid_scale * bloom_params.outer_mid_tint;
   _bloom_constants[5].bloom_local_scale =
      bloom_params.outer_scale * bloom_params.outer_tint;

   _bloom_constants[2].bloom_local_scale /= _bloom_constants[1].bloom_local_scale;
   _bloom_constants[3].bloom_local_scale /= _bloom_constants[2].bloom_local_scale;
   _bloom_constants[4].bloom_local_scale /= _bloom_constants[3].bloom_local_scale;
   _bloom_constants[5].bloom_local_scale /= _bloom_constants[4].bloom_local_scale;

   _color_grading_lut_baker.bake_color_grading_lut(dc, cg_params);
}

void Postprocess::update_randomness() noexcept
{
   _global_constants.randomness_flt = {_random_real_dist(_random_engine),
                                       _random_real_dist(_random_engine),
                                       _random_real_dist(_random_engine),
                                       _random_real_dist(_random_engine)};

   _global_constants.randomness_uint = {_random_engine(), _random_engine(),
                                        _random_engine(), _random_engine()};
}

void Postprocess::update_shaders() noexcept
{
   if (!std::exchange(_config_changed, false)) return;

   Postprocess_flags postprocess_flags{};

   if (_bloom_enabled) {
      postprocess_flags |= Postprocess_flags::bloom_active;

      if (_bloom_use_dirt)
         postprocess_flags |= Postprocess_flags::bloom_use_dirt;
   }

   if (_vignette_enabled)
      postprocess_flags |= Postprocess_flags::vignette_active;

   const auto postprocess_cmaa2_pre_flags = postprocess_flags;
   Postprocess_cmaa2_post_flags postprocess_cmaa2_post_flags{};

   if (_film_grain_enabled) {
      postprocess_flags |= Postprocess_flags::film_grain_active;
      postprocess_cmaa2_post_flags |= Postprocess_cmaa2_post_flags::film_grain_active;

      if (_colored_film_grain_enabled) {
         postprocess_flags |= Postprocess_flags::film_grain_colored;
         postprocess_cmaa2_post_flags |=
            Postprocess_cmaa2_post_flags::film_grain_colored;
      }
   }

   _postprocess_ps =
      _postprocess_ps_ep.copy(static_cast<std::uint16_t>(postprocess_flags));
   _postprocess_cmaa2_pre_ps = _postprocess_cmaa2_pre_ps_ep.copy(
      static_cast<std::uint16_t>(postprocess_cmaa2_pre_flags));
   _postprocess_cmaa2_post_ps = _postprocess_cmaa2_post_ps_ep.copy(
      static_cast<std::uint16_t>(postprocess_cmaa2_post_flags));
}

}
