#pragma once

#include <cassert>
#include <cstdint>

namespace sp::core {

inline constexpr std::uintptr_t null_handle = 0;

using Handle_type = std::uintptr_t;

using Buffer_handle = Handle_type;

using Texture_handle = Handle_type;

using Material_handle = Handle_type;

using Patch_effects_config_handle = Handle_type;

inline auto ptr_to_handle(void* const ptr) noexcept -> Handle_type
{
   return ptr ? reinterpret_cast<std::uintptr_t>(ptr) : 0;
}

template<typename T>
inline auto handle_to_ptr(const Handle_type handle) -> T*
{
   if (handle == null_handle) return nullptr;

   return static_cast<T*>(reinterpret_cast<void*>(handle));
}

}
