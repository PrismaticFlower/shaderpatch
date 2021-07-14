#pragma once

#include <cassert>
#include <cstdint>

namespace sp::core {

using Handle_type = std::uintptr_t;

enum class Buffer_handle : Handle_type {};

enum class Game_texture_handle : Handle_type {};

enum class Patch_texture_handle : Handle_type {};

enum class Material_handle : Handle_type {};

enum class Patch_effects_config_handle : Handle_type {};

struct Null_handle_t {
   operator Buffer_handle() const noexcept
   {
      return Buffer_handle{0};
   }

   operator Game_texture_handle() const noexcept
   {
      return Game_texture_handle{0};
   }

   operator Patch_texture_handle() const noexcept
   {
      return Patch_texture_handle{0};
   }

   operator Material_handle() const noexcept
   {
      return Material_handle{0};
   }

   operator Patch_effects_config_handle() const noexcept
   {
      return Patch_effects_config_handle{0};
   }
};

inline constexpr Null_handle_t null_handle;

template<typename T>
inline auto ptr_to_handle(void* const ptr) noexcept -> T
{
   return T{ptr ? reinterpret_cast<std::uintptr_t>(ptr) : 0};
}

template<typename T, typename H>
inline auto handle_to_ptr(const H handle) -> T*
{
   if (handle == H{0}) return nullptr;

   return static_cast<T*>(reinterpret_cast<void*>(handle));
}

}
