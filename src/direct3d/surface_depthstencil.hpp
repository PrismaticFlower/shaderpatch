#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "resource.hpp"

#include <d3d9.h>

namespace sp::d3d9 {

class Surface_depthstencil final : public Resource {
public:
   static constexpr auto reported_format = D3DFMT_D24S8;

   static Com_ptr<Surface_depthstencil> create(const core::Game_depthstencil depthstencil,
                                               const UINT width,
                                               const UINT height) noexcept;

   Surface_depthstencil(const Surface_depthstencil&) = delete;
   Surface_depthstencil& operator=(const Surface_depthstencil&) = delete;

   Surface_depthstencil(Surface_depthstencil&&) = delete;
   Surface_depthstencil& operator=(Surface_depthstencil&&) = delete;

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

private:
   Surface_depthstencil(const core::Game_depthstencil depthstencil,
                        const UINT width, const UINT height) noexcept;

   ~Surface_depthstencil() = default;

   const UINT _width;
   const UINT _height;
   ULONG _ref_count = 1;
};

static_assert(
   !std::has_virtual_destructor_v<Surface_depthstencil>,
   "Surface_depthstencil must not have a virtual destructor as it will cause "
   "an ABI break.");

}
