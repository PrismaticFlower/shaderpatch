
#include "vertex_shader.hpp"
#include "debug_trace.hpp"

namespace sp::d3d9 {

Com_ptr<Vertex_shader> Vertex_shader::create(const std::uint32_t game_shader_index) noexcept
{
   return Com_ptr{new Vertex_shader{game_shader_index}};
}

Vertex_shader::Vertex_shader(const std::uint32_t game_shader_index) noexcept
   : _game_shader_index{game_shader_index}
{
}

HRESULT Vertex_shader::QueryInterface(const IID& iid, void** object) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!object) return E_INVALIDARG;

   if (iid == IID_IUnknown) {
      *object = static_cast<IUnknown*>(this);
   }
   else if (iid == IID_IDirect3DVertexShader9) {
      *object = this;
   }
   else {
      *object = nullptr;

      return E_NOINTERFACE;
   }

   AddRef();

   return S_OK;
}

ULONG Vertex_shader::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Vertex_shader::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

}
