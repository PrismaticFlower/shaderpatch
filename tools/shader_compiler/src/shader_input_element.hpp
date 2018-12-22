#pragma once

#include "compose_exception.hpp"
#include "shader_flags.hpp"
#include "shader_input_type.hpp"

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
   Shader_input_type type;
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

   if (compressed) {
      if (position) {
         elements.push_back({{"POSITION"s, 0}, Shader_input_type::sint4});
      }

      if (hard_skinned) {
         elements.push_back({{"BLENDINDICES"s, 0}, Shader_input_type::float4});
      }

      if (normal) {
         elements.push_back({{"NORMAL"s, 0}, Shader_input_type::float4});
      }

      if (tangents) {
         elements.push_back({{"TANGENT"s, 0}, Shader_input_type::float4});
         elements.push_back({{"BINORMAL"s, 0}, Shader_input_type::float4});
      }

      if (color) {
         elements.push_back({{"COLOR"s, 0}, Shader_input_type::float4});
      }

      if (texcoords) {
         elements.push_back({{"TEXCOORD"s, 0}, Shader_input_type::sint2});
      }
   }
   else {
      if (position) {
         elements.push_back({{"POSITION"s, 0}, Shader_input_type::float3});
      }

      if (hard_skinned) {
         elements.push_back({{"BLENDINDICES"s, 0}, Shader_input_type::float4});
      }

      if (normal) {
         elements.push_back({{"NORMAL"s, 0}, Shader_input_type::float3});
      }

      if (tangents) {
         elements.push_back({{"TANGENT"s, 0}, Shader_input_type::float3});
         elements.push_back({{"BINORMAL"s, 0}, Shader_input_type::float3});
      }

      if (color) {
         elements.push_back({{"COLOR"s, 0}, Shader_input_type::float4});
      }

      if (texcoords) {
         elements.push_back({{"TEXCOORD"s, 0}, Shader_input_type::float2});
      }
   }

   return elements;
}

inline auto type_from_string(const std::string_view string) -> Shader_input_type
{
   using namespace std::literals;

   if (string == "float4"sv) return Shader_input_type::float4;
   if (string == "float3"sv) return Shader_input_type::float3;
   if (string == "float2"sv) return Shader_input_type::float2;
   if (string == "float1"sv) return Shader_input_type::float1;
   if (string == "uint4"sv) return Shader_input_type::uint4;
   if (string == "uint3"sv) return Shader_input_type::uint3;
   if (string == "uint2"sv) return Shader_input_type::uint2;
   if (string == "uint1"sv) return Shader_input_type::uint1;
   if (string == "sint4"sv) return Shader_input_type::sint4;
   if (string == "sint3"sv) return Shader_input_type::sint3;
   if (string == "sint2"sv) return Shader_input_type::sint2;
   if (string == "sint1"sv) return Shader_input_type::sint1;

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
   element.type = type_from_string(j.at("format"s).get<std::string>());
}
}
