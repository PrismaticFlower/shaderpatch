
#include "texture2d_rendertarget.hpp"
#include "debug_trace.hpp"

namespace sp::d3d9 {

Texture2d_rendertarget::Texture2d_rendertarget(core::Shader_patch& shader_patch,
                                               const UINT actual_width,
                                               const UINT actual_height,
                                               const UINT perceived_width,
                                               const UINT perceived_height) noexcept
   : Base_texture{Texture_type::rendertarget},
     _rendertarget_id{shader_patch.create_game_rendertarget(actual_width, actual_height),
                      shader_patch},
     _perceived_width{perceived_width},
     _perceived_height{perceived_height}
{
   this->resource = _rendertarget_id.id;
}

Com_ptr<Texture2d_rendertarget> Texture2d_rendertarget::create(
   core::Shader_patch& shader_patch, const UINT actual_width, const UINT actual_height,
   const UINT perceived_width, const UINT perceived_height) noexcept
{
   return Com_ptr{new Texture2d_rendertarget{shader_patch, actual_width, actual_height,
                                             perceived_width, perceived_height}};
}

HRESULT Texture2d_rendertarget::QueryInterface(const IID& iid, void** object) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!object) return E_INVALIDARG;

   if (iid == IID_IUnknown) {
      *object = static_cast<IUnknown*>(this);
   }
   else if (iid == IID_IDirect3DResource9) {
      *object = static_cast<Resource*>(this);
   }
   else if (iid == IID_IDirect3DBaseTexture9) {
      *object = static_cast<Base_texture*>(this);
   }
   else if (iid == IID_IDirect3DTexture9) {
      *object = nullptr;

      *object = this;
   }
   else {
      return E_NOINTERFACE;
   }

   AddRef();

   return S_OK;
}

ULONG Texture2d_rendertarget::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Texture2d_rendertarget::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

D3DRESOURCETYPE Texture2d_rendertarget::GetType() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return D3DRTYPE_TEXTURE;
}

DWORD Texture2d_rendertarget::GetLevelCount() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return 1;
}

HRESULT Texture2d_rendertarget::GetLevelDesc(UINT level, D3DSURFACE_DESC* desc) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level != 0) return D3DERR_INVALIDCALL;
   if (!desc) return D3DERR_INVALIDCALL;

   desc->Format = reported_format;
   desc->Type = D3DRTYPE_SURFACE;
   desc->Usage = D3DUSAGE_RENDERTARGET;
   desc->Pool = D3DPOOL_DEFAULT;
   desc->MultiSampleType = D3DMULTISAMPLE_NONE;
   desc->MultiSampleQuality = 0;
   desc->Width = _perceived_width;
   desc->Height = _perceived_height;

   return S_OK;
}

HRESULT Texture2d_rendertarget::GetSurfaceLevel(UINT level, IDirect3DSurface9** surface) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level != 0) return D3DERR_INVALIDCALL;
   if (!surface) return D3DERR_INVALIDCALL;

   AddRef();

   *surface = reinterpret_cast<IDirect3DSurface9*>(&_surface);

   return S_OK;
}

Texture2d_rendertarget::Surface::Surface(Texture2d_rendertarget& owning_texture) noexcept
   : owning_texture{owning_texture}
{
   Debug_trace::func(__FUNCSIG__);

   this->resource = owning_texture._rendertarget_id.id;
}

HRESULT Texture2d_rendertarget::Surface::QueryInterface(const IID& iid, void** object) noexcept
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
      *object = nullptr;

      *object = this;
   }
   else {
      return E_NOINTERFACE;
   }

   AddRef();

   return S_OK;
}

ULONG Texture2d_rendertarget::Surface::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return owning_texture.AddRef();
}

ULONG Texture2d_rendertarget::Surface::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return owning_texture.Release();
}

D3DRESOURCETYPE Texture2d_rendertarget::Surface::GetType() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return D3DRTYPE_SURFACE;
}

HRESULT Texture2d_rendertarget::Surface::GetDesc(D3DSURFACE_DESC* desc) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return owning_texture.GetLevelDesc(0, desc);
}

Texture2d_rendertarget::Rendertarget_id::~Rendertarget_id()
{
   Debug_trace::func(__FUNCSIG__);

   shader_patch.destroy_game_rendertarget(id);
}

}
