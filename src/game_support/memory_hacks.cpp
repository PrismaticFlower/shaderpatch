#include "memory_hacks.hpp"

#include <Windows.h>

#include "../imgui/imgui.h"

namespace sp::game_support {

namespace {

static float* aspect_ratio = nullptr;

int exception_filter(unsigned int code, [[maybe_unused]] EXCEPTION_POINTERS* ep)
{
   return code == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER
                                             : EXCEPTION_CONTINUE_SEARCH;
}

}

void set_aspect_ratio_ptr(float* aspect_ratio_ptr) noexcept
{
   aspect_ratio = aspect_ratio_ptr;
}

void set_aspect_ratio(float new_aspect_ratio) noexcept
{
   if (aspect_ratio == nullptr) return;

   __try {
      if (*aspect_ratio <= 0.0f || *aspect_ratio > 1.0f) return;

      *aspect_ratio = new_aspect_ratio;
   }
   __except (exception_filter(GetExceptionCode(), GetExceptionInformation())) {
      return;
   }
}

[[nodiscard]] auto get_aspect_ratio() noexcept -> float
{
   if (aspect_ratio == nullptr) return 0.0f;

   __try {
      return *aspect_ratio;
   }
   __except (exception_filter(GetExceptionCode(), GetExceptionInformation())) {
      return 0.75f;
   }
}

}