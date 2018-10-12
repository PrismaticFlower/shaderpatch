#pragma once

#include "unlock_memory.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <Windows.h>

namespace sp {

template<typename Class>
inline auto get_vtable_pointer(const Class& object) -> const std::uintptr_t*
{
   return reinterpret_cast<const std::uintptr_t* const&>(object);
}

template<typename Function, typename Class>
inline auto hook_vtable(Class& object, const std::size_t index,
                        Function* function) noexcept -> Function*
{
   static_assert(std::is_polymorphic_v<Class>, "Class must be polymorphic!");
   static_assert(sizeof(void*) == sizeof(std::uintptr_t));

   auto* const vtable = reinterpret_cast<std::uintptr_t*&>(object);

   const auto original_function = vtable[index];

   auto memory_lock = unlock_memory(&vtable[index], sizeof(std::uintptr_t));

   vtable[index] = reinterpret_cast<std::uintptr_t>(function);

   return reinterpret_cast<Function*>(original_function);
}
}
