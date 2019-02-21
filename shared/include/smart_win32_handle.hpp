#pragma once

#include <memory>

#include <Windows.h>

namespace sp::win32 {

struct Handle_deleter {
   void operator()(HANDLE handle) noexcept
   {
      if (handle == INVALID_HANDLE_VALUE || handle == nullptr) return;

      CloseHandle(handle);
   }
};

using Unique_handle = std::unique_ptr<void, Handle_deleter>;

inline auto duplicate_handle(const Unique_handle& other) noexcept -> Unique_handle
{
   if (other.get() == INVALID_HANDLE_VALUE || other.get() == nullptr)
      return Unique_handle{INVALID_HANDLE_VALUE};

   HANDLE duplicated;

   if (!DuplicateHandle(GetCurrentProcess(), other.get(), GetCurrentProcess(),
                        &duplicated, DUPLICATE_SAME_ACCESS, true, 0x0))
      std::terminate();

   return Unique_handle{duplicated};
}

}
