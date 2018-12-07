#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "com_ptr.hpp"
#include "shader_metadata.hpp"

#include <d3d9.h>

namespace sp::d3d9 {

class Vertex_shader final : public IDirect3DVertexShader9 {
public:
   static Com_ptr<Vertex_shader> create(core::Shader_patch& shader_patch,
                                        const Shader_metadata metadata) noexcept;

   Vertex_shader(const Vertex_shader&) = delete;
   Vertex_shader& operator=(const Vertex_shader&) = delete;

   Vertex_shader(Vertex_shader&&) = delete;
   Vertex_shader& operator=(Vertex_shader&&) = delete;

   HRESULT
   __stdcall QueryInterface(const IID& iid, void** object) noexcept override;

   ULONG __stdcall AddRef() noexcept override;

   ULONG __stdcall Release() noexcept override;

   [[deprecated("unimplemented")]] HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetFunction(void*, UINT*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   auto game_shader() const noexcept -> const core::Game_shader&
   {
      return _game_shader;
   }

private:
   Vertex_shader(core::Shader_patch& shader_patch, const Shader_metadata metadata) noexcept;
   ~Vertex_shader() = default;

   const core::Game_shader _game_shader;

   ULONG _ref_count = 1;
};

}
