#pragma once

#pragma once

#include <cstdint>

namespace sp {

enum class Material_flags : std::uint32_t {
   normal = 1,
   hardedged = 2,
   transparent = 4,
   glossmap = 8,
   glow = 16,
   perpixel = 32,
   additive = 64,
   specular = 128,
   env_map = 256,
   vertex_lit = 512,
   unknown = 2048,
   doublesided = 65536,

   scrolling = 16777216,
   energy = 33554432,
   animated = 67108864,

   attached_light = 134217728,
};

constexpr Material_flags operator|(const Material_flags l, const Material_flags r) noexcept
{
   return Material_flags{static_cast<std::uint32_t>(l) |
                         static_cast<std::uint32_t>(r)};
}

constexpr Material_flags operator&(Material_flags l, Material_flags r) noexcept
{
   return Material_flags{static_cast<std::uint32_t>(l) &
                         static_cast<std::uint32_t>(r)};
}

constexpr Material_flags operator^(const Material_flags l, const Material_flags r) noexcept
{
   return Material_flags{static_cast<std::uint32_t>(l) ^
                         static_cast<std::uint32_t>(r)};
}

constexpr Material_flags operator~(const Material_flags f) noexcept
{
   return Material_flags{~static_cast<std::uint32_t>(f)};
}

constexpr Material_flags& operator|=(Material_flags& l, const Material_flags r) noexcept
{
   return l = l | r;
}

constexpr Material_flags& operator&=(Material_flags& l, const Material_flags r) noexcept
{
   return l = l & r;
}

constexpr Material_flags& operator^=(Material_flags& l, const Material_flags r) noexcept
{
   return l = l ^ r;
}
}
