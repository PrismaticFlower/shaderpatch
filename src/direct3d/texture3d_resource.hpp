#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "base_texture.hpp"
#include "com_ptr.hpp"
#include "debug_trace.hpp"
#include "volume_resource.hpp"

#include <memory>
#include <type_traits>

#include <d3d9.h>

#include <gsl/gsl>

namespace sp::d3d9 {

template<typename Creator>
class Texture3d_resource final : public Base_texture {
public:
   static_assert(std::is_nothrow_invocable_v<Creator, const gsl::span<const std::byte>>,
                 "Creator must be nothrow invocable!");
   static_assert(
      std::is_nothrow_assignable_v<Resource::Resource_variant,
                                   std::invoke_result_t<Creator, const gsl::span<const std::byte>>>,
      "Creator's return type must be nothrow assignable to "
      "Resource::Resource_variant!");

   static Com_ptr<Texture3d_resource> create(Creator creator, const UINT width,
                                             const UINT height, const UINT depth,
                                             const D3DFORMAT reported_format) noexcept
   {
      return Com_ptr{new Texture3d_resource{std::move(creator), width, height,
                                            depth, reported_format}};
   }

   Texture3d_resource(const Texture3d_resource&) = delete;
   Texture3d_resource& operator=(const Texture3d_resource&) = delete;

   Texture3d_resource(Texture3d_resource&&) = delete;
   Texture3d_resource& operator=(Texture3d_resource&&) = delete;

   HRESULT
   __stdcall QueryInterface(const IID& iid, void** object) noexcept override
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

   ULONG __stdcall AddRef() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return _ref_count += 1;
   }

   ULONG __stdcall Release() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      const auto ref_count = _ref_count -= 1;

      if (ref_count == 0) delete this;

      return ref_count;
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetPrivateData(const GUID&,
                                                                    const void*, DWORD,
                                                                    DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetPrivateData(const GUID&,
                                                                    void*, DWORD*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall FreePrivateData(const GUID&) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] DWORD __stdcall SetPriority(DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated(
      "unimplemented")]] DWORD __stdcall GetPriority() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] void __stdcall PreLoad() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   D3DRESOURCETYPE __stdcall GetType() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return D3DRTYPE_VOLUMETEXTURE;
   }

   [[deprecated("unimplemented")]] DWORD __stdcall SetLOD(DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] DWORD __stdcall GetLOD() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   DWORD __stdcall GetLevelCount() noexcept override
   {
      Debug_trace::func(__FUNCSIG__);

      return 1;
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall SetAutoGenFilterType(
      D3DTEXTUREFILTERTYPE) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemente"
                "d")]] D3DTEXTUREFILTERTYPE __stdcall GetAutoGenFilterType() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated(
      "unimplemented")]] void __stdcall GenerateMipSubLevels() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   virtual HRESULT __stdcall GetLevelDesc(UINT level, D3DVOLUME_DESC* desc) noexcept
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

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetVolumeLevel(
      UINT, IDirect3DVolume9**) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   virtual HRESULT __stdcall LockBox(UINT level, D3DLOCKED_BOX* locked_box,
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

   virtual HRESULT __stdcall UnlockBox(UINT level) noexcept
   {
      Debug_trace::func(__FUNCSIG__);

      if (level != 0) return D3DERR_INVALIDCALL;
      if (!_upload_buffer) return D3DERR_INVALIDCALL;

      this->resource = _creator(gsl::make_span(_upload_buffer.get(), _resource_size));
      _upload_buffer = nullptr;

      return S_OK;
   }

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall AddDirtyBox(const D3DBOX*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

private:
   Texture3d_resource(Creator creator, const UINT width, const UINT height,
                      const UINT depth, const D3DFORMAT reported_format) noexcept
      : Base_texture{Texture_type::resource},
        _creator{std::move(creator)},
        _resource_size{unpack_resource_size(height, depth)},
        _width{width},
        _height{height},
        _depth{depth},
        _reported_format{reported_format}
   {
   }

   ~Texture3d_resource() = default;

   const Creator _creator;

   const UINT _resource_size;
   std::unique_ptr<std::byte[]> _upload_buffer;

   const UINT _width;
   const UINT _height;
   const UINT _depth;
   const D3DFORMAT _reported_format;

   ULONG _ref_count = 1;
};

template<typename Creator>
inline auto make_texture3d_resource(Creator&& creator, const UINT width,
                                    const UINT height, const UINT depth,
                                    const D3DFORMAT reported_format)
   -> Com_ptr<Texture3d_resource<std::remove_cv_t<Creator>>>
{
   using Resource = Texture3d_resource<std::remove_cv_t<Creator>>;

   static_assert(
      !std::has_virtual_destructor_v<Resource>,
      "Texture3d_resource must not have a virtual destructor as it will cause "
      "an ABI break.");

   return Resource::create(std::forward<Creator>(creator), width, height, depth,
                           reported_format);
}

}
