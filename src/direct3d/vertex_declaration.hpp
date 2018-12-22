#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "com_ptr.hpp"

#include <gsl/gsl>

#include <d3d9.h>

namespace sp::d3d9 {

class Vertex_declaration final : public IDirect3DVertexDeclaration9 {
public:
   static Com_ptr<Vertex_declaration> create(
      core::Shader_patch& shader_patch,
      const gsl::span<const D3DVERTEXELEMENT9> elements) noexcept;

   Vertex_declaration(const Vertex_declaration&) = delete;
   Vertex_declaration& operator=(const Vertex_declaration&) = delete;

   Vertex_declaration(Vertex_declaration&&) = delete;
   Vertex_declaration& operator=(Vertex_declaration&&) = delete;

   HRESULT
   __stdcall QueryInterface(const IID& iid, void** object) noexcept override;

   ULONG __stdcall AddRef() noexcept override;

   ULONG __stdcall Release() noexcept override;

   [[deprecated("unimplemented")]] HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetDeclaration(D3DVERTEXELEMENT9*,
                                                                    UINT*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   auto input_layout() const noexcept -> const core::Game_input_layout&
   {
      return _input_layout;
   }

private:
   Vertex_declaration(core::Shader_patch& shader_patch,
                      const gsl::span<const D3DVERTEXELEMENT9> elements) noexcept;
   ~Vertex_declaration() = default;

   const core::Game_input_layout _input_layout;
   UINT _ref_count = 1;
};

}
