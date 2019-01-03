#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "base_texture.hpp"
#include "com_ptr.hpp"

#include <memory>

#include <d3d9.h>

namespace sp::d3d9 {

class Volume_resource_texture final : public Base_texture {
public:
   static Com_ptr<Volume_resource_texture> create(
      core::Shader_patch& shader_patch, const UINT width, const UINT height,
      const UINT depth, const D3DFORMAT reported_format) noexcept;

   Volume_resource_texture(const Volume_resource_texture&) = delete;
   Volume_resource_texture& operator=(const Volume_resource_texture&) = delete;

   Volume_resource_texture(Volume_resource_texture&&) = delete;
   Volume_resource_texture& operator=(Volume_resource_texture&&) = delete;

   HRESULT
   __stdcall QueryInterface(const IID& iid, void** object) noexcept override;

   ULONG __stdcall AddRef() noexcept override;

   ULONG __stdcall Release() noexcept override;

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

   D3DRESOURCETYPE __stdcall GetType() noexcept override;

   [[deprecated("unimplemented")]] DWORD __stdcall SetLOD(DWORD) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] DWORD __stdcall GetLOD() noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   DWORD __stdcall GetLevelCount() noexcept override;

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

   virtual HRESULT __stdcall GetLevelDesc(UINT level, D3DVOLUME_DESC* desc) noexcept;

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetVolumeLevel(
      UINT, IDirect3DVolume9**) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   virtual HRESULT __stdcall LockBox(UINT level, D3DLOCKED_BOX* locked_box,
                                     const D3DBOX* box, DWORD flags) noexcept;

   virtual HRESULT __stdcall UnlockBox(UINT level) noexcept;

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall AddDirtyBox(const D3DBOX*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

private:
   Volume_resource_texture(core::Shader_patch& shader_patch, const UINT width,
                           const UINT height, const UINT depth,
                           const D3DFORMAT reported_format) noexcept;

   ~Volume_resource_texture() = default;

   core::Shader_patch& _shader_patch;
   core::Patch_texture _texture;

   const UINT _resource_size;
   std::unique_ptr<std::byte[]> _upload_buffer;

   const UINT _width;
   const UINT _height;
   const UINT _depth;
   const D3DFORMAT _reported_format;

   ULONG _ref_count = 1;
};

static_assert(
   !std::has_virtual_destructor_v<Volume_resource_texture>,
   "Texture3d_managed must not have a virtual destructor as it will cause "
   "an ABI break.");

}
