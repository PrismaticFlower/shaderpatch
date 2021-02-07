#pragma once

#include "framework.hpp"

template<typename T, typename Traits = std::char_traits<T>>
constexpr bool begins_with(
   std::basic_string_view<T, Traits> string,
   typename std::common_type<std::basic_string_view<T, Traits>>::type what) noexcept
{
   if (what.size() > string.size()) return false;

   return (string.substr(0, what.size()) == what);
}
