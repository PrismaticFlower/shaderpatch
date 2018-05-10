#pragma once

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
      -1.f - offset.x, 3.f + offset.x,  // pos 1
      0.f, -1.f,                        // uv 1
      3.f - offset.x, -1.f + offset.x,  // pos 2
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

}