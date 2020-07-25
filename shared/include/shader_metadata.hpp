#pragma once

#include "game_rendertypes.hpp"
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

namespace sp {

struct Shader_metadata {
   Rendertype rendertype{};
   std::string rendertype_name;
   std::string shader_name;

   Vertex_shader_flags vertex_shader_flags = Vertex_shader_flags::none;

   bool light_active = false;
   std::uint8_t light_active_point_count = 0;
   bool light_active_spot = 0;

   std::array<bool, 4> srgb_state = {false, false, false, false};
};

inline auto serialize_shader_metadata(const Shader_metadata& metadata) noexcept
   -> std::vector<std::byte>
{
   using namespace std::literals;

   nlohmann::json meta{};

   meta["rendertype"s] = static_cast<std::uint32_t>(metadata.rendertype);
   meta["rendertype_name"s] = metadata.rendertype_name;
   meta["shader_name"s] = metadata.shader_name;
   meta["vertex_shader_flags"s] =
      static_cast<std::uint32_t>(metadata.vertex_shader_flags);
   meta["light_active"s] = metadata.light_active;
   meta["light_active_point_count"s] = metadata.light_active_point_count;
   meta["light_active_spot"s] = metadata.light_active_spot;
   meta["srgb_state"s] = metadata.srgb_state;

   const auto cbor = nlohmann::json::to_cbor(meta);

   std::vector<std::byte> data;
   data.resize(cbor.size() + sizeof(std::uint32_t));

   const auto cbor_size = static_cast<std::uint32_t>(cbor.size());

   std::memcpy(data.data(), &cbor_size, sizeof(cbor_size));
   std::memcpy(data.data() + 4, cbor.data(), cbor.size());

   return data;
}

inline auto deserialize_shader_metadata(const std::byte* const data) noexcept -> Shader_metadata
{
   using namespace std::literals;

   std::uint32_t cbor_size;
   std::memcpy(&cbor_size, data, sizeof(cbor_size));

   std::vector<std::uint8_t> cbor;
   cbor.assign(reinterpret_cast<const std::uint8_t*>(data + 4),
               reinterpret_cast<const std::uint8_t*>(data + cbor_size + 4));

   const auto json = nlohmann::json::from_cbor(cbor);

   Shader_metadata metadata;

   metadata.rendertype = static_cast<Rendertype>(std::uint32_t{json["rendertype"s]});
   metadata.rendertype_name = json["rendertype_name"s].get<std::string>();
   metadata.shader_name = json["shader_name"s].get<std::string>();

   metadata.vertex_shader_flags =
      static_cast<Vertex_shader_flags>(std::uint32_t{json["vertex_shader_flags"s]});
   metadata.light_active = json["light_active"s];
   metadata.light_active_point_count = json["light_active_point_count"s];
   metadata.light_active_spot = json["light_active_spot"s];
   metadata.srgb_state = json["srgb_state"s];

   return metadata;
}

}
