#pragma once

#include "compose_exception.hpp"
#include "shader_flags.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <dxgiformat.h>

#include <nlohmann/json.hpp>

namespace sp::shader {

struct Input_semantic {
   std::string name;
   std::uint32_t index;
};

struct Input_element {
   Input_semantic semantic;
   DXGI_FORMAT format;
   std::uint32_t offset;
};

inline auto vs_flags_to_input_layout(const Vertex_shader_flags flags) noexcept
   -> std::vector<shader::Input_element>
{
   using namespace std::literals;

   const bool compressed =
      (flags & Vertex_shader_flags::compressed) != Vertex_shader_flags::none;
   const bool position =
      (flags & Vertex_shader_flags::position) != Vertex_shader_flags::none;
   const bool normal =
      (flags & Vertex_shader_flags::normal) != Vertex_shader_flags::none;
   const bool tangents =
      (flags & Vertex_shader_flags::tangents) != Vertex_shader_flags::none;
   const bool texcoords =
      (flags & Vertex_shader_flags::texcoords) != Vertex_shader_flags::none;
   const bool color =
      (flags & Vertex_shader_flags::color) != Vertex_shader_flags::none;
   const bool hard_skinned =
      (flags & Vertex_shader_flags::hard_skinned) != Vertex_shader_flags::none;

   std::vector<shader::Input_element> elements;
   elements.reserve(7);

   if (std::uint32_t offset = 0; compressed) {
      if (position) {
         elements.push_back({{"POSITION"s, 0},
                             DXGI_FORMAT_R16G16B16A16_SINT,
                             std::exchange(offset, offset + 8)});
      }

      if (hard_skinned) {
         elements.push_back({{"BLENDINDICES"s, 0},
                             DXGI_FORMAT_B8G8R8A8_UNORM,
                             std::exchange(offset, offset + 4)});
      }

      if (normal) {
         elements.push_back({{"NORMAL"s, 0},
                             DXGI_FORMAT_B8G8R8A8_UNORM,
                             std::exchange(offset, offset + 4)});
      }

      if (tangents) {
         elements.push_back({{"TANGENT"s, 0},
                             DXGI_FORMAT_B8G8R8A8_UNORM,
                             std::exchange(offset, offset + 4)});
         elements.push_back({{"BINORMAL"s, 0},
                             DXGI_FORMAT_B8G8R8A8_UNORM,
                             std::exchange(offset, offset + 4)});
      }

      if (color) {
         elements.push_back({{"COLOR"s, 0},
                             DXGI_FORMAT_B8G8R8A8_UNORM,
                             std::exchange(offset, offset + 4)});
      }

      if (texcoords) {
         elements.push_back({{"TEXCOORD"s, 0},
                             DXGI_FORMAT_R16G16_SINT,
                             std::exchange(offset, offset + 4)});
      }
   }
   else {
      if (position) {
         elements.push_back({{"POSITION"s, 0},
                             DXGI_FORMAT_R32G32B32_FLOAT,
                             std::exchange(offset, offset + 12)});
      }

      if (hard_skinned) {
         elements.push_back({{"BLENDINDICES"s, 0},
                             DXGI_FORMAT_B8G8R8A8_UNORM,
                             std::exchange(offset, offset + 4)});
      }

      if (normal) {
         elements.push_back({{"NORMAL"s, 0},
                             DXGI_FORMAT_R32G32B32_FLOAT,
                             std::exchange(offset, offset + 12)});
      }

      if (tangents) {
         elements.push_back({{"TANGENT"s, 0},
                             DXGI_FORMAT_R32G32B32_FLOAT,
                             std::exchange(offset, offset + 12)});
         elements.push_back({{"BINORMAL"s, 0},
                             DXGI_FORMAT_R32G32B32_FLOAT,
                             std::exchange(offset, offset + 12)});
      }

      if (color) {
         elements.push_back({{"COLOR"s, 0},
                             DXGI_FORMAT_B8G8R8A8_UNORM,
                             std::exchange(offset, offset + 4)});
      }

      if (texcoords) {
         elements.push_back({{"TEXCOORD"s, 0},
                             DXGI_FORMAT_R16G16_FLOAT,
                             std::exchange(offset, offset + 8)});
      }
   }

   return elements;
}

inline auto format_from_string(const std::string_view string) -> DXGI_FORMAT
{
   using namespace std::literals;

   // clang-format off
   if (string == "R32G32B32A32_FLOAT"sv) return DXGI_FORMAT_R32G32B32A32_FLOAT;
   if (string == "R32G32B32A32_UINT"sv) return DXGI_FORMAT_R32G32B32A32_UINT;
   if (string == "R32G32B32A32_SINT"sv) return DXGI_FORMAT_R32G32B32A32_SINT;
   if (string == "R32G32B32_FLOAT"sv) return DXGI_FORMAT_R32G32B32_FLOAT;
   if (string == "R32G32B32_UINT"sv) return DXGI_FORMAT_R32G32B32_UINT;
   if (string == "R32G32B32_SINT"sv) return DXGI_FORMAT_R32G32B32_SINT;
   if (string == "R16G16B16A16_FLOAT"sv) return DXGI_FORMAT_R16G16B16A16_FLOAT;
   if (string == "R16G16B16A16_UNORM"sv) return DXGI_FORMAT_R16G16B16A16_UNORM;
   if (string == "R16G16B16A16_UINT"sv) return DXGI_FORMAT_R16G16B16A16_UINT;
   if (string == "R16G16B16A16_SNORM"sv) return DXGI_FORMAT_R16G16B16A16_SNORM;
   if (string == "R16G16B16A16_SINT"sv) return DXGI_FORMAT_R16G16B16A16_SINT;
   if (string == "R32G32_FLOAT"sv) return DXGI_FORMAT_R32G32_FLOAT;
   if (string == "R32G32_UINT"sv) return DXGI_FORMAT_R32G32_UINT;
   if (string == "R32G32_SINT"sv) return DXGI_FORMAT_R32G32_SINT;
   if (string == "R10G10B10A2_UNORM"sv) return DXGI_FORMAT_R10G10B10A2_UNORM;
   if (string == "R10G10B10A2_UINT"sv) return DXGI_FORMAT_R10G10B10A2_UINT;
   if (string == "R11G11B10_FLOAT"sv) return DXGI_FORMAT_R11G11B10_FLOAT;
   if (string == "R8G8B8A8_UNORM"sv) return DXGI_FORMAT_R8G8B8A8_UNORM;
   if (string == "R8G8B8A8_UNORM_SRGB"sv) return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
   if (string == "R8G8B8A8_UINT"sv) return DXGI_FORMAT_R8G8B8A8_UINT;
   if (string == "R8G8B8A8_SNORM"sv) return DXGI_FORMAT_R8G8B8A8_SNORM;
   if (string == "R8G8B8A8_SINT"sv) return DXGI_FORMAT_R8G8B8A8_SINT;
   if (string == "R16G16_FLOAT"sv) return DXGI_FORMAT_R16G16_FLOAT;
   if (string == "R16G16_UNORM"sv) return DXGI_FORMAT_R16G16_UNORM;
   if (string == "R16G16_UINT"sv) return DXGI_FORMAT_R16G16_UINT;
   if (string == "R16G16_SNORM"sv) return DXGI_FORMAT_R16G16_SNORM;
   if (string == "R16G16_SINT"sv) return DXGI_FORMAT_R16G16_SINT;
   if (string == "R32_FLOAT"sv) return DXGI_FORMAT_R32_FLOAT;
   if (string == "R32_UINT"sv) return DXGI_FORMAT_R32_UINT;
   if (string == "R32_SINT"sv) return DXGI_FORMAT_R32_SINT;
   if (string == "R8G8_UNORM"sv) return DXGI_FORMAT_R8G8_UNORM;
   if (string == "R8G8_UINT"sv) return DXGI_FORMAT_R8G8_UINT;
   if (string == "R8G8_SNORM"sv) return DXGI_FORMAT_R8G8_SNORM;
   if (string == "R8G8_SINT"sv) return DXGI_FORMAT_R8G8_SINT;
   if (string == "R16_FLOAT"sv) return DXGI_FORMAT_R16_FLOAT;
   if (string == "R16_UNORM"sv) return DXGI_FORMAT_R16_UNORM;
   if (string == "R16_UINT"sv) return DXGI_FORMAT_R16_UINT;
   if (string == "R16_SNORM"sv) return DXGI_FORMAT_R16_SNORM;
   if (string == "R16_SINT"sv) return DXGI_FORMAT_R16_SINT;
   if (string == "R8_UNORM"sv) return DXGI_FORMAT_R8_UNORM;
   if (string == "R8_UINT"sv) return DXGI_FORMAT_R8_UINT;
   if (string == "R8_SNORM"sv) return DXGI_FORMAT_R8_SNORM;
   if (string == "R8_SINT"sv) return DXGI_FORMAT_R8_SINT;
   if (string == "R9G9B9E5_SHAREDEXP"sv) return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
   if (string == "B8G8R8A8_UNORM"sv) return DXGI_FORMAT_B8G8R8A8_UNORM;
   if (string == "B8G8R8X8_UNORM"sv) return DXGI_FORMAT_B8G8R8X8_UNORM;
   if (string == "B4G4R4A4_UNORM"sv) return DXGI_FORMAT_B4G4R4A4_UNORM;
   // clang-format on

   throw compose_exception<std::runtime_error>("Invalid input layout format specified: "sv,
                                               string);
}

inline void from_json(const nlohmann::json& j, Input_semantic& semantic)
{
   using namespace std::literals;

   semantic.name = j.at("name"s).get<std::string>();
   semantic.index = j.at("index"s).get<std::uint32_t>();
}

inline void from_json(const nlohmann::json& j, Input_element& element)
{
   using namespace std::literals;

   element.semantic = j.at("semantic"s).get<Input_semantic>();
   element.format = format_from_string(j.at("format"s).get<std::string>());
   element.offset = j.at("offset"s).get<std::uint32_t>();
}
}
