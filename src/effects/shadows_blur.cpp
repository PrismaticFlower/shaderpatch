
#include "shadows_blur.hpp"
#include "../direct3d/shader_constant.hpp"
#include "../shader_constants.hpp"
#include "helpers.hpp"

#include "imgui.h"

using namespace std::literals;

namespace sp::effects {

Shadows_blur::Shadows_blur(Com_ref<IDirect3DDevice9> device) : _device{device}
{
}

void Shadows_blur::apply(const Shader_rendertype& rendertype,
                         Rendertarget_allocator& allocator,
                         IDirect3DTexture9& depth, IDirect3DTexture9& from_to) noexcept
{
   auto [format, res] = get_texture_metrics(from_to);

   setup_samplers();
   setup_vetrex_stream();
   set_fs_pass_blend_state(*_device);
   set_ps_constants(res);

   _device->SetTexture(5, &depth);

   Com_ptr<IDirect3DSurface9> from_to_surface;
   from_to.GetSurfaceLevel(0, from_to_surface.clear_and_assign());

   rendertype.at("combine depth"s).bind(*_device);

   auto combined = allocator.allocate(D3DFMT_G16R16F, res);
   do_pass(*combined.surface(), from_to);

   rendertype.at("blur"s).bind(*_device);

   do_pass(*from_to_surface.get(), *combined.texture());
}

void Shadows_blur::do_pass(IDirect3DSurface9& dest, IDirect3DTexture9& source) const noexcept
{
   _device->SetTexture(4, &source);
   _device->SetRenderTarget(0, &dest);
   set_fs_pass_state(*_device, dest);

   _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
}

void Shadows_blur::set_ps_constants(glm::ivec2 resolution) const noexcept
{
   direct3d::Ps_2f_shader_constant<constants::ps::post_processing_start>{}
      .set(*_device, {1.0f / glm::vec2{resolution} * _params.radius_scale});
}

void Shadows_blur::drop_device_resources() noexcept
{
   _vertex_decl.reset(nullptr);
   _vertex_buffer.reset(nullptr);
}

void Shadows_blur::setup_samplers() const noexcept
{
   set_point_clamp_sampler(*_device, 4);
   set_point_clamp_sampler(*_device, 5);
}

void Shadows_blur::setup_vetrex_stream() noexcept
{
   if (!_vertex_decl || !_vertex_buffer) create_resources();

   _device->SetVertexDeclaration(_vertex_decl.get());
   _device->SetStreamSource(0, _vertex_buffer.get(), 0, fs_triangle_stride);
}

void Shadows_blur::create_resources() noexcept
{
   _vertex_decl = create_fs_triangle_decl(*_device);
   _vertex_buffer = create_fs_triangle_buffer(*_device);
}
}
