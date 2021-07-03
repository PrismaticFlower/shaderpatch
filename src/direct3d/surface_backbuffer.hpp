#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "resource_access.hpp"

#include <d3d9.h>

namespace sp::d3d9 {

class Surface_backbuffer final : public IDirect3DSurface9, public Rendertarget_accessor {
public:
   static constexpr auto reported_format = D3DFMT_A8R8G8B8;

   static Com_ptr<Surface_backbuffer> create(const core::Game_rendertarget_id rendertarget_id,
                                             const UINT width,
                                             const UINT height) noexcept;

   Surface_backbuffer(const Surface_backbuffer&) = delete;
   Surface_backbuffer& operator=(const Surface_backbuffer&) = delete;

   Surface_backbuffer(Surface_backbuffer&&) = delete;
   Surface_backbuffer& operator=(Surface_backbuffer&&) = delete;

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

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetContainer(const IID&,
                                                                          void**) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   virtual HRESULT __stdcall GetDesc(D3DSURFACE_DESC* desc) noexcept;

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall LockRect(D3DLOCKED_RECT*,
                                                                      const RECT*,
                                                                      DWORD) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated(
      "unimplemented")]] virtual HRESULT __stdcall UnlockRect() noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetDC(HDC*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall ReleaseDC(HDC) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   auto rendertarget() const noexcept -> core::Game_rendertarget_id override;

private:
   Surface_backbuffer(const core::Game_rendertarget_id rendertarget_id,
                      const UINT width, const UINT height) noexcept;

   ~Surface_backbuffer() = default;

   const core::Game_rendertarget_id _rendertarget_id;
   const UINT _width;
   const UINT _height;
   ULONG _ref_count = 1;
};

}
