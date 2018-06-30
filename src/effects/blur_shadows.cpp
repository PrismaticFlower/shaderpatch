
#include "blur_shadows.hpp"
#include "../direct3d/shader_constant.hpp"
#include "../shader_constants.hpp"
#include "helpers.hpp"

#include "imgui.h"

using namespace std::literals;

namespace sp::effects {

Blur_shadows::Blur_shadows(Com_ref<IDirect3DDevice9> device) : _device{device}
{
}

void Blur_shadows::apply(const Shader_group& shaders, Rendertarget_allocator& allocator,
                         IDirect3DTexture9& depth, IDirect3DTexture9& from_to) noexcept
{
   auto [format, res] = get_texture_metrics(from_to);

   setup_samplers();
   setup_vetrex_stream();
   set_rt_metrics_state(res);
   shaders.at("blur 7"s).bind(*_device);
   _device->SetTexture(5, &depth);

   auto x_target = allocator.allocate(format, res);

   set_direction_state(res, {1, 0});
   do_pass(*x_target.surface(), from_to);

   Com_ptr<IDirect3DSurface9> y_target;
   from_to.GetSurfaceLevel(0, y_target.clear_and_assign());

   set_direction_state(res, {0, 1});
   do_pass(*y_target, *x_target.texture());
}

void Blur_shadows::do_pass(IDirect3DSurface9& dest, IDirect3DTexture9& source) const noexcept
{
   _device->SetTexture(4, &source);
   _device->SetRenderTarget(0, &dest);

   _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
}

void Blur_shadows::set_rt_metrics_state(glm::ivec2 resolution) const noexcept
{
   direct3d::Vs_2f_shader_constant<constants::vs::post_processing_start>{}
      .set(*_device, {-1.0f / resolution.x, 1.0f / resolution.y});

   D3DVIEWPORT9 viewport{0,
                         0,
                         static_cast<DWORD>(resolution.x),
                         static_cast<DWORD>(resolution.y),
                         0.0f,
                         1.0f};

   _device->SetViewport(&viewport);
}

void Blur_shadows::set_direction_state(glm::ivec2 resolution,
                                       glm::ivec2 direction) const noexcept
{
   direct3d::Vs_2f_shader_constant<constants::vs::post_processing_start>{}
      .set(*_device,
           direction * glm::ivec2{1.0f / resolution.x, 1.0f / resolution.y});
}

void Blur_shadows::drop_device_resources() noexcept
{
   _vertex_decl.reset(nullptr);
   _vertex_buffer.reset(nullptr);
}

void Blur_shadows::setup_samplers() const noexcept
{
   _device->SetSamplerState(4, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   _device->SetSamplerState(4, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   _device->SetSamplerState(4, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
   _device->SetSamplerState(4, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
   _device->SetSamplerState(4, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   _device->SetSamplerState(4, D3DSAMP_SRGBTEXTURE, false);

   _device->SetSamplerState(5, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   _device->SetSamplerState(5, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   _device->SetSamplerState(5, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   _device->SetSamplerState(5, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
   _device->SetSamplerState(5, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   _device->SetSamplerState(5, D3DSAMP_SRGBTEXTURE, false);
}

void Blur_shadows::setup_vetrex_stream() noexcept
{
   if (!_vertex_decl || !_vertex_buffer) create_resources();

   _device->SetVertexDeclaration(_vertex_decl.get());
   _device->SetStreamSource(0, _vertex_buffer.get(), 0, fs_triangle_stride);
}

void Blur_shadows::create_resources() noexcept
{
   _vertex_decl = create_fs_triangle_decl(*_device);
   _vertex_buffer = create_fs_triangle_buffer(*_device);
}
}
