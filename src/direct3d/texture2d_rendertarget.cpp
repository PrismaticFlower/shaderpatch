
#include "texture2d_rendertarget.hpp"
#include "debug_trace.hpp"

namespace sp::d3d9 {

Texture2d_rendertarget::Texture2d_rendertarget(core::Shader_patch& shader_patch,
                                               const UINT actual_width,
                                               const UINT actual_height,
                                               const UINT perceived_width,
                                               const UINT perceived_height) noexcept
   : _rendertarget_id{shader_patch.create_game_rendertarget(actual_width, actual_height),
                      shader_patch},
     _perceived_width{perceived_width},
     _perceived_height{perceived_height}
{
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
      *object = static_cast<IUnknown*>(static_cast<IDirect3DTexture9*>(this));
   }
   else if (iid == IID_IDirect3DResource9) {
      *object = static_cast<IDirect3DResource9*>(this);
   }
   else if (iid == IID_IDirect3DBaseTexture9) {
      *object = static_cast<IDirect3DBaseTexture9*>(this);
   }
   else if (iid == IID_IDirect3DTexture9) {
      *object = static_cast<IDirect3DTexture9*>(this);
   }
   else if (iid == IID_Texture_accessor) {
      *object = static_cast<Texture_accessor*>(this);
   }
   else {
      *object = nullptr;

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

   *surface = &_surface;

   return S_OK;
}

auto Texture2d_rendertarget::dimension() const noexcept -> Texture_accessor_dimension
{
   return Texture_accessor_dimension::_2d;
}

auto Texture2d_rendertarget::type() const noexcept -> Texture_accessor_type
{
   return Texture_accessor_type::texture_rendertarget;
}

auto Texture2d_rendertarget::texture_rendertarget() const noexcept -> core::Game_rendertarget_id
{
   return _rendertarget_id.id;
}

Texture2d_rendertarget::Surface::Surface(Texture2d_rendertarget& owning_texture) noexcept
   : owning_texture{owning_texture}
{
   Debug_trace::func(__FUNCSIG__);
}

HRESULT Texture2d_rendertarget::Surface::QueryInterface(const IID& iid, void** object) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!object) return E_INVALIDARG;

   if (iid == IID_IUnknown) {
      *object = static_cast<IUnknown*>(static_cast<IDirect3DSurface9*>(this));
   }
   else if (iid == IID_IDirect3DResource9) {
      *object = static_cast<IDirect3DResource9*>(this);
   }
   else if (iid == IID_IDirect3DSurface9) {
      *object = static_cast<IDirect3DSurface9*>(this);
   }
   else if (iid == IID_Rendertarget_accessor) {
      *object = static_cast<Rendertarget_accessor*>(this);
   }
   else {
      *object = nullptr;

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

auto Texture2d_rendertarget::Surface::rendertarget() const noexcept -> core::Game_rendertarget_id
{
   return owning_texture._rendertarget_id.id;
}

Texture2d_rendertarget::Rendertarget_id::~Rendertarget_id()
{
   Debug_trace::func(__FUNCSIG__);

   shader_patch.destroy_game_rendertarget_async(id);
}

}
