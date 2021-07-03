#pragma once

#include "../core/shader_patch.hpp"

#include <Windows.h>

namespace sp::d3d9 {

enum class Texture_accessor_type {
   texture,
   texture_rendertarget,
   material,
   none
};

enum class Texture_accessor_dimension { _2d, _3d, cube, nonapplicable };

/// @brief Interface for resources that bind to texture slots.
class Texture_accessor : public IUnknown {
public:
   /// @brief Gets the type of the bindable.
   /// @return The type of the bindable.
   virtual auto type() const noexcept -> Texture_accessor_type = 0;

   /// @brief Gets the dimension of the bindable.
   /// @return The dimension of the bindable.
   virtual auto dimension() const noexcept -> Texture_accessor_dimension = 0;

   /// @brief Gets the texture for binding.
   /// @return The texture or core::nullgametex.
   virtual auto texture() const noexcept -> core::Game_texture
   {
      return core::nullgametex;
   }

   /// @brief Gets the rendertarget for binding.
   /// @return The rendertarget ID or core::Shader_patch::get_back_buffer().
   virtual auto texture_rendertarget() const noexcept -> core::Game_rendertarget_id
   {
      return core::Shader_patch::get_back_buffer();
   }

   /// @brief Gets the material for binding.
   /// @return The material or nullptr.
   virtual auto material() const noexcept -> material::Material*
   {
      return nullptr;
   }
};

// {BA94F900-CD55-4602-9732-2F9392256F9E}
static constexpr GUID IID_Texture_accessor =
   {0xba94f900, 0xcd55, 0x4602, {0x97, 0x32, 0x2f, 0x93, 0x92, 0x25, 0x6f, 0x9e}};

/// @brief Interface for resources that binds as a rendertarget.
class Rendertarget_accessor : public IUnknown {
public:
   /// @brief Gets the rendertarget for binding.
   /// @return The rendertarget ID.
   virtual auto rendertarget() const noexcept -> core::Game_rendertarget_id = 0;
};

// {E0282818-1276-4BED-AB8E-28F2A0EDA8AF}
static constexpr GUID IID_Rendertarget_accessor =
   {0xe0282818, 0x1276, 0x4bed, {0xab, 0x8e, 0x28, 0xf2, 0xa0, 0xed, 0xa8, 0xaf}};

/// @brief Interface for resources that binds as a depthstencil surface.
class Depthstencil_accessor : public IUnknown {
public:
   /// @brief Gets the depthstencil for binding.
   /// @return The depthstencil.
   virtual auto depthstencil() const noexcept -> core::Game_depthstencil = 0;
};

// {F8621F21-5A75-4105-AE19-154C1A1EA886}
static constexpr GUID IID_Depthstencil_accessor =
   {0xf8621f21, 0x5a75, 0x4105, {0xae, 0x19, 0x15, 0x4c, 0x1a, 0x1e, 0xa8, 0x86}};

}
