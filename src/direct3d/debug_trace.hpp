#pragma once

#include <mutex>
#include <vector>

namespace sp::d3d9 {

class Debug_trace {
public:
   Debug_trace() = delete;
   ~Debug_trace() = delete;

   static void reset() noexcept
   {
      std::lock_guard lock{_mutex};

      _trace.clear();
   }

   static void func(const char* const funcsig) noexcept
   {
#ifndef NDEBUG
      std::lock_guard lock{_mutex};

      _trace.emplace_back(funcsig);
#else
      (void)funcsig;
#endif
   }

   static auto get() noexcept -> const std::vector<const char*>&
   {
      return _trace;
   }

private:
   inline static std::vector<const char*> _trace = [] {
      std::vector<const char*> vec;
      vec.reserve(8192);

      return vec;
   }();
   inline static std::mutex _mutex;
};

}
