#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "base_texture.hpp"

#include <d3d9.h>

namespace sp::d3d9 {

class Texture2d_rendertarget final : public Base_texture {
public:
   static constexpr auto reported_format = D3DFMT_A8R8G8B8;

   static Com_ptr<Texture2d_rendertarget> create(core::Shader_patch& shader_patch,
                                                 const UINT actual_width,
                                                 const UINT actual_height,
                                                 const UINT perceived_width,
                                                 const UINT perceived_height) noexcept;

   Texture2d_rendertarget(const Texture2d_rendertarget&) = delete;
   Texture2d_rendertarget& operator=(const Texture2d_rendertarget&) = delete;

   Texture2d_rendertarget(Texture2d_rendertarget&&) = delete;
   Texture2d_rendertarget& operator=(Texture2d_rendertarget&&) = delete;

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override;

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

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall LockRect(
      UINT, D3DLOCKED_RECT*, const RECT*, DWORD) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall UnlockRect(UINT) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall AddDirtyRect(const RECT*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

private:
   Texture2d_rendertarget(core::Shader_patch& shader_patch, const UINT actual_width,
                          const UINT actual_height, const UINT perceived_width,
                          const UINT perceived_height) noexcept;

   ~Texture2d_rendertarget() = default;

   struct Surface final : public Resource {
      Texture2d_rendertarget& owning_texture;

      Surface(Texture2d_rendertarget& owning_texture) noexcept;

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

      virtual HRESULT __stdcall GetDesc(D3DSURFACE_DESC*) noexcept;

      [[deprecated("unimplemented")]] virtual HRESULT __stdcall LockRect(
         D3DLOCKED_RECT*, const RECT*, DWORD) noexcept
      {
         log_and_terminate(
            "Unimplemented function \"" __FUNCSIG__ "\" called.");
      }

      [[deprecated(
         "unimplemented")]] virtual HRESULT __stdcall UnlockRect() noexcept
      {
         log_and_terminate(
            "Unimplemented function \"" __FUNCSIG__ "\" called.");
      }

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

   struct Rendertarget_id {
      const core::Game_rendertarget_id id;
      core::Shader_patch& shader_patch;

      ~Rendertarget_id();
   };

   const Rendertarget_id _rendertarget_id;
   Surface _surface{*this};

   const UINT _perceived_width;
   const UINT _perceived_height;
   UINT _ref_count = 1;
};

}
