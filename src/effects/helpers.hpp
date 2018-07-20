#pragma once

#include "../direct3d/shader_constant.hpp"
#include "../shader_constants.hpp"
#include "com_ptr.hpp"

#include <array>

#include <glm/glm.hpp>

#include <d3d9.h>

namespace sp::effects {

inline auto create_fs_triangle_buffer(IDirect3DDevice9& device, const glm::ivec2 resolution)
   -> Com_ptr<IDirect3DVertexBuffer9>
{
   const auto offset = 1.f / static_cast<glm::vec2>(resolution);

   const auto vertices = std::array<float, 12>{{
      -1.f - offset.x, -1.f + offset.y, // pos 0
      0.f, 1.f,                         // uv 0
      -1.f - offset.x, 3.f + offset.y,  // pos 1
      0.f, -1.f,                        // uv 1
      3.f - offset.x, -1.f + offset.y,  // pos 2
      2.f, 1.f                          // uv 2
   }};

   Com_ptr<IDirect3DVertexBuffer9> buffer;

   device.CreateVertexBuffer(sizeof(vertices), D3DUSAGE_WRITEONLY, 0,
                             D3DPOOL_DEFAULT, buffer.clear_and_assign(), nullptr);

   void* data_ptr{};

   buffer->Lock(0, sizeof(vertices), &data_ptr, 0);

   *static_cast<std::array<float, 12>*>(data_ptr) = vertices;

   buffer->Unlock();

   return buffer;
}

inline auto create_fs_triangle_buffer(IDirect3DDevice9& device)
   -> Com_ptr<IDirect3DVertexBuffer9>
{

   const auto vertices = std::array<float, 12>{{
      -1.f, -1.f, // pos 0
      0.f, 1.f,   // uv 0
      -1.f, 3.f,  // pos 1
      0.f, -1.f,  // uv 1
      3.f, -1.f,  // pos 2
      2.f, 1.f    // uv 2
   }};

   Com_ptr<IDirect3DVertexBuffer9> buffer;

   device.CreateVertexBuffer(sizeof(vertices), D3DUSAGE_WRITEONLY, 0,
                             D3DPOOL_DEFAULT, buffer.clear_and_assign(), nullptr);

   void* data_ptr{};

   buffer->Lock(0, sizeof(vertices), &data_ptr, 0);

   *static_cast<std::array<float, 12>*>(data_ptr) = vertices;

   buffer->Unlock();

   return buffer;
}

inline auto create_fs_triangle_decl(IDirect3DDevice9& device)
   -> Com_ptr<IDirect3DVertexDeclaration9>
{
   constexpr auto declaration = std::array<D3DVERTEXELEMENT9, 3>{
      {{0, 0, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_POSITION, 0},
       {0, 8, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0},
       D3DDECL_END()}};

   Com_ptr<IDirect3DVertexDeclaration9> vertex_decl;

   device.CreateVertexDeclaration(declaration.data(), vertex_decl.clear_and_assign());

   return vertex_decl;
}

inline constexpr int fs_triangle_stride = 16;

inline auto get_texture_metrics(IDirect3DTexture9& from)
   -> std::tuple<D3DFORMAT, glm::ivec2>
{

   D3DSURFACE_DESC desc;
   from.GetLevelDesc(0, &desc);

   return std::make_tuple(desc.Format, glm::ivec2{static_cast<int>(desc.Width),
                                                  static_cast<int>(desc.Height)});
}

inline auto get_surface_metrics(IDirect3DSurface9& from)
   -> std::tuple<D3DFORMAT, glm::ivec2>
{

   D3DSURFACE_DESC desc;
   from.GetDesc(&desc);

   return std::make_tuple(desc.Format, glm::ivec2{static_cast<int>(desc.Width),
                                                  static_cast<int>(desc.Height)});
}

inline void set_fs_pass_state(IDirect3DDevice9& device, IDirect3DSurface9& dest) noexcept
{
   auto dest_size = std::get<glm::ivec2>(get_surface_metrics(dest));

   direct3d::Vs_2f_shader_constant<constants::vs::post_processing_start>{}
      .set(device, {-1.0f / dest_size.x, 1.0f / dest_size.y});

   D3DVIEWPORT9 viewport{0,
                         0,
                         static_cast<DWORD>(dest_size.x),
                         static_cast<DWORD>(dest_size.y),
                         0.0f,
                         1.0f};

   device.SetViewport(&viewport);
}

inline void set_fs_pass_ps_state(IDirect3DDevice9& device, IDirect3DTexture9& source) noexcept
{
   auto src_size =
      static_cast<glm::vec2>(std::get<glm::ivec2>(get_texture_metrics(source)));

   direct3d::Ps_4f_shader_constant<constants::ps::post_processing_start>{}
      .set(device, {1.0f / src_size, src_size});
}

inline void set_blur_pass_state(IDirect3DDevice9& device, IDirect3DTexture9& source,
                                glm::vec2 direction) noexcept
{
   auto src_size =
      static_cast<glm::vec2>(std::get<glm::ivec2>(get_texture_metrics(source)));

   direct3d::Ps_4f_shader_constant<constants::ps::post_processing_start>{}
      .set(device, {1.0f / src_size * direction, 1.0f / src_size});
}

inline constexpr auto colorwrite_all =
   D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
   D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA;

inline void set_fs_pass_blend_state(IDirect3DDevice9& device) noexcept
{
   device.SetRenderState(D3DRS_ALPHABLENDENABLE, true);
   device.SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, true);
   device.SetRenderState(D3DRS_COLORWRITEENABLE, colorwrite_all);
   device.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
   device.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
   device.SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
   device.SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
   device.SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
}

inline void set_fs_pass_additive_blend_state(IDirect3DDevice9& device) noexcept
{
   device.SetRenderState(D3DRS_ALPHABLENDENABLE, true);
   device.SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, true);
   device.SetRenderState(D3DRS_COLORWRITEENABLE, colorwrite_all);
   device.SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
   device.SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
   device.SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
   device.SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
   device.SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
}

inline void set_linear_clamp_sampler(IDirect3DDevice9& device, int slot,
                                     bool srgb = false) noexcept
{
   device.SetSamplerState(slot, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   device.SetSamplerState(slot, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   device.SetSamplerState(slot, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
   device.SetSamplerState(slot, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
   device.SetSamplerState(slot, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   device.SetSamplerState(slot, D3DSAMP_SRGBTEXTURE, srgb);
}

inline void set_point_clamp_sampler(IDirect3DDevice9& device, int slot,
                                    bool srgb = false) noexcept
{
   device.SetSamplerState(slot, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   device.SetSamplerState(slot, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   device.SetSamplerState(slot, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   device.SetSamplerState(slot, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
   device.SetSamplerState(slot, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   device.SetSamplerState(slot, D3DSAMP_SRGBTEXTURE, srgb);
}

}
