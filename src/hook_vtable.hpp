#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <Windows.h>

namespace sp {

template<typename Function, typename Class>
auto hook_vtable(Class& object, const std::size_t index, Function* function) noexcept
   -> Function*
{
   static_assert(std::is_polymorphic_v<Class>, "Class must be polymorphic!");
   static_assert(sizeof(void*) == sizeof(std::uintptr_t));

   auto* const vtable = reinterpret_cast<std::uintptr_t*&>(object);

   const auto original_function = vtable[index];

   DWORD old_protect{};

   VirtualProtect(&vtable[index], sizeof(std::uintptr_t),
                  PAGE_EXECUTE_READWRITE, &old_protect);

   vtable[index] = reinterpret_cast<std::uintptr_t>(function);

   VirtualProtect(&vtable[index], sizeof(std::uintptr_t), old_protect, nullptr);

   return reinterpret_cast<Function*>(original_function);
}
}
