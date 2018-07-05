
#include "scene_blur.hpp"
#include "helpers.hpp"

namespace sp::effects {

Scene_blur::Scene_blur(Com_ref<IDirect3DDevice9> device) : _device{device} {}

void Scene_blur::apply(const Shader_database& shaders, Rendertarget_allocator& allocator,
                       IDirect3DTexture9& from_to) noexcept
{
   auto [format, res] = get_texture_metrics(from_to);

   setup_vetrex_stream();
   setup_state(format);

   auto rt_a_x = allocator.allocate(format, res);

   Com_ptr<IDirect3DSurface9> to;
   from_to.GetSurfaceLevel(0, to.clear_and_assign());

   shaders.at("gaussian blur"s).at("blur 13"s).bind(*_device);

   do_pass(*rt_a_x.surface(), from_to, {1.f, 0.f});
   do_pass(*to.get(), *rt_a_x.texture(), {0.f, 1.f});
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
