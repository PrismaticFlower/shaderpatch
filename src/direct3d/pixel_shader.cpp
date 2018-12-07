
#include "pixel_shader.hpp"
#include "debug_trace.hpp"

namespace sp::d3d9 {

Com_ptr<Pixel_shader> Pixel_shader::create() noexcept
{
   static auto shader = Pixel_shader{};

   return Com_ptr{&shader};
}

HRESULT Pixel_shader::QueryInterface(const IID& iid, void** object) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!object) return E_INVALIDARG;

   if (iid == IID_IUnknown) {
      *object = static_cast<IUnknown*>(this);
   }
   else if (iid == IID_IDirect3DPixelShader9) {
      *object = this;
   }
   else {
      *object = nullptr;

      return E_NOINTERFACE;
   }

   AddRef();

   return S_OK;
}

ULONG Pixel_shader::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Pixel_shader::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   return ref_count;
}

}
