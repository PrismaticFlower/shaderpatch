#pragma once

#include <cstddef>

#include <gsl/gsl>

#include <Windows.h>

namespace sp {

[[nodiscard]] inline auto unlock_memory(void* address, std::size_t size)
{
   DWORD old_protect{};

   VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &old_protect);

   return gsl::finally([=] {
      DWORD old_new_protect{};
      VirtualProtect(address, size, old_protect, &old_new_protect);
   });
}

}
