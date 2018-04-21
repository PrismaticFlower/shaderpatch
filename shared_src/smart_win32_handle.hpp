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
}
