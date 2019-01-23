#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iosfwd>

namespace sp {

enum class Magic_number : std::uint32_t {};

constexpr Magic_number create_magic_number(const char c_0, const char c_1,
                                           const char c_2, const char c_3)
{
   std::uint32_t result = 0;

   result |= (static_cast<std::uint8_t>(c_0) << 0);
   result |= (static_cast<std::uint8_t>(c_1) << 8);
   result |= (static_cast<std::uint8_t>(c_2) << 16);
   result |= (static_cast<std::uint8_t>(c_3) << 24);

   return static_cast<Magic_number>(result);
}

constexpr Magic_number create_magic_number(const std::array<char, 4> chars)
{
   return create_magic_number(chars[0], chars[1], chars[2], chars[3]);
}

constexpr Magic_number operator""_mn(const char* chars, const std::size_t) noexcept
{
   return create_magic_number(chars[0], chars[1], chars[2], chars[3]);
}
}
