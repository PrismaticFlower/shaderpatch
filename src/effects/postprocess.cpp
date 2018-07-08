
#include "postprocess.hpp"
#include "color_grading_lut_baker.hpp"
#include "helpers.hpp"

#include <random>

namespace sp::effects {

Postprocess::Postprocess(Com_ref<IDirect3DDevice9> device, Post_aa_quality aa_quality)
   : _device{device}, _vertex_decl{create_fs_triangle_decl(*_device)}
{
   bloom_params(Bloom_params{});
   this->aa_quality(aa_quality);
}

void Postprocess::bloom_params(const Bloom_params& params) noexcept
{
   _bloom_params = params;

   auto& bloom = _constants.bloom;

   bloom.threshold = params.threshold;
   bloom.global = params.tint * params.intensity;
   bloom.inner = params.inner_tint * params.inner_scale;
   bloom.inner_mid = params.inner_mid_scale * params.inner_mid_tint;
   bloom.mid = params.mid_scale * params.mid_tint;
   bloom.outer_mid = params.outer_mid_scale * params.outer_mid_tint;
   bloom.outer = params.outer_scale * params.outer_tint;
   bloom.dirt = params.dirt_scale * params.dirt_tint;
}

void Postprocess::color_grading_params(const Color_grading_params& params) noexcept
{
   _color_grading_params = params;

   _constants.color_grading.color_filter_exposure =
      get_lut_exposure_color_filter(_color_grading_params);
   _constants.color_grading.saturation = params.saturation;
   _color_luts = std::nullopt;
}

void Postprocess::drop_device_resources() noexcept
{
   _vertex_buffer = nullptr;
   _color_luts = std::nullopt;
}

void Postprocess::hdr_state(Hdr_state state) noexcept
{
   if (state == Hdr_state::hdr) {
      _hdr_suffix = ""s;
   }
   else {
      _hdr_suffix = " stock hdr"s;
   }

   _hdr_state = state;
}

void Postprocess::aa_quality(Post_aa_quality quality) noexcept
{
   _aa_suffix = to_string(quality);
}

void Postprocess::apply(const Shader_database& shaders, Rendertarget_allocator& allocator,
                        const Texture_database& textures,
                        IDirect3DTexture9& input, IDirect3DSurface9& output) noexcept
{
   set_fs_pass_blend_state(*_device);
   set_shader_constants();
   setup_vetrex_stream();

   auto [format, res] = get_surface_metrics(output);

   auto scratch_rt = allocator.allocate(format, res);

   if (_bloom_params.enabled) {
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
   for (int i = 0; i < 6; ++i) {
      set_linear_clamp_sampler(*_device, i, _hdr_state != Hdr_state::hdr);
   }

   _device->SetRenderState(D3DRS_SRGBWRITEENABLE, _hdr_state != Hdr_state::hdr);
   _device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, false);

   auto [format, res] = get_texture_metrics(input);

   auto& post_shaders = shaders.at("postprocess"s);
   auto& blurs = shaders.at("gaussian blur"s);

   auto rt_a_x = allocator.allocate(format, res / 2);
   auto rt_a_y = allocator.allocate(format, res / 2);

   post_shaders.at("bloom threshold"s + _hdr_suffix).bind(*_device);

   do_pass(input, *rt_a_y.surface());

   _device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, _hdr_state != Hdr_state::hdr);

   blurs.at("blur 12"s).bind(*_device);

   do_blur_pass(*rt_a_y.texture(), *rt_a_x.surface(), {1.f, 0.f});
   do_blur_pass(*rt_a_x.texture(), *rt_a_y.surface(), {0.f, 1.f});

   auto rt_b_x = allocator.allocate(format, res / 4);
   auto rt_b_y = allocator.allocate(format, res / 4);

   linear_resample(*rt_a_y.surface(), *rt_b_y.surface());
   do_blur_pass(*rt_b_y.texture(), *rt_b_x.surface(), {1.f, 0.f});
   do_blur_pass(*rt_b_x.texture(), *rt_b_y.surface(), {0.f, 1.f});

   auto rt_c_x = allocator.allocate(format, res / 8);
   auto rt_c_y = allocator.allocate(format, res / 8);

   linear_resample(*rt_b_y.surface(), *rt_c_y.surface());
   do_blur_pass(*rt_c_y.texture(), *rt_c_x.surface(), {1.f, 0.f});
   do_blur_pass(*rt_c_x.texture(), *rt_c_y.surface(), {0.f, 1.f});

   auto rt_d_x = allocator.allocate(format, res / 16);
   auto rt_d_y = allocator.allocate(format, res / 16);

   linear_resample(*rt_c_y.surface(), *rt_d_y.surface());
   do_blur_pass(*rt_d_y.texture(), *rt_d_x.surface(), {1.f, 0.f});
   do_blur_pass(*rt_d_x.texture(), *rt_d_y.surface(), {0.f, 1.f});

   auto rt_e_x = allocator.allocate(format, res / 32);
   auto rt_e_y = allocator.allocate(format, res / 32);

   linear_resample(*rt_d_y.surface(), *rt_e_y.surface());
   do_blur_pass(*rt_e_y.texture(), *rt_e_x.surface(), {1.f, 0.f});
   do_blur_pass(*rt_e_x.texture(), *rt_e_y.surface(), {0.f, 1.f});

   _device->SetTexture(bloom_sampler_slots_start, rt_a_y.texture());
   _device->SetTexture(bloom_sampler_slots_start + 1, rt_b_y.texture());
   _device->SetTexture(bloom_sampler_slots_start + 2, rt_c_y.texture());
   _device->SetTexture(bloom_sampler_slots_start + 3, rt_d_y.texture());
   _device->SetTexture(bloom_sampler_slots_start + 4, rt_e_y.texture());
   bind_color_grading_luts();

   _device->SetRenderState(D3DRS_SRGBWRITEENABLE, true);
   _device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, false);

   if (_bloom_params.use_dirt) {
      textures.get(_bloom_params.dirt_texture_name).bind(dirt_sampler_slot_start);
      post_shaders.at("bloom dirt colorgrade"s + _hdr_suffix).bind(*_device);
   }
   else {
      post_shaders.at("bloom colorgrade"s + _hdr_suffix).bind(*_device);
   }

   do_pass(input, output);
}

void Postprocess::do_color_grading(const Shader_database& shaders,
                                   IDirect3DTexture9& input,
                                   IDirect3DSurface9& output) noexcept
{
   set_linear_clamp_sampler(*_device, 0, _hdr_state != Hdr_state::hdr);
   bind_color_grading_luts();
   _device->SetRenderState(D3DRS_SRGBWRITEENABLE, true);

   shaders.at("postprocess"s).at("colorgrade"s + _hdr_suffix).bind(*_device);

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

   shaders.at("postprocess"s).at("finalize "s + _aa_suffix).bind(*_device);

   do_pass(input, output);
}

void Postprocess::do_pass(IDirect3DTexture9& input, IDirect3DSurface9& output) noexcept
{
   _device->SetTexture(0, &input);
   _device->SetRenderTarget(0, &output);

   set_fs_pass_state(*_device, output);

   _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
}

void Postprocess::do_blur_pass(IDirect3DTexture9& input, IDirect3DSurface9& output,
                               glm::vec2 direction) noexcept
{
   _device->SetTexture(0, &input);
   _device->SetRenderTarget(0, &output);

   set_fs_pass_state(*_device, output);
   set_blur_pass_state(*_device, input, direction);

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

   _device->SetPixelShaderConstantF(constants::ps::post_processing_start + 1,
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

void Postprocess::bind_color_grading_luts() noexcept
{
   if (!_color_luts) {
      _color_luts = bake_color_grading_luts(*_device, _color_grading_params);
   }

   for (int i = 0; i < gsl::narrow_cast<int>(_color_luts->size()); ++i) {
      (*_color_luts)[i].bind(lut_sampler_slots_start + i);
   }
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

}
