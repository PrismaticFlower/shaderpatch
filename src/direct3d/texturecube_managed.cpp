
#include "texturecube_managed.hpp"
#include "debug_trace.hpp"

namespace sp::d3d9 {

Com_ptr<Texturecube_managed> Texturecube_managed::create(
   core::Shader_patch& shader_patch, const UINT width, const UINT mip_levels,
   const DXGI_FORMAT format, const D3DFORMAT reported_format) noexcept
{
   Expects(mip_levels != 0);

   return Com_ptr{new Texturecube_managed{shader_patch, width, mip_levels,
                                          format, reported_format}};
}

Texturecube_managed::Texturecube_managed(core::Shader_patch& shader_patch,
                                         const UINT width, const UINT mip_levels,
                                         const DXGI_FORMAT format,
                                         const D3DFORMAT reported_format) noexcept
   : Base_texture{Texture_type::texturecube},
     _shader_patch{shader_patch},
     _width{width},
     _mip_levels{mip_levels},
     _format{format},
     _reported_format{reported_format},
     _last_level{mip_levels - 1}
{
   Expects(mip_levels != 0);

   _upload_texture.emplace(upload_scratch_buffer, format, width, width, 1,
                           mip_levels, 6);
}

HRESULT Texturecube_managed::QueryInterface(const IID& iid, void** object) noexcept
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
   else if (iid == IID_IDirect3DCubeTexture9) {
      *object = this;
   }
   else {
      *object = nullptr;

      return E_NOINTERFACE;
   }

   AddRef();

   return S_OK;
}

ULONG Texturecube_managed::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Texturecube_managed::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

D3DRESOURCETYPE Texturecube_managed::GetType() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return D3DRTYPE_CUBETEXTURE;
}

DWORD Texturecube_managed::GetLevelCount() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _mip_levels;
}

HRESULT Texturecube_managed::GetLevelDesc(UINT level, D3DSURFACE_DESC* desc) noexcept
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
   desc->Height = desc->Width;

   return S_OK;
}

HRESULT Texturecube_managed::LockRect(D3DCUBEMAP_FACES face, UINT level,
                                      D3DLOCKED_RECT* locked_rect,
                                      const RECT* rect, DWORD flags) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto face_index = static_cast<UINT>(face);

   if (face >= 6) return D3DERR_INVALIDCALL;
   if (level >= _mip_levels) return D3DERR_INVALIDCALL;
   if (!locked_rect) return D3DERR_INVALIDCALL;

   if (!_upload_texture || rect || flags) {
      log_and_terminate("Unexpected texture lock call!");
   }

   const auto subresource = _upload_texture->subresource(level, face_index);

   locked_rect->pBits = subresource.data;
   locked_rect->Pitch = subresource.row_pitch;

   return S_OK;
}

HRESULT Texturecube_managed::UnlockRect(D3DCUBEMAP_FACES face, UINT level) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (static_cast<UINT>(face) >= 6) return D3DERR_INVALIDCALL;
   if (level >= _mip_levels) return D3DERR_INVALIDCALL;
   if (!_upload_texture) return D3DERR_INVALIDCALL;

   if (level == _last_level && face == D3DCUBEMAP_FACE_NEGATIVE_Z) {
      this->resource =
         _shader_patch.create_game_texture_cube(_width, _width, _mip_levels, _format,
                                                _upload_texture->subresources());
      _upload_texture = std::nullopt;
   }

   return S_OK;
}

}
