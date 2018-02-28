#pragma once

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

#include <d3d9types.h>

namespace sp::direct3d {

enum class Vs_flags : std::uint32_t {
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

constexpr auto operator&(Vs_flags left, Vs_flags right)
{
   using T = std::underlying_type_t<Vs_flags>;

   return static_cast<Vs_flags>(static_cast<T>(left) & static_cast<T>(right));
}

struct Shader_metadata {
   std::string name;
   std::string entry_point;
   std::string target;

   std::optional<Vs_flags> vs_flags;
};

auto read_shader_metadata(std::vector<std::uint8_t> msgpack) noexcept -> Shader_metadata
{
   using namespace std::literals;

   const auto json = nlohmann::json::from_msgpack(msgpack, 0);

   Shader_metadata data{json["name"s], json["entry_point"s], json["target"s]};

   if (json.value("vs_flags"s, 0)) {
      data.vs_flags = static_cast<Vs_flags>(std::uint32_t{json["vs_flags"s]});
   }

   return data;
}

auto get_shader_metadata(const DWORD* const bytecode) noexcept
   -> std::optional<Shader_metadata>
{
   const auto split_dword = [](const DWORD dword) {
      std::array<std::uint16_t, 2> split;
      split[0] = dword & 0xFFFFui16;
      split[1] = (dword >> 16) & 0xFFFFui16;

      return split;
   };

   constexpr auto comment = 0xfffeui16;
   constexpr auto meta_mark = 0x4154454dui32;

   const auto words = split_dword(bytecode[1]);

   if (words[0] != comment) return std::nullopt;
   if (words[1] < 2) return std::nullopt;
   if (bytecode[2] != meta_mark) return std::nullopt;

   const auto size = bytecode[3];
   const auto data_ptr = reinterpret_cast<const std::uint8_t*>(&bytecode[4]);

   std::vector<std::uint8_t> data{data_ptr, data_ptr + size};

   return read_shader_metadata(data);
}
}
