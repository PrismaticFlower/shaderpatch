
#include "texture2d_managed.hpp"
#include "debug_trace.hpp"

#include <cstring>

namespace sp::d3d9 {

Texture2d_managed::Texture2d_managed(core::Shader_patch& shader_patch,
                                     const UINT width, const UINT height,
                                     const UINT mip_levels, const DXGI_FORMAT format,
                                     const D3DFORMAT reported_format,
                                     std::unique_ptr<Format_patcher> format_patcher) noexcept
   : _shader_patch{shader_patch},
     _format_patcher{std::move(format_patcher)},
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
}

Texture2d_managed::~Texture2d_managed()
{
   if (_game_texture) _shader_patch.destroy_game_texture_async(_game_texture);
}

Com_ptr<Texture2d_managed> Texture2d_managed::create(
   core::Shader_patch& shader_patch, const UINT width, const UINT height,
   const UINT mip_levels, const DXGI_FORMAT format, const D3DFORMAT reported_format,
   std::unique_ptr<Format_patcher> format_patcher) noexcept
{
   return Com_ptr{new Texture2d_managed{shader_patch, width, height, mip_levels, format,
                                        reported_format, std::move(format_patcher)}};
}

HRESULT Texture2d_managed::QueryInterface(const IID& iid, void** object) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!object) return E_INVALIDARG;

   if (iid == IID_Texture_accessor) [[likely]] {
      *object = static_cast<Texture_accessor*>(this);
   }
   else if (iid == IID_IUnknown) {
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
   else [[unlikely]] {
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
   desc->Width = std::max(_width >> level, 1u);
   desc->Height = std::max(_height >> level, 1u);

   return S_OK;
}

HRESULT Texture2d_managed::GetSurfaceLevel(UINT level, IDirect3DSurface9** surface) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level >= _mip_levels) return D3DERR_INVALIDCALL;
   if (!surface) return D3DERR_INVALIDCALL;

   AddRef();

   *surface = _surfaces[level].get();

   return S_OK;
}

HRESULT Texture2d_managed::LockRect(UINT level, D3DLOCKED_RECT* locked_rect,
                                    const RECT* rect, DWORD flags) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level >= _mip_levels) return D3DERR_INVALIDCALL;
   if (!locked_rect) return D3DERR_INVALIDCALL;

   if (rect || flags) {
      log_and_terminate("Unexpected texture lock call!");
   }

   if (std::exchange(_first_lock, false)) {
      _upload_texture =
         std::make_unique<Upload_texture>(upload_scratch_buffer, _format,
                                          _width, _height, 1, _mip_levels, 1);
   }
   else if (!_upload_texture && !std::exchange(_dynamic_texture, true)) {
      _shader_patch.convert_game_texture2d_to_dynamic(_game_texture);
      _dynamic_texture = true;
   }

   if (_dynamic_texture) {
      auto mapped_texture =
         _format_patcher
            ? _format_patcher->map_dynamic_texture(_format, _width, _height, level)
            : _shader_patch.map_dynamic_texture(_game_texture, level,
                                                D3D11_MAP_WRITE_DISCARD);

      locked_rect->Pitch = mapped_texture.row_pitch;
      locked_rect->pBits = mapped_texture.data;
   }
   else {
      const auto subresource = _upload_texture->subresource(level, 0);

      locked_rect->Pitch = subresource.row_pitch;
      locked_rect->pBits = subresource.data;
   }

   return S_OK;
}

HRESULT Texture2d_managed::UnlockRect(UINT level) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level >= _mip_levels) return D3DERR_INVALIDCALL;

   if (_dynamic_texture) {
      if (_format_patcher) {
         _format_patcher->unmap_dynamic_texture(_shader_patch, _game_texture, level);
      }
      else {
         _shader_patch.unmap_dynamic_texture(_game_texture, level);
      }
   }
   else {
      if (!_upload_texture) return D3DERR_INVALIDCALL;

      if (level == _last_level) {
         if (_format_patcher) {
            auto [patched_format, patched_texture] =
               _format_patcher->patch_texture(_format, _width, _height,
                                              _mip_levels, 1, *_upload_texture);

            _game_texture =
               _shader_patch.create_game_texture2d(_width, _height, _mip_levels,
                                                   patched_format,
                                                   patched_texture->subresources());
         }
         else {
            _game_texture =
               _shader_patch.create_game_texture2d(_width, _height, _mip_levels, _format,
                                                   _upload_texture->subresources());
         }

         _upload_texture = nullptr;
      }
   }

   return S_OK;
}

auto Texture2d_managed::type() const noexcept -> Texture_accessor_type
{
   return Texture_accessor_type::texture;
}

auto Texture2d_managed::dimension() const noexcept -> Texture_accessor_dimension
{
   return Texture_accessor_dimension::_2d;
}

auto Texture2d_managed::texture() const noexcept -> core::Game_texture_handle
{
   return _game_texture;
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
      *object = static_cast<IDirect3DResource9*>(this);
   }
   else if (iid == IID_IDirect3DSurface9) {
      *object = static_cast<IDirect3DSurface9*>(this);
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
