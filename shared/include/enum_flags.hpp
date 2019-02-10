#pragma once

#include <type_traits>

namespace sp {

template<typename Enum>
struct is_enum_flag : std::false_type {
};

template<typename Enum>
constexpr bool is_enum_flag_v = is_enum_flag<Enum>::value;

template<typename Enum, typename = std::enable_if_t<is_enum_flag_v<Enum>>>
constexpr Enum operator|(const Enum l, const Enum r) noexcept
{
   return Enum{static_cast<std::underlying_type_t<Enum>>(l) |
               static_cast<std::underlying_type_t<Enum>>(r)};
}

template<typename Enum, typename = std::enable_if_t<is_enum_flag_v<Enum>>>
constexpr Enum operator&(Enum l, Enum r) noexcept
{
   return Enum{static_cast<std::underlying_type_t<Enum>>(l) &
               static_cast<std::underlying_type_t<Enum>>(r)};
}

template<typename Enum, typename = std::enable_if_t<is_enum_flag_v<Enum>>>
constexpr Enum operator^(const Enum l, const Enum r) noexcept
{
   return Enum{static_cast<std::underlying_type_t<Enum>>(l) ^
               static_cast<std::underlying_type_t<Enum>>(r)};
}

template<typename Enum, typename = std::enable_if_t<is_enum_flag_v<Enum>>>
constexpr Enum operator~(const Enum f) noexcept
{
   return Enum{~static_cast<std::underlying_type_t<Enum>>(f)};
}

template<typename Enum, typename = std::enable_if_t<is_enum_flag_v<Enum>>>
constexpr Enum& operator|=(Enum& l, const Enum r) noexcept
{
   return l = l | r;
}

template<typename Enum, typename = std::enable_if_t<is_enum_flag_v<Enum>>>
constexpr Enum& operator&=(Enum& l, const Enum r) noexcept
{
   return l = l & r;
}

template<typename Enum, typename = std::enable_if_t<is_enum_flag_v<Enum>>>
constexpr Enum& operator^=(Enum& l, const Enum r) noexcept
{
   return l = l ^ r;
}

}
