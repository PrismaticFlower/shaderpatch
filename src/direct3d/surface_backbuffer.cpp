
#include "surface_backbuffer.hpp"
#include "debug_trace.hpp"

namespace sp::d3d9 {

Com_ptr<Surface_backbuffer> Surface_backbuffer::create(const core::Game_rendertarget_id rendertarget_id,
                                                       const UINT width,
                                                       const UINT height) noexcept
{
   return Com_ptr{new Surface_backbuffer{rendertarget_id, width, height}};
}

Surface_backbuffer::Surface_backbuffer(const core::Game_rendertarget_id rendertarget_id,
                                       const UINT width, const UINT height) noexcept
   : _width{width}, _height{height}
{
   this->resource = rendertarget_id;
}

HRESULT Surface_backbuffer::QueryInterface(const IID& iid, void** object) noexcept
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

ULONG Surface_backbuffer::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Surface_backbuffer::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

D3DRESOURCETYPE Surface_backbuffer::GetType() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return D3DRTYPE_SURFACE;
}

HRESULT Surface_backbuffer::GetDesc(D3DSURFACE_DESC* desc) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!desc) return D3DERR_INVALIDCALL;

   desc->Format = reported_format;
   desc->Type = D3DRTYPE_SURFACE;
   desc->Usage = D3DUSAGE_RENDERTARGET;
   desc->Pool = D3DPOOL_DEFAULT;
   desc->MultiSampleType = D3DMULTISAMPLE_NONE;
   desc->MultiSampleQuality = 0;
   desc->Width = _width;
   desc->Height = _height;

   return S_OK;
}

}
