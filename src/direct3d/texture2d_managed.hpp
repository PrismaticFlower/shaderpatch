#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "base_texture.hpp"
#include "com_ptr.hpp"
#include "image_patcher.hpp"
#include "upload_texture.hpp"

#include <memory>
#include <vector>

#include <d3d9.h>

namespace sp::d3d9 {

class Texture2d_managed final : public Base_texture {
public:
   static Com_ptr<Texture2d_managed> create(
      core::Shader_patch& shader_patch, const UINT width, const UINT height,
      const UINT mip_levels, const DXGI_FORMAT format, const D3DFORMAT reported_format,
      std::unique_ptr<Image_patcher> image_patcher = nullptr) noexcept;

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

private:
   struct Surface;

   Texture2d_managed(core::Shader_patch& shader_patch, const UINT width,
                     const UINT height, const UINT mip_levels,
                     const DXGI_FORMAT format, const D3DFORMAT reported_format,
                     std::unique_ptr<Image_patcher> image_patcher) noexcept;

   ~Texture2d_managed() = default;

   std::unique_ptr<Image_patcher> _image_patcher;
   std::unique_ptr<Upload_texture> _upload_texture;
   core::Shader_patch& _shader_patch;

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

   struct Surface final : public Resource {
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

      [[deprecated(
         "unimplemented")]] DWORD __stdcall GetPriority() noexcept override
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

   static_assert(!std::has_virtual_destructor_v<Surface>,
                 "Texture2d_managed::Surface must not have a virtual "
                 "destructor as it will cause "
                 "an ABI break.");
};

static_assert(
   !std::has_virtual_destructor_v<Texture2d_managed>,
   "Texture2d_managed must not have a virtual destructor as it will cause "
   "an ABI break.");

}
