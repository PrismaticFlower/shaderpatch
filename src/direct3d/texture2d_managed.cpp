
#include "texture2d_managed.hpp"
#include "debug_trace.hpp"

#include <cstring>

namespace sp::d3d9 {

Texture2d_managed::Texture2d_managed(core::Shader_patch& shader_patch,
                                     const UINT width, const UINT height,
                                     const UINT mip_levels, const DXGI_FORMAT format,
                                     const D3DFORMAT reported_format,
                                     std::unique_ptr<Image_patcher> image_patcher) noexcept
   : Base_texture{Texture_type::texture2d},
     _image_patcher{std::move(image_patcher)},
     _shader_patch{shader_patch},
     _width{width},
     _height{height},
     _mip_levels{mip_levels},
     _last_level{mip_levels - 1},
     _format{format},
     _reported_format{reported_format},
     _surfaces{[&] {
        std::vector<std::unique_ptr<Surface>> surfaces;
        surfaces.reserve(_mip_levels);

        for (auto i = 0u; i < _mip_levels; ++i) {
           surfaces.emplace_back(std::make_unique<Surface>(*this, i));
        }

        return surfaces;
     }()}
{
   Expects(mip_levels != 0);

   if (_image_patcher) {
      _upload_image = _image_patcher->create_image(format, width, height, mip_levels);
   }
   else {
      _upload_image = DirectX::ScratchImage{};
      _upload_image->Initialize2D(format, width, height, 1, mip_levels);
   }
}

Com_ptr<Texture2d_managed> Texture2d_managed::create(
   core::Shader_patch& shader_patch, const UINT width, const UINT height,
   const UINT mip_levels, const DXGI_FORMAT format, const D3DFORMAT reported_format,
   std::unique_ptr<Image_patcher> image_patcher) noexcept
{
   return Com_ptr{new Texture2d_managed{shader_patch, width, height, mip_levels, format,
                                        reported_format, std::move(image_patcher)}};
}

HRESULT Texture2d_managed::QueryInterface(const IID& iid, void** object) noexcept
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
      *object = this;
   }
   else {
      *object = nullptr;

      return E_NOINTERFACE;
   }

   AddRef();

   return S_OK;
}

ULONG Texture2d_managed::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Texture2d_managed::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

D3DRESOURCETYPE Texture2d_managed::GetType() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return D3DRTYPE_TEXTURE;
}

DWORD Texture2d_managed::GetLevelCount() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _mip_levels;
}

HRESULT Texture2d_managed::GetLevelDesc(UINT level, D3DSURFACE_DESC* desc) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!desc) return D3DERR_INVALIDCALL;
   if (level >= _mip_levels) return D3DERR_INVALIDCALL;

   desc->Format = _reported_format;
   desc->Type = D3DRTYPE_SURFACE;
   desc->Usage = 0;
   desc->Pool = D3DPOOL_MANAGED;
   desc->MultiSampleType = D3DMULTISAMPLE_NONE;
   desc->MultiSampleQuality = 0;
   desc->Width = _width / (level + 1);
   desc->Height = _height / (level + 1);

   return S_OK;
}

HRESULT Texture2d_managed::GetSurfaceLevel(UINT level, IDirect3DSurface9** surface) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level >= _mip_levels) return D3DERR_INVALIDCALL;
   if (!surface) return D3DERR_INVALIDCALL;

   AddRef();

   *surface = reinterpret_cast<IDirect3DSurface9*>(_surfaces[level].get());

   return S_OK;
}

HRESULT Texture2d_managed::LockRect(UINT level, D3DLOCKED_RECT* locked_rect,
                                    const RECT* rect, DWORD flags) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level >= _mip_levels) return D3DERR_INVALIDCALL;
   if (!locked_rect) return D3DERR_INVALIDCALL;

   if (!_upload_image || rect || flags) {
      log_and_terminate("Unexpected texture lock call!");
   }

   const auto* image = _upload_image->GetImage(level, 0, 0);

   locked_rect->Pitch = image->rowPitch;
   locked_rect->pBits = image->pixels;

   return S_OK;
}

HRESULT Texture2d_managed::UnlockRect(UINT level) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level >= _mip_levels) return D3DERR_INVALIDCALL;
   if (!_upload_image) return D3DERR_INVALIDCALL;

   if (level == _last_level) {
      if (_image_patcher) {
         _upload_image = _image_patcher->patch_image(*_upload_image);
      }

      this->resource = _shader_patch.create_game_texture2d(*_upload_image);
      _upload_image = std::nullopt;
   }

   return S_OK;
}

Texture2d_managed::Surface::Surface(Texture2d_managed& owning_texture,
                                    const UINT level) noexcept
   : owning_texture{owning_texture}, level{level}
{
}

HRESULT Texture2d_managed::Surface::QueryInterface(const IID& iid, void** object) noexcept
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

ULONG Texture2d_managed::Surface::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return owning_texture.AddRef();
}

ULONG Texture2d_managed::Surface::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return owning_texture.Release();
}

D3DRESOURCETYPE Texture2d_managed::Surface::GetType() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return D3DRTYPE_SURFACE;
}

HRESULT Texture2d_managed::Surface::GetDesc(D3DSURFACE_DESC* desc) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return owning_texture.GetLevelDesc(level, desc);
}

HRESULT Texture2d_managed::Surface::LockRect(D3DLOCKED_RECT* locked_rect,
                                             const RECT* rect, DWORD flags) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return owning_texture.LockRect(level, locked_rect, rect, flags);
}

HRESULT Texture2d_managed::Surface::UnlockRect() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return owning_texture.UnlockRect(level);
}

}
