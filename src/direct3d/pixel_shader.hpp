#pragma once

#include "../logger.hpp"
#include "com_ptr.hpp"

#include <d3d9.h>

namespace sp::d3d9 {

class Pixel_shader final : public IDirect3DPixelShader9 {
public:
   static Com_ptr<Pixel_shader> create() noexcept;

   Pixel_shader(const Pixel_shader&) = delete;
   Pixel_shader& operator=(const Pixel_shader&) = delete;

   Pixel_shader(Pixel_shader&&) = delete;
   Pixel_shader& operator=(Pixel_shader&&) = delete;

   HRESULT
   __stdcall QueryInterface(const IID& iid, void** object) noexcept override;

   ULONG __stdcall AddRef() noexcept override;

   ULONG __stdcall Release() noexcept override;

   [[deprecated("unimplemented")]] HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

   [[deprecated("unimplemented")]] HRESULT __stdcall GetFunction(void*, UINT*) noexcept override
   {
      log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
   }

private:
   Pixel_shader() noexcept = default;
   ~Pixel_shader() = default;

   ULONG _ref_count = 1;
};

}
