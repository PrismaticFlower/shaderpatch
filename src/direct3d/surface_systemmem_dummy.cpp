
#include "surface_systemmem_dummy.hpp"
#include "debug_trace.hpp"

#include <utility>

namespace sp::d3d9 {

Com_ptr<Surface_systemmem_dummy> Surface_systemmem_dummy::create(const UINT width,
                                                                 const UINT height) noexcept
{
   return Com_ptr{new Surface_systemmem_dummy{width, height}};
}

Surface_systemmem_dummy::Surface_systemmem_dummy(const UINT width, const UINT height) noexcept
   : _width{width}, _height{height}
{
}

HRESULT Surface_systemmem_dummy::QueryInterface(const IID& iid, void** object) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!object) return E_INVALIDARG;

   if (iid == IID_IUnknown) {
      *object = static_cast<IUnknown*>(this);
   }
   else if (iid == IID_IDirect3DResource9) {
      *object = static_cast<Resource*>(this);
   }
   else if (iid == IID_IDirect3DSurface9) {
      *object = this;
   }
   else {
      *object = nullptr;

      return E_NOINTERFACE;
   }

   AddRef();

   return S_OK;
}

ULONG Surface_systemmem_dummy::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Surface_systemmem_dummy::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

D3DRESOURCETYPE Surface_systemmem_dummy::GetType() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return D3DRTYPE_SURFACE;
}

HRESULT Surface_systemmem_dummy::GetDesc(D3DSURFACE_DESC* desc) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!desc) return D3DERR_INVALIDCALL;

   desc->Format = dummy_format;
   desc->Type = D3DRTYPE_SURFACE;
   desc->Usage = 0;
   desc->Pool = D3DPOOL_SYSTEMMEM;
   desc->MultiSampleType = D3DMULTISAMPLE_NONE;
   desc->MultiSampleQuality = 0;
   desc->Width = _width;
   desc->Height = _height;

   return S_OK;
}

HRESULT Surface_systemmem_dummy::LockRect(D3DLOCKED_RECT* locked_rect,
                                          const RECT* rect, DWORD flags) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (_dummy_buffer || rect || flags) {
      log_and_terminate("Unexpected dummy surface lock!");
   }

   _dummy_buffer =
      std::make_unique<std::byte[]>(_width * _height * sizeof(std::uint32_t));

   locked_rect->Pitch = _width * sizeof(std::uint32_t);
   locked_rect->pBits = _dummy_buffer.get();

   return S_OK;
}

HRESULT Surface_systemmem_dummy::UnlockRect() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (std::exchange(_dummy_buffer, nullptr)) {
      log_and_terminate("Unexpected dummy surface unlock!");
   }

   return S_OK;
}
}
