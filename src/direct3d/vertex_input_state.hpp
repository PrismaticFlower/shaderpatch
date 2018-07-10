#pragma once

#include "com_ptr.hpp"

#include <d3d9.h>

namespace sp::direct3d {

struct Vertex_input_state {
   Com_ptr<IDirect3DVertexBuffer9> buffer;
   UINT offset;
   UINT stride;
   Com_ptr<IDirect3DVertexDeclaration9> declaration;
};

inline auto create_filled_vertex_input_state(IDirect3DDevice9& device) noexcept
{
   Vertex_input_state state{};

   device.GetStreamSource(0, state.buffer.clear_and_assign(), &state.offset,
                          &state.stride);
   device.GetVertexDeclaration(state.declaration.clear_and_assign());

   return state;
}

inline void apply_vertex_input_state(IDirect3DDevice9& device,
                                     const Vertex_input_state& state) noexcept
{
   device.SetStreamSource(0, state.buffer.get(), state.offset, state.stride);
   device.SetVertexDeclaration(state.declaration.get());
}

}
