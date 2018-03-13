#pragma once

#include <cstdint>

namespace sp {

enum class Shader_flags : std::uint32_t {
   none = 0,

   light_dir = 16,   // Directional Light
   light_point = 1,  // Point Light
   light_point2 = 2, // 2 Point Lights
   light_point4 = 3, // 4 Point Lights
   light_spot = 4,   // Spot Light

   light_dir_point = 17,      // Directional Lights + Point Light
   light_dir_point2 = 18,     // Directional Lights + 2 Point Lights
   light_dir_point4 = 19,     // Directional Lights + 4 Point Lights
   light_dir_spot = 20,       // Direction Lights + Spot Light
   light_dir_point_spot = 21, // Directional Lights + Point Light + Spot Light
   light_dir_point2_spot = 22, // Directional Lights + 2 Point Lights + Spot Light

   skinned = 64,     // Soft Skinned
   vertexcolor = 128 // Vertexcolor
};

constexpr Shader_flags operator|(const Shader_flags l, const Shader_flags r) noexcept
{
   return Shader_flags{static_cast<std::uint32_t>(l) | static_cast<std::uint32_t>(r)};
}

constexpr Shader_flags operator&(Shader_flags l, Shader_flags r) noexcept
{
   return Shader_flags{static_cast<std::uint32_t>(l) & static_cast<std::uint32_t>(r)};
}

constexpr Shader_flags operator^(const Shader_flags l, const Shader_flags r) noexcept
{
   return Shader_flags{static_cast<std::uint32_t>(l) ^ static_cast<std::uint32_t>(r)};
}

constexpr Shader_flags operator~(const Shader_flags f) noexcept
{
   return Shader_flags{~static_cast<std::uint32_t>(f)};
}

constexpr Shader_flags& operator|=(Shader_flags& l, const Shader_flags r) noexcept
{
   return l = l | r;
}

constexpr Shader_flags& operator&=(Shader_flags& l, const Shader_flags r) noexcept
{
   return l = l & r;
}

constexpr Shader_flags& operator^=(Shader_flags& l, const Shader_flags r) noexcept
{
   return l = l ^ r;
}
}
