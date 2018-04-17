#pragma once

#include <array>
#include <string_view>
#include <type_traits>

namespace sp {

template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr auto split_string(
   std::basic_string_view<Char_t, Char_triats> string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type delimiter) noexcept
   -> std::array<std::basic_string_view<Char_t, Char_triats>, 2>
{
   const auto offset = string.find_first_of(delimiter);

   if (offset == string.npos) return {string, std::string_view{""}};

   std::array<std::basic_string_view<Char_t, Char_triats>, 2> split_strings;

   split_strings[0] = string.substr(0, offset);
   split_strings[1] = string.substr(offset + delimiter.length(),
                                    string.length() - offset - delimiter.length());

   return split_strings;
}

template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
constexpr auto split_string(
   const Char_t* string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type delimiter) noexcept
   -> std::array<std::basic_string_view<Char_t, Char_triats>, 2>
{
   return split_string(std::basic_string_view<Char_t, Char_triats>{string}, delimiter);
}

template<typename Char_t, typename Char_triats = std::char_traits<Char_t>>
inline auto split_string(
   const std::basic_string<Char_t, Char_triats>& string,
   typename std::common_type<std::basic_string_view<Char_t, Char_triats>>::type delimiter) noexcept
   -> std::array<std::basic_string_view<Char_t, Char_triats>, 2>
{
   return split_string(std::basic_string_view<Char_t, Char_triats>{string}, delimiter);
}
}
