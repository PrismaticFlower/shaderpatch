#pragma once

#include "resource.hpp"

#include <variant>

namespace sp::d3d9 {

enum class Texture_type { texture2d, texture3d, texturecube, rendertarget };

class Base_texture : public Resource {
public:
   virtual HRESULT __stdcall QueryInterface(const IID& iid, void** obj) noexcept = 0;
   virtual ULONG __stdcall AddRef() noexcept = 0;
   virtual ULONG __stdcall Release() noexcept = 0;
   virtual HRESULT __stdcall GetDevice(IDirect3DDevice9** device) noexcept = 0;
   virtual HRESULT __stdcall SetPrivateData(const GUID& guid, const void* data,
                                            DWORD data_size, DWORD flags) noexcept = 0;
   virtual HRESULT __stdcall GetPrivateData(const GUID& guid, void* data,
                                            DWORD* data_size) noexcept = 0;
   virtual HRESULT __stdcall FreePrivateData(const GUID& guid) noexcept = 0;
   virtual DWORD __stdcall SetPriority(DWORD priority) noexcept = 0;
   virtual DWORD __stdcall GetPriority() noexcept = 0;
   virtual void __stdcall PreLoad() noexcept = 0;
   virtual D3DRESOURCETYPE __stdcall GetType() noexcept = 0;
   virtual DWORD __stdcall SetLOD(DWORD lod) noexcept = 0;
   virtual DWORD __stdcall GetLOD() noexcept = 0;
   virtual DWORD __stdcall GetLevelCount() noexcept = 0;
   virtual HRESULT __stdcall SetAutoGenFilterType(D3DTEXTUREFILTERTYPE filter) noexcept = 0;
   virtual D3DTEXTUREFILTERTYPE __stdcall GetAutoGenFilterType() noexcept = 0;
   virtual void __stdcall GenerateMipSubLevels() noexcept = 0;

   Base_texture(const Texture_type texture_type) noexcept
      : texture_type{texture_type}
   {
   }

   Base_texture(const Base_texture&) = delete;
   Base_texture& operator=(const Base_texture&) = delete;

   Base_texture(Base_texture&&) = delete;
   Base_texture& operator=(Base_texture&&) = delete;

   ~Base_texture() = default;

   const Texture_type texture_type;
};

static_assert(
   !std::has_virtual_destructor_v<Base_texture>,
   "Base_texture must not have a virtual destructor as it will cause "
   "an ABI break.");

}
