#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "com_ptr.hpp"
#include "resource_access.hpp"
#include "upload_texture.hpp"

#include <optional>

#include <d3d9.h>

namespace sp::d3d9 {

class Texture3d_managed final : public IDirect3DVolumeTexture9, public Texture_accessor {
public:
   static Com_ptr<Texture3d_managed> create(core::Shader_patch& shader_patch,
                                            const UINT width, const UINT height,
                                            const UINT depth, const UINT mip_levels,
                                            const DXGI_FORMAT format,
                                            const D3DFORMAT reported_format) noexcept;

   Texture3d_managed(const Texture3d_managed&) = delete;
   Texture3d_managed& operator=(const Texture3d_managed&) = delete;

   Texture3d_managed(Texture3d_managed&&) = delete;
   Texture3d_managed& operator=(Texture3d_managed&&) = delete;

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

   [[deprecated("unimplemented")]] DWORD __stdcall GetPriority() noexcept override
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

   auto type() const noexcept -> Texture_accessor_type override;

   auto dimension() const noexcept -> Texture_accessor_dimension override;

   auto texture() const noexcept -> core::Game_texture_handle override;

private:
   Texture3d_managed(core::Shader_patch& shader_patch, const UINT width,
                     const UINT height, const UINT depth, const UINT mip_levels,
                     const DXGI_FORMAT format, const D3DFORMAT reported_format) noexcept;

   ~Texture3d_managed();

   core::Shader_patch& _shader_patch;
   core::Game_texture_handle _game_texture = core::null_handle;

   std::optional<Upload_texture> _upload_texture;

   const UINT _width;
   const UINT _height;
   const UINT _depth;
   const UINT _mip_levels;
   const DXGI_FORMAT _format;
   const D3DFORMAT _reported_format;
   const UINT _last_level;

   ULONG _ref_count = 1;
};

}
