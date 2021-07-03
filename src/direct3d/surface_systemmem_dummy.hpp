#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "com_ptr.hpp"

#include <memory>

#include <d3d9.h>

namespace sp::d3d9 {

struct Surface_systemmem_dummy final : public IDirect3DSurface9 {
public:
   constexpr static auto dummy_format = D3DFMT_A8R8G8B8;

   static Com_ptr<Surface_systemmem_dummy> create(const UINT width,
                                                  const UINT height) noexcept;

   Surface_systemmem_dummy(const Surface_systemmem_dummy&) = delete;
   Surface_systemmem_dummy& operator=(const Surface_systemmem_dummy&) = delete;

   Surface_systemmem_dummy(Surface_systemmem_dummy&&) = delete;
   Surface_systemmem_dummy& operator=(Surface_systemmem_dummy&&) = delete;

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

   virtual HRESULT __stdcall LockRect(D3DLOCKED_RECT* locked_rect,
                                      const RECT* rect, DWORD flags) noexcept;

   virtual HRESULT __stdcall UnlockRect() noexcept;

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall GetDC(HDC*) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] virtual HRESULT __stdcall ReleaseDC(HDC) noexcept
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

private:
   Surface_systemmem_dummy(const UINT width, const UINT height) noexcept;

   ~Surface_systemmem_dummy() = default;

   const UINT _width;
   const UINT _height;
   std::unique_ptr<std::byte[]> _dummy_buffer;

   ULONG _ref_count = 1;
};

}
