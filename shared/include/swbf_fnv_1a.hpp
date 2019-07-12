#pragma once

#include <cstdint>
#include <string_view>

namespace sp {

constexpr std::uint32_t fnv_1a_hash(const std::string_view str)
{
   constexpr std::uint32_t FNV_prime = 16777619;
   constexpr std::uint32_t offset_basis = 2166136261;

   std::uint32_t hash = offset_basis;

   for (auto c : str) {
      c |= 0x20;

      hash ^= c;
      hash *= FNV_prime;
   }

   return hash;
}

constexpr auto operator""_fnv(const char* const chars, const std::size_t size) noexcept
{
   return fnv_1a_hash({chars, size});
}

}
