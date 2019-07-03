#pragma once

#include <cstdint>
#include <limits>
#include <random>

namespace sp {

class xor_shift32 {
public:
   using result_type = std::uint32_t;

   constexpr xor_shift32(){};
   constexpr xor_shift32(const result_type seed) : v{seed} {};

   constexpr static auto min() noexcept -> std::uint32_t
   {
      return std::numeric_limits<std::uint32_t>::min();
   }

   constexpr static auto max() noexcept -> std::uint32_t
   {
      return std::numeric_limits<std::uint32_t>::max();
   }

   constexpr auto operator()() noexcept -> std::uint32_t
   {
      v ^= v << 13u;
      v ^= v >> 17u;
      v ^= v << 5u;

      return v;
   }

   constexpr void discard(const unsigned long long count) noexcept
   {
      for (auto i = 0ull; i < count; ++i) (*this)();
   }

private:
   std::uint32_t v = 551518763u;
};
}
