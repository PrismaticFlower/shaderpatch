
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
   : Base_texture{Texture_type::texture3d},
     _shader_patch{shader_patch},
     _width{width},
     _height{height},
     _depth{depth},
     _mip_levels{mip_levels},
     _format{format},
     _reported_format{reported_format},
     _last_level{mip_levels - 1}
{
   Expects(mip_levels != 0);

   _upload_image = DirectX::ScratchImage{};
   _upload_image->Initialize3D(_format, _width, _height, _depth, _mip_levels);
}

HRESULT Texture3d_managed::QueryInterface(const IID& iid, void** object) noexcept
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
   else if (iid == IID_IDirect3DVolumeTexture9) {
      *object = this;
   }
   else {
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
   desc->Type = D3DRTYPE_SURFACE;
   desc->Usage = 0;
   desc->Pool = D3DPOOL_MANAGED;
   desc->Width = _width / (level + 1);
   desc->Height = _height / (level + 1);
   desc->Depth = _depth / (level + 1);

   return S_OK;
}

HRESULT Texture3d_managed::LockBox(UINT level, D3DLOCKED_BOX* locked_box,
                                   const D3DBOX* box, DWORD flags) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level >= _mip_levels) return D3DERR_INVALIDCALL;
   if (!locked_box) return D3DERR_INVALIDCALL;

   if (!_upload_image || box || flags) {
      log_and_terminate("Unexpected volume texture lock call!");
   }

   const auto* image = _upload_image->GetImage(level, 0, 0);

   locked_box->RowPitch = image->rowPitch;
   locked_box->SlicePitch = image->slicePitch;
   locked_box->pBits = image->pixels;

   return S_OK;
}

HRESULT Texture3d_managed::UnlockBox(UINT level) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level >= _mip_levels) return D3DERR_INVALIDCALL;
   if (!_upload_image) return D3DERR_INVALIDCALL;

   if (level == _last_level) {
      this->resource = _shader_patch.create_game_texture3d(*_upload_image);
      _upload_image = std::nullopt;
   }

   return S_OK;
}
}
