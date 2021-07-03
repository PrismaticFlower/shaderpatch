#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "com_ptr.hpp"
#include "format_patcher.hpp"
#include "resource_access.hpp"
#include "upload_texture.hpp"

#include <memory>
#include <optional>

#include <d3d9.h>

namespace sp::d3d9 {

class Texturecube_managed final : public IDirect3DCubeTexture9, public Texture_accessor {
public:
   static Com_ptr<Texturecube_managed> create(
      core::Shader_patch& shader_patch, const UINT width, const UINT mip_levels,
      const DXGI_FORMAT format, const D3DFORMAT reported_format,
      std::unique_ptr<Format_patcher> format_patcher) noexcept;

   Texturecube_managed(const Texturecube_managed&) = delete;
   Texturecube_managed& operator=(const Texturecube_managed&) = delete;

   Texturecube_managed(Texturecube_managed&&) = delete;
   Texturecube_managed& operator=(Texturecube_managed&&) = delete;

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

   virtual HRESULT __stdcall GetLevelDesc(UINT level, D3DSURFACE_DESC* desc) noexcept;

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetCubeMapSurface(
      D3DCUBEMAP_FACES, UINT, IDirect3DSurface9**) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   virtual HRESULT __stdcall LockRect(D3DCUBEMAP_FACES face, UINT level,
                                      D3DLOCKED_RECT* locked_rect,
                                      const RECT* rect, DWORD flags) noexcept;

   virtual HRESULT __stdcall UnlockRect(D3DCUBEMAP_FACES face, UINT level) noexcept;

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall AddDirtyRect(
      D3DCUBEMAP_FACES, const RECT*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   auto type() const noexcept -> Texture_accessor_type override;

   auto dimension() const noexcept -> Texture_accessor_dimension override;

   auto texture() const noexcept -> core::Game_texture override;

private:
   Texturecube_managed(core::Shader_patch& shader_patch, const UINT width,
                       const UINT mip_levels, const DXGI_FORMAT format,
                       const D3DFORMAT reported_format,
                       std::unique_ptr<Format_patcher> format_patcher) noexcept;

   ~Texturecube_managed() = default;

   core::Shader_patch& _shader_patch;
   core::Game_texture _game_texture = core::nullgametex;

   std::unique_ptr<Format_patcher> _format_patcher;
   std::optional<Upload_texture> _upload_texture;

   const UINT _width;
   const UINT _mip_levels;
   const DXGI_FORMAT _format;
   const D3DFORMAT _reported_format;
   const UINT _last_level;

   ULONG _ref_count = 1;
};

static_assert(
   !std::has_virtual_destructor_v<Texturecube_managed>,
   "Texturecube_managed must not have a virtual destructor as it will cause "
   "an ABI break.");

}
