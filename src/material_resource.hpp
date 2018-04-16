#pragma once

#include "com_ptr.hpp"
#include "logger.hpp"
#include "magic_number.hpp"
#include "material.hpp"

#include <atomic>

#include <gsl/gsl>

#include <d3d9.h>

namespace sp {

class Material_resource final : public IDirect3DVolumeTexture9 {
public:
   Material_resource(std::uint32_t resource_size,
                     gsl::not_null<Com_ptr<IDirect3DDevice9>> device,
                     const Shader_database& shaders, const Texture_database& textures)
      : _resource_size{resource_size}, _device{std::move(device)}, _shaders{shaders}, _textures{textures}
   {
   }

   Material_resource(const Material_resource&) = delete;
   Material_resource& operator=(const Material_resource&) = delete;

   Material_resource(Material_resource&&) = delete;
   Material_resource& operator=(Material_resource&&) = delete;

   HRESULT __stdcall LockBox(UINT level, D3DLOCKED_BOX* locked_volume,
                             const D3DBOX* box, DWORD) noexcept override
   {
      if (level != 0) return D3DERR_INVALIDCALL;
      if (locked_volume == nullptr) return D3DERR_INVALIDCALL;
      if (box != nullptr) return D3DERR_INVALIDCALL;
      if (_data) return D3DERR_INVALIDCALL;

      _data = std::make_unique<std::byte[]>(_resource_size);

      locked_volume->pBits = _data.get();
      locked_volume->RowPitch = -1;
      locked_volume->SlicePitch = -1;

      return S_OK;
   }

   HRESULT __stdcall UnlockBox(UINT level) noexcept override
   {
      if (level != 0) return D3DERR_INVALIDCALL;
      if (!_data) return D3DERR_INVALIDCALL;

      try {
         _material.emplace(read_patch_material(ucfb::Reader{
                              gsl::make_span(_data.get(), _resource_size)}),
                           _device, _shaders, _textures);
      }
      catch (std::exception& e) {
         log_and_terminate("Failed to read material in from game. Exception message: "sv,
                           e.what());
      }

      _data.reset();

      return S_OK;
   }

   HRESULT __stdcall QueryInterface(const IID& iid, void** obj) noexcept override
   {
      if (iid == IID_IDirect3DVolumeTexture9) {
         *obj = static_cast<IDirect3DVolumeTexture9*>(this);
      }
      else if (iid == IID_IDirect3DBaseTexture9) {
         *obj = static_cast<IDirect3DBaseTexture9*>(this);
      }
      else if (iid == IID_IDirect3DResource9) {
         *obj = static_cast<IDirect3DResource9*>(this);
      }
      else if (iid == IID_IUnknown) {
         *obj = static_cast<IUnknown*>(this);
      }
      else {
         return E_NOINTERFACE;
      }

      AddRef();

      return S_OK;
   }

   ULONG __stdcall AddRef() noexcept override
   {
      return _ref_count.fetch_add(1);
   }

   ULONG __stdcall Release() noexcept override
   {
      const auto ref_count = _ref_count.fetch_sub(1);

      if (ref_count == 0) delete this;

      return ref_count;
   }

   HRESULT __stdcall GetDevice(IDirect3DDevice9**) noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return E_NOTIMPL;
   }

   HRESULT __stdcall SetPrivateData(const GUID&, const void*, DWORD, DWORD) noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return E_NOTIMPL;
   }

   HRESULT __stdcall GetPrivateData(const GUID&, void*, DWORD*) noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return E_NOTIMPL;
   }

   HRESULT __stdcall FreePrivateData(const GUID&) noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return E_NOTIMPL;
   }

   DWORD __stdcall SetPriority(DWORD) noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return 0;
   }

   DWORD __stdcall GetPriority() noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return 0;
   }

   void __stdcall PreLoad() noexcept override {}

   D3DRESOURCETYPE __stdcall GetType() noexcept override
   {
      return id;
   }

   DWORD __stdcall SetLOD(DWORD) noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return 0;
   }

   DWORD __stdcall GetLOD() noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return 0;
   }

   DWORD __stdcall GetLevelCount() noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return 1;
   }

   HRESULT __stdcall SetAutoGenFilterType(D3DTEXTUREFILTERTYPE) noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return S_OK;
   }

   D3DTEXTUREFILTERTYPE __stdcall GetAutoGenFilterType() noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return D3DTEXF_NONE;
   }

   void __stdcall GenerateMipSubLevels() noexcept override {}

   HRESULT __stdcall GetLevelDesc(UINT, D3DVOLUME_DESC*) noexcept override
   {
      return E_NOTIMPL;
   }

   HRESULT __stdcall GetVolumeLevel(UINT, IDirect3DVolume9**) noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return E_NOTIMPL;
   }

   HRESULT __stdcall AddDirtyBox(const D3DBOX*) noexcept override
   {
      log(Log_level::warning,
          "Unimplemented function \"" __FUNCSIG__ "\" called.");

      return E_NOTIMPL;
   }

   auto get_material() const noexcept -> Material
   {
      if (!_material) {
         log_and_terminate("Material bound before being uploaded."sv);
      }

      return *_material;
   }

   constexpr static auto id = static_cast<D3DRESOURCETYPE>("mtrl"_mn);

private:
   ~Material_resource() = default;

   const std::uint32_t _resource_size;

   const Com_ptr<IDirect3DDevice9> _device;
   const Shader_database& _shaders;
   const Texture_database& _textures;

   std::optional<Material> _material;

   std::unique_ptr<std::byte[]> _data;
   std::atomic<ULONG> _ref_count{1};
};
}
