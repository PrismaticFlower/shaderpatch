
#include "texture3d_managed.hpp"
#include "debug_trace.hpp"

#include <gsl/gsl>

namespace sp::d3d9 {

Com_ptr<Texture3d_managed> Texture3d_managed::create(
   core::Shader_patch& shader_patch, const UINT width, const UINT height,
   const UINT depth, const UINT mip_levels, const DXGI_FORMAT format,
   const D3DFORMAT reported_format) noexcept
{
   Expects(mip_levels != 0);

   return Com_ptr{new Texture3d_managed{shader_patch, width, height, depth,
                                        mip_levels, format, reported_format}};
}

Texture3d_managed::Texture3d_managed(core::Shader_patch& shader_patch,
                                     const UINT width, const UINT height,
                                     const UINT depth, const UINT mip_levels,
                                     const DXGI_FORMAT format,
                                     const D3DFORMAT reported_format) noexcept
   : _shader_patch{shader_patch},
     _width{width},
     _height{height},
     _depth{depth},
     _mip_levels{mip_levels},
     _format{format},
     _reported_format{reported_format},
     _last_level{mip_levels - 1}
{
   Expects(mip_levels != 0);

   _upload_texture.emplace(upload_scratch_buffer, _format, _width, _height,
                           _depth, _mip_levels, 1);
}

Texture3d_managed::~Texture3d_managed()
{
   if (_game_texture) _shader_patch.destroy_game_texture(_game_texture);
}

HRESULT Texture3d_managed::QueryInterface(const IID& iid, void** object) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!object) return E_INVALIDARG;

   if (iid == IID_Texture_accessor) [[likely]] {
      *object = static_cast<Texture_accessor*>(this);
   }
   else if (iid == IID_IUnknown) {
      *object = static_cast<IUnknown*>(static_cast<IDirect3DVolumeTexture9*>(this));
   }
   else if (iid == IID_IDirect3DResource9) {
      *object = static_cast<IDirect3DResource9*>(this);
   }
   else if (iid == IID_IDirect3DBaseTexture9) {
      *object = static_cast<IDirect3DBaseTexture9*>(this);
   }
   else if (iid == IID_IDirect3DVolumeTexture9) {
      *object = static_cast<IDirect3DVolumeTexture9*>(this);
   }
   else [[unlikely]] {
      *object = nullptr;

      return E_NOINTERFACE;
   }

   AddRef();

   return S_OK;
}

ULONG Texture3d_managed::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Texture3d_managed::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

D3DRESOURCETYPE Texture3d_managed::GetType() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return D3DRTYPE_VOLUMETEXTURE;
}

DWORD Texture3d_managed::GetLevelCount() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _mip_levels;
}

HRESULT Texture3d_managed::GetLevelDesc(UINT level, D3DVOLUME_DESC* desc) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!desc) return D3DERR_INVALIDCALL;
   if (level >= _mip_levels) return D3DERR_INVALIDCALL;

   desc->Format = _reported_format;
   desc->Type = D3DRTYPE_VOLUME;
   desc->Usage = 0;
   desc->Pool = D3DPOOL_MANAGED;
   desc->Width = std::max(_width >> level, 1u);
   desc->Height = std::max(_height >> level, 1u);
   desc->Depth = std::max(_depth >> level, 1u);

   return S_OK;
}

HRESULT Texture3d_managed::LockBox(UINT level, D3DLOCKED_BOX* locked_box,
                                   const D3DBOX* box, DWORD flags) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level >= _mip_levels) return D3DERR_INVALIDCALL;
   if (!locked_box) return D3DERR_INVALIDCALL;

   if (!_upload_texture || box || flags) {
      log_and_terminate("Unexpected volume texture lock call!");
   }

   const auto subresource = _upload_texture->subresource(level, 0);

   locked_box->RowPitch = subresource.row_pitch;
   locked_box->SlicePitch = subresource.depth_pitch;
   locked_box->pBits = subresource.data;

   return S_OK;
}

HRESULT Texture3d_managed::UnlockBox(UINT level) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level >= _mip_levels) return D3DERR_INVALIDCALL;
   if (!_upload_texture) return D3DERR_INVALIDCALL;

   if (level == _last_level) {
      _game_texture =
         _shader_patch.create_game_texture3d(_width, _height, _depth,
                                             _mip_levels, _format,
                                             _upload_texture->subresources());
      _upload_texture = std::nullopt;
   }

   return S_OK;
}

auto Texture3d_managed::type() const noexcept -> Texture_accessor_type
{
   return Texture_accessor_type::texture;
}

auto Texture3d_managed::dimension() const noexcept -> Texture_accessor_dimension
{
   return Texture_accessor_dimension::_3d;
}

auto Texture3d_managed::texture() const noexcept -> core::Game_texture_handle
{
   return _game_texture;
}

}
