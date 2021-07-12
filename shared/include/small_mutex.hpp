#pragma once

#include <atomic>
#include <cassert>

namespace sp {

/// @brief A simple mutex built on top of C++ atomics.
///
/// small_mutex is useful in structures that need a mutex but want as small a memory footprint as possible.
///
/// It should be expected this mutex will be slower than std::mutex on average. Internally small_mutex uses C++20's atomic
/// wait and notify functions to avoid spinning.
class small_mutex {
public:
   small_mutex() = default;

   small_mutex(const small_mutex&) = delete;
   auto operator=(const small_mutex&) -> small_mutex& = delete;

   small_mutex(small_mutex&&) noexcept = delete;
   auto operator=(small_mutex&&) noexcept -> small_mutex& = delete;

   void lock() noexcept
   {
      char expected = 0;

      while (!_value.compare_exchange_strong(expected, 1)) {
         _value.wait(1);
      }
   }

   void unlock() noexcept
   {
      char old_value = _value.exchange(0);

      assert(old_value != 0);

      _value.notify_one();
   }

   bool try_lock() noexcept
   {
      char expected = 0;

      return _value.compare_exchange_strong(expected, 1);
   }

private:
   std::atomic_char _value;

   static_assert(std::atomic_char::is_always_lock_free);
};

}
