
#include "scene_blur.hpp"
#include "helpers.hpp"

namespace sp::effects {

Scene_blur::Scene_blur(Com_ref<IDirect3DDevice9> device) : _device{device} {}

void Scene_blur::compute(const Shader_rendertype& rendertype,
                         Rendertarget_allocator& allocator,
                         IDirect3DSurface9& from, IDirect3DTexture9& to) noexcept
{
   const auto [format, resolution] = effects::get_surface_metrics(from);
   auto resolve = allocator.allocate(format, resolution / 2);

   Com_ptr<IDirect3DSurface9> to_surface;
   to.GetSurfaceLevel(0, to_surface.clear_and_assign());

   _device->StretchRect(&from, nullptr, resolve.surface(), nullptr, D3DTEXF_LINEAR);
   _device->StretchRect(resolve.surface(), nullptr, to_surface.get(), nullptr,
                        D3DTEXF_LINEAR);

   setup_vetrex_stream();
   setup_state(format);

   auto rt_a_x = allocator.allocate(format, resolution / 4);

   rendertype.at("blur 6"s).bind(*_device);

   do_pass(*rt_a_x.surface(), to, {1.f, 0.f});
   do_pass(*to_surface.get(), *rt_a_x.texture(), {0.f, 1.f});
}

void Scene_blur::apply(const Shader_database& shaders, Rendertarget_allocator& allocator,
                       IDirect3DSurface9& from_to, float alpha) noexcept
{
   const auto [format, resolution] = effects::get_surface_metrics(from_to);
   auto blur_target = allocator.allocate(format, resolution / 4);

   compute(shaders.rendertypes.at("gaussian blur"s), allocator, from_to,
           *blur_target.texture());

   set_fs_pass_alpha_blend_state(*_device);
   _device->SetTexture(0, blur_target.texture());
   _device->SetRenderTarget(0, &from_to);
   set_fs_pass_state(*_device, from_to);

   shaders.rendertypes.at("scene blur apply"s).at("main"s).bind(*_device);

   direct3d::Ps_1f_shader_constant<constants::ps::post_processing_start>{}.set(*_device,
                                                                               alpha);

   _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
}

void Scene_blur::do_pass(IDirect3DSurface9& dest, IDirect3DTexture9& source,
                         glm::vec2 direction) const noexcept
{
   _device->SetTexture(0, &source);
   _device->SetRenderTarget(0, &dest);

   set_fs_pass_state(*_device, dest);
   set_blur_pass_state(*_device, source, direction);

   _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
}

void Scene_blur::drop_device_resources() noexcept
{
   _vertex_decl.reset(nullptr);
   _vertex_buffer.reset(nullptr);
}

void Scene_blur::setup_state(D3DFORMAT format) noexcept
{
   const bool srgb = format == D3DFMT_A8R8G8B8;

   _device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   _device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   _device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
   _device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
   _device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   _device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, srgb);

   _device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
   _device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
   _device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
   _device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
   _device->SetRenderState(D3DRS_SRGBWRITEENABLE, srgb);
}

void Scene_blur::setup_vetrex_stream() noexcept
{
   if (!_vertex_decl || !_vertex_buffer) create_resources();

   _device->SetVertexDeclaration(_vertex_decl.get());
   _device->SetStreamSource(0, _vertex_buffer.get(), 0, fs_triangle_stride);
}

void Scene_blur::create_resources() noexcept
{
   _vertex_decl = create_fs_triangle_decl(*_device);
   _vertex_buffer = create_fs_triangle_buffer(*_device);
}

}
