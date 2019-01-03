
#include "volume_resource_texture.hpp"
#include "debug_trace.hpp"
#include "volume_resource.hpp"

#include <gsl/gsl>

namespace sp::d3d9 {

Com_ptr<Volume_resource_texture> Volume_resource_texture::create(
   core::Shader_patch& shader_patch, const UINT width, const UINT height,
   const UINT depth, const D3DFORMAT reported_format) noexcept
{
   return Com_ptr{new Volume_resource_texture{shader_patch, width, height,
                                              depth, reported_format}};
}

Volume_resource_texture::Volume_resource_texture(core::Shader_patch& shader_patch,
                                                 const UINT width,
                                                 const UINT height, const UINT depth,
                                                 const D3DFORMAT reported_format) noexcept
   : Base_texture{Texture_type::resource},
     _shader_patch{shader_patch},
     _resource_size{unpack_resource_size(height, depth)},
     _width{width},
     _height{height},
     _depth{depth},
     _reported_format{reported_format}
{
}

HRESULT Volume_resource_texture::QueryInterface(const IID& iid, void** object) noexcept
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

ULONG Volume_resource_texture::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Volume_resource_texture::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

D3DRESOURCETYPE Volume_resource_texture::GetType() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return D3DRTYPE_VOLUMETEXTURE;
}

DWORD Volume_resource_texture::GetLevelCount() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return 1;
}

HRESULT Volume_resource_texture::GetLevelDesc(UINT level, D3DVOLUME_DESC* desc) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!desc) return D3DERR_INVALIDCALL;
   if (level != 0) return D3DERR_INVALIDCALL;

   desc->Format = _reported_format;
   desc->Type = D3DRTYPE_SURFACE;
   desc->Usage = 0;
   desc->Pool = D3DPOOL_MANAGED;
   desc->Width = _width;
   desc->Height = _height;
   desc->Depth = _depth;

   return S_OK;
}

HRESULT Volume_resource_texture::LockBox(UINT level, D3DLOCKED_BOX* locked_box,
                                         const D3DBOX* box, DWORD flags) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level != 0) return D3DERR_INVALIDCALL;
   if (!locked_box) return D3DERR_INVALIDCALL;

   if (_upload_buffer || box || flags) {
      log_and_terminate("Unexpected volume texture lock call!");
   }

   _upload_buffer = std::make_unique<std::byte[]>(_resource_size);

   locked_box->RowPitch = -1;
   locked_box->SlicePitch = -1;
   locked_box->pBits = _upload_buffer.get();

   return S_OK;
}

HRESULT Volume_resource_texture::UnlockBox(UINT level) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level != 0) return D3DERR_INVALIDCALL;
   if (!_upload_buffer) return D3DERR_INVALIDCALL;

   _texture = _shader_patch.create_patch_texture(
      gsl::make_span(_upload_buffer.get(), _resource_size));
   _upload_buffer = nullptr;

   return S_OK;
}
}
