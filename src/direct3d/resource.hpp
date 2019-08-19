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
      visit_impl(std::forward<Visitors>(visitors)...);
   }

protected:
   Resource_variant resource;

private:
   template<typename... Visitors>
   void visit_impl(Visitors&&... visitors) const noexcept
   {
      // Nice simple version.
      // struct Visitor_set : Visitors... {
      //    using Visitors::operator()...;
      // };
      //
      // std::visit(Visitor_set{std::forward<Visitors>(visitors)..., [](auto&&) {}}, resource);

      // Recursive version to work around VS 16.2 compiler bugs.
      visit_select_impl<0>(std::forward<Visitors>(visitors)...);
   }

   template<const std::size_t index, typename... Funcs>
   void visit_select_impl(Funcs&&... funcs) const noexcept
   {
      if (resource.index() == index) {
         visit_call_impl(std::get<index>(resource), std::forward<Funcs>(funcs)...);
      }
      else if constexpr ((index + 1) < std::variant_size_v<Resource_variant>) {
         visit_select_impl<index + 1>(std::forward<Funcs>(funcs)...);
      }
   }

   template<typename Type, typename Func, typename... Rest_funcs>
   void visit_call_impl([[maybe_unused]] Type&& type, [[maybe_unused]] Func&& func,
                        [[maybe_unused]] Rest_funcs&&... rest_funcs) const noexcept
   {
      if constexpr (std::is_invocable_v<Func, Type>) {
         std::invoke(std::forward<Func>(func), std::forward<Type>(type));

         // Apparently [[maybe_unused]] doesn't work so well on paramter packs in VS 16.2
         // so we have to use a lambda to silence the warning instead.
         [](auto&&...) {}(std::forward<Rest_funcs>(rest_funcs)...);
      }
      else if constexpr (sizeof...(Rest_funcs) > 0) {
         visit_call_impl(std::forward<Type>(type),
                         std::forward<Rest_funcs>(rest_funcs)...);
      }
   }
};

static_assert(!std::has_virtual_destructor_v<Resource>,
              "Resource must not have a virtual destructor as it will cause "
              "an ABI break.");

}
