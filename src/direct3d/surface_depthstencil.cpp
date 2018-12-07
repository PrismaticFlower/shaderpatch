
#include "surface_depthstencil.hpp"
#include "debug_trace.hpp"

namespace sp::d3d9 {

Com_ptr<Surface_depthstencil> Surface_depthstencil::create(
   const core::Game_depthstencil depthstencil, const UINT width, const UINT height) noexcept
{
   return Com_ptr{new Surface_depthstencil{depthstencil, width, height}};
}

Surface_depthstencil::Surface_depthstencil(const core::Game_depthstencil depthstencil,
                                           const UINT width, const UINT height) noexcept
   : _width{width}, _height{height}
{
   this->resource = depthstencil;
}

HRESULT Surface_depthstencil::QueryInterface(const IID& iid, void** object) noexcept
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

ULONG Surface_depthstencil::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Surface_depthstencil::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

D3DRESOURCETYPE __stdcall Surface_depthstencil::GetType() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return D3DRTYPE_SURFACE;
}

HRESULT __stdcall Surface_depthstencil::GetDesc(D3DSURFACE_DESC* desc) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!desc) return D3DERR_INVALIDCALL;

   desc->Format = reported_format;
   desc->Type = D3DRTYPE_SURFACE;
   desc->Usage = D3DUSAGE_DEPTHSTENCIL;
   desc->Pool = D3DPOOL_DEFAULT;
   desc->MultiSampleType = D3DMULTISAMPLE_NONE;
   desc->MultiSampleQuality = 0;
   desc->Width = _width;
   desc->Height = _height;

   return S_OK;
}

}
