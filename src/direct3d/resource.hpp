#pragma once

#include "../core/shader_patch.hpp"

#include <variant>

#include <d3d11_1.h>
#include <d3d9.h>

namespace sp::d3d9 {

class Resource : public IUnknown {
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

   Resource() noexcept = default;

   Resource(const Resource&) = delete;
   Resource& operator=(const Resource&) = delete;

   Resource(Resource&&) = delete;
   Resource& operator=(Resource&&) = delete;

   ~Resource() = default;

   using Resource_variant =
      std::variant<std::monostate, core::Game_texture, core::Game_rendertarget_id,
                   ID3D11Buffer*, core::Game_depthstencil, core::Texture_handle,
                   core::Material_handle, core::Patch_effects_config_handle>;

   template<typename Type>
   const Type* get_if() const noexcept
   {
      return std::get_if<Type>(&resource);
   }

   template<typename Type>
   const Type& get() const noexcept
   {
      return std::get<Type>(resource);
   }

   template<typename... Visitors>
   void visit(Visitors&&... visitors) const noexcept
   {
      visit_impl(std::forward<Visitors>(visitors)..., [](auto&&) {});
   }

protected:
   Resource_variant resource;

private:
   template<typename... Visitors>
   void visit_impl(Visitors&&... visitors) const noexcept
   {
      struct Visitor_set : Visitors... {
         using Visitors::operator()...;
      };

      std::visit(Visitor_set{std::forward<Visitors>(visitors)...}, resource);
   }
};

static_assert(!std::has_virtual_destructor_v<Resource>,
              "Resource must not have a virtual destructor as it will cause "
              "an ABI break.");

}
