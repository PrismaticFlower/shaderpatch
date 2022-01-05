
#include "postprocess.hpp"
#include "../core/d3d11_helpers.hpp"
#include "../core/game_rendertarget.hpp"
#include "../core/texture_database.hpp"
#include "../shader/database.hpp"
#include "../user_config.hpp"
#include "cmaa2.hpp"
#include "color_grading_lut_baker.hpp"
#include "color_grading_regions_blender.hpp"
#include "color_grading_regions_io.hpp"
#include "com_ptr.hpp"
#include "enum_flags.hpp"
#include "ffx_cas.hpp"
#include "helpers.hpp"
#include "postprocess_params.hpp"
#include "profiler.hpp"
#include "rendertarget_allocator.hpp"
#include "utility.hpp"

#include <array>
#include <limits>
#include <optional>
#include <random>
#include <string>

#include <glm/glm.hpp>

namespace sp::effects {

namespace {

class Work_texture {
public:
   Work_texture() noexcept = default;

   Work_texture(Postprocess_input input) noexcept
   {
      _srv = &input.srv;
      _width = input.width;
      _height = input.height;
   }

   Work_texture(Rendertarget_allocator::Handle handle) noexcept
   {
      _srv = handle.srv();
      _rtv = handle.rtv();
      _uav = handle.uav();
      _width = handle.width();
      _height = handle.height();

      _handle.emplace(std::move(handle));
   }

   Work_texture(Work_texture&& other) noexcept
   {
      _srv = std::exchange(other._srv, nullptr);
      _rtv = std::exchange(other._rtv, nullptr);
      _uav = std::exchange(other._uav, nullptr);
      _width = std::exchange(other._width, 0);
      _height = std::exchange(other._height, 0);

      if (other._handle) {
         _handle.emplace(std::move(other._handle.value()));
         other._handle = std::nullopt;
      }
   }

   auto operator=(Work_texture&& other) noexcept -> Work_texture&
   {
      _srv = std::exchange(other._srv, nullptr);
      _rtv = std::exchange(other._rtv, nullptr);
      _uav = std::exchange(other._uav, nullptr);
      _width = std::exchange(other._width, 0);
      _height = std::exchange(other._height, 0);

      if (other._handle) {
         _handle.emplace(std::move(other._handle.value()));
         other._handle = std::nullopt;
      }

      return *this;
   }

   auto srv() const noexcept -> ID3D11ShaderResourceView*
   {
      return _srv;
   }

   auto rtv() const noexcept -> ID3D11RenderTargetView*
   {
      return _rtv;
   }

   auto uav() const noexcept -> ID3D11UnorderedAccessView*
   {
      return _uav;
   }

   auto width() const noexcept -> UINT
   {
      return _width;
   }

   auto height() const noexcept -> UINT
   {
      return _height;
   }

private:
   ID3D11ShaderResourceView* _srv = nullptr;
   ID3D11RenderTargetView* _rtv = nullptr;
   ID3D11UnorderedAccessView* _uav = nullptr;
   UINT _width = 0;
   UINT _height = 0;
   std::optional<Rendertarget_allocator::Handle> _handle = std::nullopt;
};

enum class Postprocess_combine_flags {
   bloom_active = 0b1,
   bloom_use_dirt = 0b10,
   output_luma = 0b100
};

enum class Postprocess_finalize_flags {
   film_grain_active = 0b1,
   film_grain_colored = 0b10,
   vignette_active = 0b100
};

constexpr bool marked_as_enum_flag(Postprocess_combine_flags) noexcept
{
   return true;
}

constexpr bool marked_as_enum_flag(Postprocess_finalize_flags) noexcept
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

class Postprocess::Impl {
public:
   Impl(Com_ptr<ID3D11Device5> device, shader::Database& shaders)
      : _device{device},
        _fullscreen_vs{
           std::get<0>(shaders.vertex("postprocess"sv).entrypoint("main_vs"sv))},
        _shaders{shaders.pixel("postprocess"sv)},
        _stock_hdr_to_linear_ps{
           shaders.pixel("postprocess"sv).entrypoint("stock_hdr_to_linear_ps"sv)},
        _msaa_hdr_resolve_x2_ps{
           shaders.pixel("postprocess_msaa_resolve"sv).entrypoint("main_x2_ps"sv)},
        _msaa_hdr_resolve_x4_ps{
           shaders.pixel("postprocess_msaa_resolve"sv).entrypoint("main_x4_ps"sv)},
        _msaa_hdr_resolve_x8_ps{
           shaders.pixel("postprocess_msaa_resolve"sv).entrypoint("main_x8_ps"sv)},
        _msaa_stock_hdr_resolve_x2_ps{
           shaders.pixel("postprocess_msaa_resolve"sv).entrypoint("main_stock_hdr_x2_ps"sv)},
        _msaa_stock_hdr_resolve_x4_ps{
           shaders.pixel("postprocess_msaa_resolve"sv).entrypoint("main_stock_hdr_x4_ps"sv)},
        _msaa_stock_hdr_resolve_x8_ps{
           shaders.pixel("postprocess_msaa_resolve"sv).entrypoint("main_stock_hdr_x8_ps"sv)},
        _bloom_downsample_ps{
           shaders.pixel("postprocess_bloom"sv).entrypoint("downsample_ps"sv)},
        _bloom_upsample_ps{
           shaders.pixel("postprocess_bloom"sv).entrypoint("upsample_ps"sv)},
        _bloom_threshold_ps{
           shaders.pixel("postprocess_bloom"sv).entrypoint("threshold_ps"sv)},
        _color_grading_lut_baker{_device, shaders}
   {
      bloom_params(Bloom_params{});
      vignette_params(Vignette_params{});
      color_grading_params(Color_grading_params{});
      film_grain_params(Film_grain_params{});
   }

   void bloom_params(const Bloom_params& params) noexcept
   {
      _color_grading_regions_blender.global_bloom_params(params);

      _bloom_enabled = user_config.effects.bloom && params.enabled;
      _bloom_use_dirt = params.use_dirt;
      _bloom_dirt_texture_name = params.dirt_texture_name;

      _config_changed = true;
   }

   auto bloom_params() const noexcept -> Bloom_params
   {
      return _color_grading_regions_blender.global_bloom_params();
   }

   void vignette_params(const Vignette_params& params) noexcept
   {
      _vignette_params = params;

      _global_constants.vignette_end = _vignette_params.end;
      _global_constants.vignette_start = _vignette_params.start;

      _vignette_enabled = user_config.effects.vignette && _vignette_params.enabled;

      _config_changed = true;
   }

   auto vignette_params() const noexcept -> const Vignette_params&
   {
      return _vignette_params;
   }

   void color_grading_params(const Color_grading_params& params) noexcept
   {
      _color_grading_regions_blender.global_cg_params(params);

      _config_changed = true;
   }

   auto color_grading_params() const noexcept -> Color_grading_params
   {
      return _color_grading_regions_blender.global_cg_params();
   }

   void film_grain_params(const Film_grain_params& params) noexcept
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

   auto film_grain_params() const noexcept -> const Film_grain_params&
   {
      return _film_grain_params;
   }

   void color_grading_regions(const Color_grading_regions& colorgrading_regions) noexcept
   {
      _color_grading_regions_blender.regions(colorgrading_regions);

      _config_changed = true;
   }

   void show_color_grading_regions_imgui(
      const HWND game_window,
      Small_function<Color_grading_params(Color_grading_params) noexcept> show_cg_params_imgui,
      Small_function<Bloom_params(Bloom_params) noexcept> show_bloom_params_imgui) noexcept
   {
      _color_grading_regions_blender.show_imgui(game_window,
                                                std::move(show_cg_params_imgui),
                                                std::move(show_bloom_params_imgui));
   }

   void apply(ID3D11DeviceContext1& dc, Rendertarget_allocator& rt_allocator,
              Profiler& profiler, const core::Shader_resource_database& textures,
              const glm::vec3 camera_position, FFX_cas& ffx_cas, CMAA2& cmaa2,
              const Postprocess_input input, const Postprocess_output output,
              const Postprocess_options options) noexcept
   {
      const auto process_format = _hdr_state == Hdr_state::hdr
                                     ? DXGI_FORMAT_R16G16B16A16_FLOAT
                                     : DXGI_FORMAT_R11G11B10_FLOAT;
      const bool stock_hdr = _hdr_state == Hdr_state::stock;
      const bool resolve_msaa = input.sample_count != 1;
      const bool cas_enabled = ffx_cas.params().enabled;
      const bool cmaa2_enabled = options.apply_cmaa2;
      const bool output_luma = cmaa2_enabled;
      const bool compute_stages_active = cas_enabled || cmaa2_enabled;

      // State setup.
      bind_common_state(dc);

      update_colorgrading_bloom(dc, camera_position);
      update_shaders(output_luma);
      update_and_bind_cb(dc, input.width, input.height);

      // Postprocessing!
      Work_texture work_texture = input;

      if (resolve_msaa) {
         work_texture =
            do_msaa_resolve_input(dc, work_texture, input.sample_count,
                                  rt_allocator, process_format, profiler);
      }

      if (stock_hdr && !resolve_msaa) {
         work_texture = do_linearize_input(dc, work_texture, rt_allocator,
                                           process_format, profiler);
      }

      std::optional luma_target =
         output_luma
            ? rt_allocator.allocate({.format = DXGI_FORMAT_R8_UNORM,
                                     .width = input.width,
                                     .height = input.height,
                                     .bind_flags = rendertarget_bind_srv_rtv})
            : std::optional<Rendertarget_allocator::Handle>{};

      if (_bloom_enabled) {
         work_texture =
            do_bloom_and_color_grading(dc, rt_allocator, textures, work_texture,
                                       process_format, compute_stages_active, profiler,
                                       output_luma ? luma_target->rtv() : nullptr);
      }
      else {
         work_texture = do_color_grading(dc, rt_allocator, textures, work_texture,
                                         compute_stages_active, profiler,
                                         output_luma ? luma_target->rtv() : nullptr);
      }

      if (cas_enabled) {
         work_texture = do_cas(dc, ffx_cas, work_texture, rt_allocator, profiler);
      }

      if (cmaa2_enabled) {
         do_cmaa2(dc, cmaa2, work_texture, *luma_target->srv(),
                  profiler); // CMAA2 operators inplace, no need to change work_texture.
      }

      // Copy the anti-aliased scene into the backbuffer, apply dithering and film grain.
      do_finalize(dc, work_texture, output, profiler);
   }

   void hdr_state(Hdr_state state) noexcept
   {
      _hdr_state = state;

      _config_changed = true;
   }

private:
   void bind_common_state(ID3D11DeviceContext1& dc) noexcept
   {
      dc.ClearState();
      dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      dc.VSSetShader(_fullscreen_vs.get(), nullptr, 0);

      auto* const sampler = _linear_clamp_sampler.get();
      dc.PSSetSamplers(0, 1, &sampler);
   }

   auto do_msaa_resolve_input(ID3D11DeviceContext1& dc, const Work_texture& input,
                              const UINT input_sample_count,
                              Rendertarget_allocator& rt_allocator,
                              const DXGI_FORMAT process_format,
                              Profiler& profiler) noexcept -> Work_texture
   {
      Expects(input_sample_count > 1);

      Profile profile{profiler, dc, "Postprocessing - MSAA Resolve"};

      // Update constants buffer.
      const Resolve_constants constants{{input.width() - 1, input.height() - 1},
                                        _global_constants.exposure};

      core::update_dynamic_buffer(dc, *_msaa_resolve_constant_buffer, constants);

      auto output = rt_allocator.allocate(
         {.format = process_format,
          .width = input.width(),
          .height = input.height(),
          .bind_flags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET});

      auto* const srv = input.srv();
      auto* const rtv = output.rtv();
      auto* const shader = select_msaa_resolve_shader(input_sample_count);

      dc.OMSetBlendState(nullptr, nullptr, 0xffffffff);
      set_viewport(dc, input.width(), input.height());

      dc.PSSetShaderResources(0, 1, &srv);
      dc.PSSetShader(shader, nullptr, 0);
      dc.OMSetRenderTargets(1, &rtv, nullptr);

      dc.Draw(3, 0);

      return std::move(output);
   };

   auto do_linearize_input(ID3D11DeviceContext1& dc, const Work_texture& input,
                           Rendertarget_allocator& rt_allocator,
                           const DXGI_FORMAT process_format,
                           Profiler& profiler) noexcept -> Work_texture
   {
      Profile profile{profiler, dc, "Postprocessing - Linearize Input"};

      auto output = rt_allocator.allocate(
         {.format = process_format,
          .width = input.width(),
          .height = input.height(),
          .bind_flags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET});

      auto* const srv = input.srv();
      auto* const rtv = output.rtv();

      set_viewport(dc, input.width(), input.height());

      dc.PSSetShaderResources(0, 1, &srv);
      dc.PSSetShader(_stock_hdr_to_linear_ps.get(), nullptr, 0);
      dc.OMSetRenderTargets(1, &rtv, nullptr);

      dc.Draw(3, 0);

      return std::move(output);
   }

   auto do_bloom_and_color_grading(
      ID3D11DeviceContext1& dc, Rendertarget_allocator& allocator,
      const core::Shader_resource_database& textures, const Work_texture& input,
      const DXGI_FORMAT process_format, const bool uav_bindable_output,
      Profiler& profiler, ID3D11RenderTargetView* luma_rtv = nullptr) noexcept -> Work_texture
   {
      auto rt_a = allocator.allocate({.format = process_format,
                                      .width = input.width() / 2,
                                      .height = input.height() / 2,
                                      .bind_flags = rendertarget_bind_srv_rtv});
      auto rt_b = allocator.allocate({.format = process_format,
                                      .width = input.width() / 4,
                                      .height = input.height() / 4,
                                      .bind_flags = rendertarget_bind_srv_rtv});
      auto rt_c = allocator.allocate({.format = process_format,
                                      .width = input.width() / 8,
                                      .height = input.height() / 8,
                                      .bind_flags = rendertarget_bind_srv_rtv});
      auto rt_d = allocator.allocate({.format = process_format,
                                      .width = input.width() / 16,
                                      .height = input.height() / 16,
                                      .bind_flags = rendertarget_bind_srv_rtv});
      auto rt_e = allocator.allocate({.format = process_format,
                                      .width = input.width() / 32,
                                      .height = input.height() / 32,
                                      .bind_flags = rendertarget_bind_srv_rtv});

      // Bloom Threshold
      {
         Profile profile{profiler, dc, "Postprocessing - Bloom Threshold"};

         dc.PSSetShader(_bloom_threshold_ps.get(), nullptr, 0);

         bind_bloom_cb(dc, 0);
         set_viewport(dc, input.width() / 2, input.height() / 2);
         do_pass(dc, *input.srv(), *rt_a.rtv());
      }

      // Bloom Downsample
      {
         Profile profile{profiler, dc, "Postprocessing - Bloom Downsample"};

         dc.PSSetShader(_bloom_downsample_ps.get(), nullptr, 0);

         bind_bloom_cb(dc, 1);
         set_viewport(dc, input.width() / 4, input.height() / 4);
         do_pass(dc, *rt_a.srv(), *rt_b.rtv());

         bind_bloom_cb(dc, 2);
         set_viewport(dc, input.width() / 8, input.height() / 8);
         do_pass(dc, *rt_b.srv(), *rt_c.rtv());

         bind_bloom_cb(dc, 3);
         set_viewport(dc, input.width() / 16, input.height() / 16);
         do_pass(dc, *rt_c.srv(), *rt_d.rtv());

         bind_bloom_cb(dc, 4);
         set_viewport(dc, input.width() / 32, input.height() / 32);
         do_pass(dc, *rt_d.srv(), *rt_e.rtv());
      }

      // Bloom Upsample
      {
         Profile profile{profiler, dc, "Postprocessing - Bloom Upsample"};

         dc.PSSetShader(_bloom_upsample_ps.get(), nullptr, 0);
         dc.OMSetBlendState(_additive_blend_state.get(), nullptr, 0xffffffff);

         bind_bloom_cb(dc, 5);
         set_viewport(dc, input.width() / 16, input.height() / 16);
         do_pass(dc, *rt_e.srv(), *rt_d.rtv());

         bind_bloom_cb(dc, 4);
         set_viewport(dc, input.width() / 8, input.height() / 8);
         do_pass(dc, *rt_d.srv(), *rt_c.rtv());

         bind_bloom_cb(dc, 3);
         set_viewport(dc, input.width() / 4, input.height() / 4);
         do_pass(dc, *rt_c.srv(), *rt_b.rtv());

         bind_bloom_cb(dc, 2);
         set_viewport(dc, input.width() / 2, input.height() / 2);
         do_pass(dc, *rt_b.srv(), *rt_a.rtv());
      }

      Profile profile{profiler, dc, "Postprocessing - Combine"};

      std::array<ID3D11ShaderResourceView*, 5> srvs{};

      srvs[scene_texture_slot] = input.srv();
      srvs[bloom_texture_slot] = rt_a.srv();
      srvs[dirt_texture_slot] =
         _bloom_use_dirt ? textures.at_if(_bloom_dirt_texture_name).get() : nullptr;
      srvs[lut_texture_slot] = _color_grading_lut_baker.srv();
      srvs[blue_noise_texture_slot] = blue_noise_srv(textures);

      dc.PSSetShader(_postprocess_combine_ps.get(), nullptr, 0);
      bind_bloom_cb(dc, 1);
      dc.OMSetBlendState(nullptr, nullptr, 0xffffffff);
      set_viewport(dc, input.width(), input.height());

      auto output = allocator.allocate(
         {.format = uav_bindable_output ? DXGI_FORMAT_R8G8B8A8_TYPELESS
                                        : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
          .width = input.width(),
          .height = input.height(),
          .bind_flags = uav_bindable_output ? rendertarget_bind_srv_rtv_uav
                                            : rendertarget_bind_srv_rtv,
          .srv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
          .rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
          .uav_format = DXGI_FORMAT_R8G8B8A8_UNORM});

      if (luma_rtv)
         do_pass(dc, srvs, {output.rtv(), luma_rtv});
      else
         do_pass(dc, srvs, *output.rtv());

      return std::move(output);
   }

   auto do_color_grading(ID3D11DeviceContext1& dc, Rendertarget_allocator& allocator,
                         const core::Shader_resource_database& textures,
                         const Work_texture& input,
                         const bool uav_bindable_output, Profiler& profiler,
                         ID3D11RenderTargetView* luma_rtv = nullptr) noexcept -> Work_texture
   {
      Profile profile{profiler, dc, "Postprocessing - Combine"};

      std::array<ID3D11ShaderResourceView*, 5> srvs{};
      set_viewport(dc, input.width(), input.height());

      srvs[scene_texture_slot] = input.srv();
      srvs[lut_texture_slot] = _color_grading_lut_baker.srv();
      srvs[blue_noise_texture_slot] = blue_noise_srv(textures);

      dc.PSSetShader(_postprocess_combine_ps.get(), nullptr, 0);

      auto output = allocator.allocate(
         {.format = uav_bindable_output ? DXGI_FORMAT_R8G8B8A8_TYPELESS
                                        : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
          .width = input.width(),
          .height = input.height(),
          .bind_flags = uav_bindable_output ? rendertarget_bind_srv_rtv_uav
                                            : rendertarget_bind_srv_rtv,
          .srv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
          .rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
          .uav_format = DXGI_FORMAT_R8G8B8A8_UNORM});

      if (luma_rtv)
         do_pass(dc, srvs, {output.rtv(), luma_rtv});
      else
         do_pass(dc, srvs, *output.rtv());

      return std::move(output);
   }

   auto do_cas(ID3D11DeviceContext1& dc, FFX_cas& ffx_cas,
               const Work_texture& input, Rendertarget_allocator& rt_allocator,
               Profiler& profiler) noexcept -> Work_texture
   {
      auto output = rt_allocator.allocate(
         {.format = DXGI_FORMAT_R8G8B8A8_TYPELESS,
          .width = input.width(),
          .height = input.height(),
          .bind_flags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET |
                        D3D11_BIND_UNORDERED_ACCESS,
          .srv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
          .rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
          .uav_format = DXGI_FORMAT_R8G8B8A8_UNORM});

      ffx_cas.apply(dc, profiler,
                    {.width = input.width(),
                     .height = input.height(),
                     .input = *input.srv(),
                     .output = *output.uav()});

      return std::move(output);
   }

   void do_cmaa2(ID3D11DeviceContext1& dc, CMAA2& cmaa2, const Work_texture& input_output,
                 ID3D11ShaderResourceView& luma_input, Profiler& profiler) noexcept
   {
      cmaa2.apply(dc, profiler,
                  {.input = *input_output.srv(),
                   .output = *input_output.uav(),
                   .luma_srv = &luma_input,
                   .width = input_output.width(),
                   .height = input_output.height()});
   }

   void do_finalize(ID3D11DeviceContext1& dc, const Work_texture& input,
                    const Postprocess_output output, Profiler& profiler) noexcept
   {
      Profile profile{profiler, dc, "Postprocessing - Finalize Output"};

      dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      dc.VSSetShader(_fullscreen_vs.get(), nullptr, 0);

      auto* const sampler = _linear_clamp_sampler.get();
      auto* const cb = _global_constant_buffer.get();
      dc.PSSetSamplers(0, 1, &sampler);
      dc.PSSetConstantBuffers(global_cb_slot, 1, &cb);
      dc.PSSetShader(_postprocess_finalize_ps.get(), nullptr, 0);

      set_viewport(dc, output.width, output.height);
      do_pass(dc, *input.srv(), output.rtv);
   }

   auto select_msaa_resolve_shader(const UINT sample_count) const noexcept
      -> ID3D11PixelShader*
   {
      Expects(sample_count > 1);

      if (_hdr_state == Hdr_state::hdr) {
         if (sample_count == 2) return _msaa_hdr_resolve_x2_ps.get();
         if (sample_count == 4) return _msaa_hdr_resolve_x4_ps.get();
         if (sample_count == 8) return _msaa_hdr_resolve_x8_ps.get();
      }
      else {
         if (sample_count == 2) return _msaa_stock_hdr_resolve_x2_ps.get();
         if (sample_count == 4) return _msaa_stock_hdr_resolve_x4_ps.get();
         if (sample_count == 8) return _msaa_stock_hdr_resolve_x8_ps.get();
      }

      std::terminate();
   }

   void update_and_bind_cb(ID3D11DeviceContext1& dc, const UINT width,
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

   void bind_bloom_cb(ID3D11DeviceContext1& dc, const UINT index) noexcept
   {
      auto* const cb = _bloom_constant_buffers[index].get();

      dc.PSSetConstantBuffers(bloom_cb_slot, 1, &cb);
   }

   auto blue_noise_srv(const core::Shader_resource_database& textures) noexcept
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

   void update_colorgrading_bloom(ID3D11DeviceContext1& dc,
                                  const glm::vec3 camera_position) noexcept
   {
      const auto [cg_params, bloom_params] =
         _color_grading_regions_blender.blend(camera_position);

      _global_constants.exposure = glm::exp2(cg_params.exposure) * cg_params.brightness;
      _global_constants.bloom_threshold = bloom_params.threshold;
      _global_constants.bloom_global_scale = bloom_params.tint * bloom_params.intensity;
      _global_constants.bloom_dirt_scale =
         bloom_params.dirt_scale * bloom_params.dirt_tint;

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

   void update_randomness() noexcept
   {
      _global_constants.randomness_flt = {_random_real_dist(_random_engine),
                                          _random_real_dist(_random_engine),
                                          _random_real_dist(_random_engine),
                                          _random_real_dist(_random_engine)};

      _global_constants.randomness_uint = {_random_engine(), _random_engine(),
                                           _random_engine(), _random_engine()};
   }

   void update_shaders(const bool output_luma) noexcept
   {
      if (!std::exchange(_config_changed, false)) return;

      Postprocess_combine_flags postprocess_combine_flags{};

      if (_bloom_enabled) {
         postprocess_combine_flags |= Postprocess_combine_flags::bloom_active;

         if (_bloom_use_dirt)
            postprocess_combine_flags |= Postprocess_combine_flags::bloom_use_dirt;
      }

      if (output_luma)
         postprocess_combine_flags |= Postprocess_combine_flags::output_luma;

      Postprocess_finalize_flags postprocess_finalize_flags{};

      if (_film_grain_enabled) {
         postprocess_finalize_flags |= Postprocess_finalize_flags::film_grain_active;

         if (_colored_film_grain_enabled) {
            postprocess_finalize_flags |= Postprocess_finalize_flags::film_grain_colored;
         }
      }

      if (_vignette_enabled)
         postprocess_finalize_flags |= Postprocess_finalize_flags::vignette_active;

      _postprocess_combine_ps =
         _shaders.entrypoint("main_combine_ps"sv, postprocess_combine_flags);
      _postprocess_finalize_ps =
         _shaders.entrypoint("main_finalize_ps"sv, postprocess_finalize_flags);
   }

   struct Resolve_constants {
      glm::ivec2 input_range;
      float exposure;
      float _padding{};
   };

   static_assert(sizeof(Resolve_constants) == 16);

   struct Global_constants {
      glm::vec2 scene_pixel_size;

      float vignette_end;
      float vignette_start;

      glm::vec3 bloom_global_scale;
      float bloom_threshold;
      glm::vec3 bloom_dirt_scale;

      float exposure;

      float film_grain_amount;
      float film_grain_aspect;
      float film_grain_color_amount;
      float film_grain_luma_amount;

      glm::vec2 film_grain_size;
      std::array<float, 2> _padding0;

      glm::vec4 randomness_flt;
      glm::uvec4 randomness_uint;
   };

   static_assert(sizeof(Global_constants) == 112);

   struct Bloom_constants {
      glm::vec3 bloom_local_scale;
      float _padding0{};
      glm::vec2 bloom_texel_size;
      std::array<float, 2> _padding1{};
   };

   static_assert(sizeof(Bloom_constants) == 32);

   Global_constants _global_constants;
   std::array<Bloom_constants, 6> _bloom_constants;

   const Com_ptr<ID3D11Device1> _device;
   const Com_ptr<ID3D11BlendState> _additive_blend_state =
      create_additive_blend_state(*_device);
   const Com_ptr<ID3D11SamplerState> _linear_clamp_sampler = [this] {
      const CD3D11_SAMPLER_DESC desc{D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
                                     D3D11_TEXTURE_ADDRESS_CLAMP,
                                     D3D11_TEXTURE_ADDRESS_CLAMP,
                                     D3D11_TEXTURE_ADDRESS_CLAMP,
                                     0.0f,
                                     1,
                                     D3D11_COMPARISON_NEVER,
                                     nullptr,
                                     0.0f,
                                     std::numeric_limits<float>::max()};

      Com_ptr<ID3D11SamplerState> state;

      _device->CreateSamplerState(&desc, state.clear_and_assign());

      return state;
   }();

   const Com_ptr<ID3D11Buffer> _msaa_resolve_constant_buffer =
      core::create_dynamic_constant_buffer(*_device, sizeof(Resolve_constants));
   const Com_ptr<ID3D11Buffer> _global_constant_buffer =
      core::create_dynamic_constant_buffer(*_device, sizeof(Global_constants));
   const std::array<Com_ptr<ID3D11Buffer>, 6> _bloom_constant_buffers{
      core::create_dynamic_constant_buffer(*_device, sizeof(Bloom_constants)),
      core::create_dynamic_constant_buffer(*_device, sizeof(Bloom_constants)),
      core::create_dynamic_constant_buffer(*_device, sizeof(Bloom_constants)),
      core::create_dynamic_constant_buffer(*_device, sizeof(Bloom_constants)),
      core::create_dynamic_constant_buffer(*_device, sizeof(Bloom_constants)),
      core::create_dynamic_constant_buffer(*_device, sizeof(Bloom_constants))};

   shader::Group_pixel& _shaders;

   const Com_ptr<ID3D11VertexShader> _fullscreen_vs;
   const Com_ptr<ID3D11PixelShader> _stock_hdr_to_linear_ps;
   const Com_ptr<ID3D11PixelShader> _msaa_hdr_resolve_x2_ps;
   const Com_ptr<ID3D11PixelShader> _msaa_hdr_resolve_x4_ps;
   const Com_ptr<ID3D11PixelShader> _msaa_hdr_resolve_x8_ps;
   const Com_ptr<ID3D11PixelShader> _msaa_stock_hdr_resolve_x2_ps;
   const Com_ptr<ID3D11PixelShader> _msaa_stock_hdr_resolve_x4_ps;
   const Com_ptr<ID3D11PixelShader> _msaa_stock_hdr_resolve_x8_ps;
   const Com_ptr<ID3D11PixelShader> _bloom_downsample_ps;
   const Com_ptr<ID3D11PixelShader> _bloom_upsample_ps;
   const Com_ptr<ID3D11PixelShader> _bloom_threshold_ps;
   Com_ptr<ID3D11PixelShader> _postprocess_combine_ps;
   Com_ptr<ID3D11PixelShader> _postprocess_finalize_ps;

   Color_grading_lut_baker _color_grading_lut_baker;
   Color_grading_regions_blender _color_grading_regions_blender;

   Vignette_params _vignette_params{};
   Film_grain_params _film_grain_params{};

   Hdr_state _hdr_state = Hdr_state::hdr;

   bool _config_changed = true;
   bool _bloom_enabled = true;
   bool _bloom_use_dirt = false;
   bool _vignette_enabled = true;
   bool _film_grain_enabled = true;
   bool _colored_film_grain_enabled = true;
   bool _mask_nans = false;

   std::mt19937 _random_engine{std::random_device{}()};
   std::uniform_real_distribution<float> _random_real_dist{0.0f, 256.0f};
   std::uniform_int<int> _random_int_dist{0, 63};

   std::vector<Com_ptr<ID3D11ShaderResourceView>> _blue_noise_srvs;

   std::string _bloom_dirt_texture_name;

   constexpr static auto global_cb_slot = 0;
   constexpr static auto bloom_cb_slot = 1;

   constexpr static auto scene_texture_slot = 0;
   constexpr static auto bloom_texture_slot = 1;
   constexpr static auto dirt_texture_slot = 2;
   constexpr static auto lut_texture_slot = 3;
   constexpr static auto blue_noise_texture_slot = 4;
};

Postprocess::Postprocess(Com_ptr<ID3D11Device5> device, shader::Database& shaders)
{
   // The new here is required to access the private constructor of Impl.
   // This is about the only exception to the rule of using std::make_unique in the codebase.
   _impl = std::unique_ptr<Impl>{new Impl{device, shaders}};
}

Postprocess::~Postprocess() = default;

void Postprocess::bloom_params(const Bloom_params& params) noexcept
{
   _impl->bloom_params(params);
}

auto Postprocess::bloom_params() const noexcept -> Bloom_params
{
   return _impl->bloom_params();
}

void Postprocess::vignette_params(const Vignette_params& params) noexcept
{
   _impl->vignette_params(params);
}

auto Postprocess::vignette_params() const noexcept -> const Vignette_params&
{
   return _impl->vignette_params();
}

void Postprocess::color_grading_params(const Color_grading_params& params) noexcept
{
   _impl->color_grading_params(params);
}

auto Postprocess::color_grading_params() const noexcept -> Color_grading_params
{
   return _impl->color_grading_params();
}

void Postprocess::film_grain_params(const Film_grain_params& params) noexcept
{
   _impl->film_grain_params(params);
}

auto Postprocess::film_grain_params() const noexcept -> const Film_grain_params&
{
   return _impl->film_grain_params();
}

void Postprocess::color_grading_regions(const Color_grading_regions& colorgrading_regions) noexcept
{
   _impl->color_grading_regions(colorgrading_regions);
}

void Postprocess::show_color_grading_regions_imgui(
   const HWND game_window,
   Small_function<Color_grading_params(Color_grading_params) noexcept> show_cg_params_imgui,
   Small_function<Bloom_params(Bloom_params) noexcept> show_bloom_params_imgui) noexcept
{
   _impl->show_color_grading_regions_imgui(game_window,
                                           std::move(show_cg_params_imgui),
                                           std::move(show_bloom_params_imgui));
}

void Postprocess::apply(ID3D11DeviceContext1& dc,
                        Rendertarget_allocator& rt_allocator, Profiler& profiler,
                        const core::Shader_resource_database& textures,
                        const glm::vec3 camera_position, FFX_cas& ffx_cas,
                        CMAA2& cmaa2, const Postprocess_input input,
                        const Postprocess_output output,
                        const Postprocess_options options) noexcept
{
   _impl->apply(dc, rt_allocator, profiler, textures, camera_position, ffx_cas,
                cmaa2, input, output, options);
}

void Postprocess::hdr_state(Hdr_state state) noexcept
{
   _impl->hdr_state(state);
}

}
