#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "com_ptr.hpp"
#include "format_patcher.hpp"
#include "resource_access.hpp"
#include "upload_texture.hpp"

#include <memory>
#include <vector>

#include <d3d9.h>

namespace sp::d3d9 {

class Texture2d_managed final : public IDirect3DTexture9, public Texture_accessor {
public:
   static Com_ptr<Texture2d_managed> create(
      core::Shader_patch& shader_patch, const UINT width, const UINT height,
      const UINT mip_levels, const DXGI_FORMAT format, const D3DFORMAT reported_format,
      std::unique_ptr<Format_patcher> format_patcher = nullptr) noexcept;

   Texture2d_managed(const Texture2d_managed&) = delete;
   Texture2d_managed& operator=(const Texture2d_managed&) = delete;

   Texture2d_managed(Texture2d_managed&&) = delete;
   Texture2d_managed& operator=(Texture2d_managed&&) = delete;

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

   virtual HRESULT __stdcall GetSurfaceLevel(UINT level,
                                             IDirect3DSurface9** surface) noexcept;

   virtual HRESULT __stdcall LockRect(UINT level, D3DLOCKED_RECT* locked_rect,
                                      const RECT* rect, DWORD flags) noexcept;

   virtual HRESULT __stdcall UnlockRect(UINT level) noexcept;

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall AddDirtyRect(const RECT*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   auto type() const noexcept -> Texture_accessor_type override;

   auto dimension() const noexcept -> Texture_accessor_dimension override;

   auto texture() const noexcept -> core::Game_texture_handle override;

private:
   struct Surface;

   Texture2d_managed(core::Shader_patch& shader_patch, const UINT width,
                     const UINT height, const UINT mip_levels,
                     const DXGI_FORMAT format, const D3DFORMAT reported_format,
                     std::unique_ptr<Format_patcher> format_patcher) noexcept;

   ~Texture2d_managed();

   core::Shader_patch& _shader_patch;
   core::Game_texture_handle _game_texture = core::null_handle;

   std::unique_ptr<Format_patcher> _format_patcher;
   std::unique_ptr<Upload_texture> _upload_texture;

   bool _first_lock = true;
   bool _dynamic_texture = false;

   const UINT _width;
   const UINT _height;
   const UINT _mip_levels;
   const DXGI_FORMAT _format;
   const D3DFORMAT _reported_format;
   const UINT _last_level;
   const std::vector<std::unique_ptr<Surface>> _surfaces;

   ULONG _ref_count = 1;

   struct Surface final : public IDirect3DSurface9 {
      Texture2d_managed& owning_texture;
      const UINT level;

      Surface(Texture2d_managed& owning_texture, const UINT level) noexcept;

      Surface(const Surface&) = delete;
      Surface& operator=(const Surface&) = delete;

      Surface(Surface&&) = delete;
      Surface& operator=(Surface&&) = delete;

   private:
      HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override;

      ULONG __stdcall AddRef() noexcept override;

      ULONG __stdcall Release() noexcept override;

      [[deprecated("unimplemented")]] HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override
      {
         log_and_terminate(
            "Unimplemented function \"" __FUNCSIG__ "\" called.");
      }

      [[deprecated("unimplemented")]] HRESULT __stdcall SetPrivateData(
         const GUID&, const void*, DWORD, DWORD) noexcept override
      {
         log_and_terminate(
            "Unimplemented function \"" __FUNCSIG__ "\" called.");
      }

      [[deprecated("unimplemented")]] HRESULT __stdcall GetPrivateData(const GUID&,
                                                                       void*,
                                                                       DWORD*) noexcept override
      {
         log_and_terminate(
            "Unimplemented function \"" __FUNCSIG__ "\" called.");
      }

      [[deprecated("unimplemented")]] HRESULT __stdcall FreePrivateData(const GUID&) noexcept override
      {
         log_and_terminate(
            "Unimplemented function \"" __FUNCSIG__ "\" called.");
      }

      [[deprecated("unimplemented")]] DWORD __stdcall SetPriority(DWORD) noexcept override
      {
         log_and_terminate(
            "Unimplemented function \"" __FUNCSIG__ "\" called.");
      }

      [[deprecated("unimplemented")]] DWORD __stdcall GetPriority() noexcept override
      {
         log_and_terminate(
            "Unimplemented function \"" __FUNCSIG__ "\" called.");
      }

      [[deprecated("unimplemented")]] void __stdcall PreLoad() noexcept override
      {
         log_and_terminate(
            "Unimplemented function \"" __FUNCSIG__ "\" called.");
      }

      D3DRESOURCETYPE __stdcall GetType() noexcept override;

      [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetContainer(const IID&,
                                                                             void**) noexcept
      {
         log_and_terminate(
            "Unimplemented function \"" __FUNCSIG__ "\" called.");
      }

      virtual HRESULT __stdcall GetDesc(D3DSURFACE_DESC* desc) noexcept;

      virtual HRESULT __stdcall LockRect(D3DLOCKED_RECT* locked_rect,
                                         const RECT* rect, DWORD flags) noexcept;

      virtual HRESULT __stdcall UnlockRect() noexcept;

      [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetDC(HDC*) noexcept
      {
         log_and_terminate(
            "Unimplemented function \"" __FUNCSIG__ "\" called.");
      }

      [[deprecated("unimplemented")]] virtual HRESULT __stdcall ReleaseDC(HDC) noexcept
      {
         log_and_terminate(
            "Unimplemented function \"" __FUNCSIG__ "\" called.");
      }
   };
};

}
