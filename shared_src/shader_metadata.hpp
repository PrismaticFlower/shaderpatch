#pragma once

#include "magic_number.hpp"
#include "shader_flags.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include <gsl/gsl>

#include <d3d9.h>

namespace sp {

struct Shader_metadata {
   std::string name;
   std::string entry_point;
   std::string target;

   Shader_flags shader_flags;
};

inline auto read_shader_metadata(std::vector<std::uint8_t> msgpack) noexcept -> Shader_metadata
{
   using namespace std::literals;

   const auto json = nlohmann::json::from_msgpack(msgpack, 0);

   Shader_metadata data{json["name"s], json["entry_point"s], json["target"s]};

   data.shader_flags =
      static_cast<Shader_flags>(std::uint32_t{json["shader_flags"s]});

   return data;
}

inline auto get_shader_metadata(const DWORD* const bytecode) noexcept
   -> std::optional<Shader_metadata>
{
   const auto split_dword = [](const DWORD dword) {
      std::array<std::uint16_t, 2> split;
      split[0] = dword & 0xffffu;
      split[1] = (dword >> 16) & 0xffffu;

      return split;
   };

   constexpr std::uint16_t comment = 0xfffeu;
   constexpr auto meta_mark = static_cast<DWORD>("META"_mn);

   const auto words = split_dword(bytecode[1]);

   if (words[0] != comment) return std::nullopt;
   if (words[1] < 2) return std::nullopt;
   if (bytecode[2] != meta_mark) return std::nullopt;

   const auto size = bytecode[3];
   const auto data_ptr = reinterpret_cast<const std::uint8_t*>(&bytecode[4]);

   std::vector<std::uint8_t> data{data_ptr, data_ptr + size};

   return read_shader_metadata(data);
}

inline auto embed_meta_data(const nlohmann::json& extra_metadata,
                            std::string_view rendertype, std::string_view entry_point,
                            std::string_view target, Shader_flags flags,
                            gsl::span<const DWORD> bytecode_span) -> std::vector<DWORD>
{
   using namespace std::literals;

   nlohmann::json meta = extra_metadata;

   meta["name"s] = std::string{rendertype};
   meta["entry_point"s] = std::string{entry_point};
   meta["target"s] = std::string{target};
   meta["shader_flags"s] = static_cast<std::uint32_t>(flags);

   auto meta_buffer = nlohmann::json::to_msgpack(meta);

   const auto meta_size =
      (meta_buffer.size() / sizeof(DWORD)) + (meta_buffer.size() % sizeof(DWORD));

   if (meta_size >= 100000) {
      throw std::runtime_error{
         "Shader meta data too large to embed in bytecode."};
   }

   std::vector<DWORD> bytecode;
   bytecode.reserve(bytecode_span.size() + 3 + meta_size);

   bytecode.emplace_back(bytecode_span[0]);

   // comment 0xfffe
   bytecode.emplace_back((0xfffeu) | ((meta_size + 2u) << 16u));
   bytecode.emplace_back(static_cast<DWORD>("META"_mn));
   bytecode.emplace_back(static_cast<DWORD>(meta_buffer.size()));

   const auto meta_start = bytecode.size();

   bytecode.resize(bytecode.size() + meta_size);

   std::memcpy(&bytecode[meta_start], meta_buffer.data(), meta_buffer.size());

   const auto shader_start = bytecode.size();

   bytecode.resize(bytecode.size() + bytecode_span.size() - 1);

   std::memcpy(&bytecode[shader_start], &bytecode_span[1],
               bytecode_span.size_bytes() - sizeof(DWORD));

   return bytecode;
}
}
