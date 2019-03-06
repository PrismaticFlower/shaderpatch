
#include "texture3d_resource.hpp"
#include "debug_trace.hpp"
#include "upload_scratch_buffer.hpp"
#include "volume_resource.hpp"

namespace sp::d3d9 {

Com_ptr<Texture3d_resource> Texture3d_resource::create(core::Shader_patch& patch,
                                                       const UINT width,
                                                       const UINT height,
                                                       const UINT depth) noexcept
{
   return Com_ptr{new Texture3d_resource{patch, width, height, depth}};
}

Texture3d_resource::Texture3d_resource(core::Shader_patch& patch, const UINT width,
                                       const UINT height, const UINT depth) noexcept
   : Base_texture{Texture_type::resource}, _patch{patch}, _width{width}, _height{height}, _depth{depth}
{
   Expects(_resource_size >= sizeof(Volume_resource_header));
}

HRESULT
Texture3d_resource::QueryInterface(const IID& iid, void** object) noexcept
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

ULONG Texture3d_resource::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Texture3d_resource::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

D3DRESOURCETYPE Texture3d_resource::GetType() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return D3DRTYPE_VOLUMETEXTURE;
}

DWORD Texture3d_resource::GetLevelCount() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return 1;
}

HRESULT Texture3d_resource::GetLevelDesc(UINT level, D3DVOLUME_DESC* desc) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!desc) return D3DERR_INVALIDCALL;
   if (level != 0) return D3DERR_INVALIDCALL;

   desc->Type = D3DRTYPE_VOLUME;
   desc->Usage = 0;
   desc->Pool = D3DPOOL_MANAGED;
   desc->Width = _width;
   desc->Height = _height;
   desc->Depth = _depth;

   return S_OK;
}

HRESULT Texture3d_resource::LockBox(UINT level, D3DLOCKED_BOX* locked_box,
                                    const D3DBOX* box, DWORD flags) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level != 0) return D3DERR_INVALIDCALL;
   if (!locked_box) return D3DERR_INVALIDCALL;

   if (std::exchange(_locked, true) || box || flags) {
      log_and_terminate("Unexpected volume texture lock call!");
   }

   _lock_data = upload_scratch_buffer.lock(_resource_size);

   locked_box->RowPitch = _width;
   locked_box->SlicePitch = _width * _height;
   locked_box->pBits = _lock_data;

   return S_OK;
}

HRESULT Texture3d_resource::UnlockBox(UINT level) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (level != 0) return D3DERR_INVALIDCALL;

   if (!std::exchange(_locked, false)) {
      log_and_terminate("Unexpected volume texture unlock call!");
   }

   create_resource();

   upload_scratch_buffer.unlock();

   return S_OK;
}

void Texture3d_resource::create_resource() noexcept
{
   const auto volume_res_header = bit_cast<Volume_resource_header>(
      gsl::span{_lock_data, sizeof(Volume_resource_header)});

   if (volume_res_header.mn != "spvr"_mn) {
      log_and_terminate("Unexpected volume resource magic number!");
   }

   const auto payload = gsl::make_span(_lock_data + sizeof(Volume_resource_header),
                                       volume_res_header.payload_size);

   switch (volume_res_header.type) {
   case Volume_resource_type::material:
      this->resource = _patch.create_patch_material(payload);
      break;
   case Volume_resource_type::texture:
      this->resource = _patch.create_patch_texture(payload);
      break;
   case Volume_resource_type::fx_config:
      this->resource = _patch.create_patch_effects_config(payload);
      break;
   }
}

}
