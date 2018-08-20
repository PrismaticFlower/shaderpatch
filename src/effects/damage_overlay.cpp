
#include "damage_overlay.hpp"
#include "helpers.hpp"

namespace sp::effects {

Damage_overlay::Damage_overlay(Com_ref<IDirect3DDevice9> device)
   : _device{device}
{
}

void Damage_overlay::apply(const Shader_database& shaders, glm::vec4 color,
                           IDirect3DTexture9& from, IDirect3DSurface9& over) noexcept
{
   setup_vetrex_stream();
   set_constants(color, from);
   set_fs_pass_alpha_blend_state(*_device);
   set_fs_pass_state(*_device, over);
   set_linear_mirror_sampler(*_device, 0);
   _device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED |
                                                      D3DCOLORWRITEENABLE_GREEN |
                                                      D3DCOLORWRITEENABLE_BLUE);

   shaders.at("damage overlay"s).at("apply"s).bind(*_device);

   _device->SetTexture(0, &from);
   _device->SetRenderTarget(0, &over);

   _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
}

void Damage_overlay::drop_device_resources() noexcept
{
   _vertex_decl.reset(nullptr);
   _vertex_buffer.reset(nullptr);
}

void Damage_overlay::setup_vetrex_stream() noexcept
{
   if (!_vertex_decl || !_vertex_buffer) create_resources();

   _device->SetVertexDeclaration(_vertex_decl.get());
   _device->SetStreamSource(0, _vertex_buffer.get(), 0, fs_triangle_stride);
}

void Damage_overlay::set_constants(glm::vec4 color, IDirect3DTexture9& from) noexcept
{
   Constants constants;

   constants.color = color;

   auto [_, rt_size] = get_texture_metrics(from);

   const auto x_pix_size = 1.0f / static_cast<float>(rt_size.x);

   constants.step_size = x_pix_size * 2.0f;
   constants.sample_count = glm::round(glm::mix(1.0f, 32.0f, color.a));

   _device->SetPixelShaderConstantF(constants::ps::post_processing_start,
                                    glm::value_ptr(constants.color),
                                    sizeof(Constants) / sizeof(glm::vec4));
}

void Damage_overlay::create_resources() noexcept
{
   _vertex_decl = create_fs_triangle_decl(*_device);
   _vertex_buffer = create_fs_triangle_buffer(*_device);
}

}
