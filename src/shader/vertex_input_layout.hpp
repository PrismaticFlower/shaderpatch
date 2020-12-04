#pragma once

#include "common.hpp"

#include <cstdint>
#include <string>

#include <absl/container/inlined_vector.h>

#include <dxgiformat.h>

namespace sp::shader {

struct Vertex_generic_input_state {
   bool dynamic_compression : 1 = false;
   bool always_compressed : 1 = false;
   bool position : 1 = false;
   bool skinned : 1 = false;
   bool normal : 1 = false;
   bool tangents : 1 = false;
   bool tangents_unflagged : 1 = false;
   bool color : 1 = false;
   bool texture_coords : 1 = false;
};

enum class Vertex_input_type : std::uint8_t {
   float4,
   float3,
   float2,
   float1,
   uint4,
   uint3,
   uint2,
   uint1,
   sint4,
   sint3,
   sint2,
   sint1
};

struct Vertex_input_element {
   std::string semantic_name;
   std::uint8_t semantic_index = 0;
   Vertex_input_type input_type{};
};

using Vertex_input_layout = absl::InlinedVector<Vertex_input_element, 12>;

auto dxgi_format_to_input_type(const DXGI_FORMAT format) noexcept -> Vertex_input_type;

auto input_type_to_dxgi_format(const Vertex_input_type type) noexcept -> DXGI_FORMAT;

auto get_vertex_input_layout(const Vertex_generic_input_state state,
                             const Vertex_shader_flags flags) -> Vertex_input_layout;

}
