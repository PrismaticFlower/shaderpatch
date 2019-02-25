#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "base_texture.hpp"
#include "com_ptr.hpp"
#include "utility.hpp"

#include <memory>

#include <d3d9.h>

#include <gsl/gsl>

namespace sp::d3d9 {

class Texture3d_resource final : public Base_texture {
public:
   static Com_ptr<Texture3d_resource> create(core::Shader_patch& patch,
                                             const UINT width, const UINT height,
                                             const UINT depth) noexcept;

   Texture3d_resource(const Texture3d_resource&) = delete;
   Texture3d_resource& operator=(const Texture3d_resource&) = delete;

   Texture3d_resource(Texture3d_resource&&) = delete;
   Texture3d_resource& operator=(Texture3d_resource&&) = delete;

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
   Texture3d_resource(core::Shader_patch& patch, const UINT width,
                      const UINT height, const UINT depth) noexcept;

   ~Texture3d_resource() = default;

   void create_resource() noexcept;

   inline static Aligned_scratch_buffer<16> _scratch_buffer;
   constexpr static auto max_persist_buffer_size = 4194304;

   core::Shader_patch& _patch;

   bool _locked = false;
   const UINT _width;
   const UINT _height;
   const UINT _depth;
   const UINT _resource_size{_width * _height * _depth};

   ULONG _ref_count = 1;
};

}
