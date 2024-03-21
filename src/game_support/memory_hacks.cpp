#include "memory_hacks.hpp"

#include <Windows.h>

#include <math.h>

namespace sp::game_support {

namespace {

constexpr int aspect_ratio_search_radius = 1024;
constexpr float aspect_ratio_search_eps = 0.0001f;

static float* aspect_ratio_search_start = nullptr;
static float* aspect_ratio = nullptr;

int exception_filter(unsigned int code, [[maybe_unused]] EXCEPTION_POINTERS* ep)
{
   return code == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER
                                             : EXCEPTION_CONTINUE_SEARCH;
}

#pragma intrinsic(fabs)

bool equals(const float l, const float r) noexcept
{
   return fabs(l - r) <= aspect_ratio_search_eps;
}

}

void set_aspect_ratio_search_ptr(float* aspect_ratio_search_ptr) noexcept
{
   aspect_ratio_search_start = aspect_ratio_search_ptr;
   aspect_ratio = nullptr;
}

void find_aspect_ratio(float expected_aspect_ratio)
{
   if (aspect_ratio != nullptr) return;
   if (aspect_ratio_search_start == nullptr) return;

   for (int i = 0; i >= -aspect_ratio_search_radius; --i) {
      __try {
         if (equals(aspect_ratio_search_start[i], expected_aspect_ratio)) {
            aspect_ratio = &aspect_ratio_search_start[i];

            return;
         }
      }
      __except (exception_filter(GetExceptionCode(), GetExceptionInformation())) {
         break;
      }
   }

   for (int i = 0; i <= aspect_ratio_search_radius; ++i) {
      __try {
         if (equals(aspect_ratio_search_start[i], expected_aspect_ratio)) {
            aspect_ratio = &aspect_ratio_search_start[i];

            return;
         }
      }
      __except (exception_filter(GetExceptionCode(), GetExceptionInformation())) {
         break;
      }
   }
}

void set_aspect_ratio(float new_aspect_ratio) noexcept
{
   if (aspect_ratio == nullptr) return;

   __try {
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