
#include "postprocess.hpp"
#include "helpers.hpp"

#include <random>

namespace sp::effects {

Postprocess::Postprocess(Com_ref<IDirect3DDevice9> device)
   : _device{device}, _vertex_decl{create_fs_triangle_decl(*_device)}
{
   bloom_params(Bloom_params{});
   vignette_params(Vignette_params{});
   color_grading_params(Color_grading_params{});
   film_grain_params(Film_grain_params{});
   user_config(Effects_user_config{});
}

void Postprocess::bloom_params(const Bloom_params& params) noexcept
{
   _bloom_params = params;

   auto& bloom = _constants.bloom;

   bloom.threshold = params.threshold;
   bloom.global = params.tint * params.intensity;
   bloom.dirt = params.dirt_scale * params.dirt_tint;

   _bloom_inner_scale = params.inner_tint * params.inner_scale;
   _bloom_inner_mid_scale =
      (params.inner_mid_scale * params.inner_mid_tint) / _bloom_inner_scale;
   _bloom_mid_scale = (params.mid_scale * params.mid_tint) / _bloom_inner_mid_scale;
   _bloom_outer_mid_scale =
      params.outer_mid_scale * params.outer_mid_tint / _bloom_mid_scale;
   _bloom_outer_scale = params.outer_scale * params.outer_tint / _bloom_outer_mid_scale;

   _bloom_enabled = _user_config.bloom && _bloom_params.enabled;
}

void Postprocess::vignette_params(const Vignette_params& params) noexcept
{
   _vignette_params = params;

   _constants.vignette.end = _vignette_params.end;
   _constants.vignette.start = _vignette_params.start;

   _vignette_enabled = _user_config.vignette && _vignette_params.enabled;
}

void Postprocess::color_grading_params(const Color_grading_params& params) noexcept
{
   _color_grading_params = params;

   _constants.color_grading.exposure =
      glm::exp2(_color_grading_params.exposure) * _color_grading_params.brightness;

   _color_grading_lut_baker.update_params(params);
}

void Postprocess::film_grain_params(const Film_grain_params& params) noexcept
{
   _film_grain_params = params;

   _constants.film_grain.amount = params.amount;
   _constants.film_grain.size = params.size;
   _constants.film_grain.color_amount = params.color_amount;
   _constants.film_grain.luma_amount = params.luma_amount;

   _film_grain_enabled = _user_config.film_grain && _film_grain_params.enabled;
   _colored_film_grain_enabled =
      _user_config.colored_film_grain && _film_grain_params.colored;
}

void Postprocess::drop_device_resources() noexcept
{
   _vertex_buffer = nullptr;

   _color_grading_lut_baker.drop_device_resources();
}

void Postprocess::hdr_state(Hdr_state state) noexcept
{
   _hdr_state = state;
}

void Postprocess::user_config(const Effects_user_config& config) noexcept
{
   _user_config = config;
   _aa_quality = to_string(config.post_aa_quality);

   _bloom_enabled = _user_config.bloom && _bloom_params.enabled;
   _vignette_enabled = _user_config.vignette && _vignette_params.enabled;
   _film_grain_enabled = _user_config.film_grain && _film_grain_params.enabled;
   _colored_film_grain_enabled =
      _user_config.colored_film_grain && _film_grain_params.colored;
}

void Postprocess::apply(const Shader_database& shaders, Rendertarget_allocator& allocator,
                        const Texture_database& textures,
                        IDirect3DTexture9& input, IDirect3DSurface9& output) noexcept
{
   set_fs_pass_blend_state(*_device);
   set_shader_constants();
   setup_vetrex_stream();
   update_shaders_names();

   auto [format, res] = get_surface_metrics(output);

   auto scratch_rt = allocator.allocate(format, res);

   if (_bloom_enabled) {
      do_bloom_and_color_grading(shaders, allocator, textures, input,
                                 *scratch_rt.surface());
   }
   else {
      do_color_grading(shaders, input, *scratch_rt.surface());
   }

   do_finalize(shaders, textures, *scratch_rt.texture(), output);
}

void Postprocess::do_bloom_and_color_grading(const Shader_database& shaders,
                                             Rendertarget_allocator& allocator,
                                             const Texture_database& textures,
                                             IDirect3DTexture9& input,
                                             IDirect3DSurface9& output) noexcept
{
   auto [format, res] = get_texture_metrics(input);

   set_linear_clamp_sampler(*_device, 0);
   _device->SetRenderState(D3DRS_SRGBWRITEENABLE, format == D3DFMT_A8R8G8B8);
   _device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, false);

   auto& rendertypes = shaders.rendertypes;
   auto& post_shaders = rendertypes.at("postprocess"s);
   auto& bloom_shaders = rendertypes.at("bloom"s);

   auto rt_a = allocator.allocate(format, res / 2);
   auto rt_b = allocator.allocate(format, res / 4);
   auto rt_c = allocator.allocate(format, res / 8);
   auto rt_d = allocator.allocate(format, res / 16);
   auto rt_e = allocator.allocate(format, res / 32);

   bloom_shaders.at(_threshold_shader).bind(*_device);

   _device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, format == D3DFMT_A8R8G8B8);

   do_bloom_pass(input, *rt_a.surface());

   bloom_shaders.at("downsample"s).bind(*_device);

   do_bloom_pass(*rt_a.texture(), *rt_b.surface());
   do_bloom_pass(*rt_b.texture(), *rt_c.surface());
   do_bloom_pass(*rt_c.texture(), *rt_d.surface());
   do_bloom_pass(*rt_d.texture(), *rt_e.surface());

   bloom_shaders.at("upsample"s).bind(*_device);
   set_fs_pass_additive_blend_state(*_device);

   direct3d::Ps_3f_shader_constant<constants::ps::post_processing_start + 10>{}
      .set(*_device, _bloom_outer_scale);
   do_bloom_pass(*rt_e.texture(), *rt_d.surface());

   direct3d::Ps_3f_shader_constant<constants::ps::post_processing_start + 10>{}
      .set(*_device, _bloom_outer_mid_scale);
   do_bloom_pass(*rt_d.texture(), *rt_c.surface());

   direct3d::Ps_3f_shader_constant<constants::ps::post_processing_start + 10>{}
      .set(*_device, _bloom_mid_scale);
   do_bloom_pass(*rt_c.texture(), *rt_b.surface());

   direct3d::Ps_3f_shader_constant<constants::ps::post_processing_start + 10>{}
      .set(*_device, _bloom_inner_mid_scale);
   do_bloom_pass(*rt_b.texture(), *rt_a.surface());

   set_fs_pass_blend_state(*_device);

   _device->SetTexture(bloom_sampler_slot, rt_a.texture());
   _color_grading_lut_baker.bind_texture(lut_sampler_slot);

   _device->SetRenderState(D3DRS_SRGBWRITEENABLE, true);
   _device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, false);
   set_linear_clamp_sampler(*_device, bloom_sampler_slot, format == D3DFMT_A8R8G8B8);

   if (_bloom_params.use_dirt) {
      textures.get(_bloom_params.dirt_texture_name).bind(dirt_sampler_slot);
   }

   post_shaders.at(_uber_shader).bind(*_device);

   const auto input_size = static_cast<glm::vec2>(
      std::get<glm::ivec2>(get_surface_metrics(*rt_a.surface())));
   const auto output_size =
      static_cast<glm::vec2>(std::get<glm::ivec2>(get_surface_metrics(output)));

   direct3d::Ps_2f_shader_constant<constants::ps::post_processing_start + 1>{}
      .set(*_device, {1.0f / input_size});
   direct3d::Ps_3f_shader_constant<constants::ps::post_processing_start + 10>{}
      .set(*_device, _bloom_inner_scale);

   do_pass(input, output);
}

void Postprocess::do_color_grading(const Shader_database& shaders,
                                   IDirect3DTexture9& input,
                                   IDirect3DSurface9& output) noexcept
{
   set_linear_clamp_sampler(*_device, 0);
   _color_grading_lut_baker.bind_texture(lut_sampler_slot);
   _device->SetRenderState(D3DRS_SRGBWRITEENABLE, true);

   shaders.rendertypes.at("postprocess"s).at(_uber_shader).bind(*_device);

   do_pass(input, output);
}

void Postprocess::do_finalize(const Shader_database& shaders,
                              const Texture_database& textures,
                              IDirect3DTexture9& input, IDirect3DSurface9& output)
{
   bind_blue_noise_texture(textures);
   set_fs_pass_ps_state(*_device, input);
   set_linear_clamp_sampler(*_device, 0, false);
   _device->SetRenderState(D3DRS_SRGBWRITEENABLE, false);

   shaders.rendertypes.at("postprocess"s).at(_finalize_shader).bind(*_device);

   do_pass(input, output);
}

void Postprocess::do_pass(IDirect3DTexture9& input, IDirect3DSurface9& output) noexcept
{
   _device->SetTexture(0, &input);
   _device->SetRenderTarget(0, &output);

   set_fs_pass_state(*_device, output);

   _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
}

void Postprocess::do_bloom_pass(IDirect3DTexture9& input, IDirect3DSurface9& output) noexcept
{
   _device->SetTexture(0, &input);
   _device->SetRenderTarget(0, &output);

   set_fs_pass_state(*_device, output);

   const auto input_size =
      static_cast<glm::vec2>(std::get<glm::ivec2>(get_texture_metrics(input)));

   direct3d::Ps_2f_shader_constant<constants::ps::post_processing_start + 1>{}
      .set(*_device, {1.0f / input_size});

   _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
}

void Postprocess::linear_resample(IDirect3DSurface9& input,
                                  IDirect3DSurface9& output) const noexcept
{
   _device->StretchRect(&input, nullptr, &output, nullptr, D3DTEXF_LINEAR);
}

void Postprocess::set_shader_constants() noexcept
{
   update_randomness();

   _device->SetPixelShaderConstantF(constants::ps::post_processing_start + 2,
                                    reinterpret_cast<const float*>(&_constants),
                                    sizeof(_constants) / 16);
}

void Postprocess::update_randomness() noexcept
{
   _constants.randomness = {_random_real_dist(_random_engine),
                            _random_real_dist(_random_engine),
                            _random_real_dist(_random_engine),
                            _random_real_dist(_random_engine)};
}

void Postprocess::bind_blue_noise_texture(const Texture_database& textures) noexcept
{
   auto texture = textures.get("_SP_BUILTIN_blue_noise_rgb_"s +
                               std::to_string(_random_int_dist(_random_engine)));

   texture.bind(blue_noise_sampler_slot);
}

void Postprocess::setup_vetrex_stream() noexcept
{
   if (!_vertex_decl || !_vertex_buffer) create_resources();

   _device->SetVertexDeclaration(_vertex_decl.get());
   _device->SetStreamSource(0, _vertex_buffer.get(), 0, fs_triangle_stride);
}

void Postprocess::create_resources() noexcept
{
   _vertex_buffer = create_fs_triangle_buffer(*_device);
}

void Postprocess::update_shaders_names() noexcept
{
   if (_bloom_enabled) {
      _uber_shader = "bloom"s;

      if (_bloom_params.use_dirt) _uber_shader += " dirt"sv;
      if (_vignette_enabled) _uber_shader += " vignette"sv;

      _uber_shader += " colorgrade"sv;
   }
   else {
      _uber_shader = _vignette_enabled ? "vignette colorgrade"sv : "colorgrade"sv;
   }

   _threshold_shader = "downsample threshold"s;

   if (_hdr_state == Hdr_state::stock) {
      _threshold_shader += " stock hdr"sv;
      _uber_shader += " stock hdr"sv;
   }

   _finalize_shader = "finalize "s + _aa_quality;

   if (_film_grain_enabled) {
      _finalize_shader += " film grain"sv;
      if (_colored_film_grain_enabled) _finalize_shader += " colored"sv;
   }
}
}
