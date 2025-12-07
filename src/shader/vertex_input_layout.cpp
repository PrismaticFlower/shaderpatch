
#include "vertex_input_layout.hpp"

#include <exception>

using namespace std::literals;

namespace sp::shader {

auto dxgi_format_to_input_type(const DXGI_FORMAT format) noexcept -> Vertex_input_type
{
   switch (format) {
   case DXGI_FORMAT_R32G32B32A32_FLOAT:
   case DXGI_FORMAT_R16G16B16A16_SNORM:
   case DXGI_FORMAT_R16G16B16A16_FLOAT:
   case DXGI_FORMAT_R16G16B16A16_UNORM:
   case DXGI_FORMAT_R10G10B10A2_UNORM:
   case DXGI_FORMAT_R8G8B8A8_SNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
   case DXGI_FORMAT_B5G5R5A1_UNORM:
   case DXGI_FORMAT_B8G8R8A8_UNORM:
   case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
   case DXGI_FORMAT_B4G4R4A4_UNORM:
      return Vertex_input_type::float4;
   case DXGI_FORMAT_R11G11B10_FLOAT:
   case DXGI_FORMAT_R32G32B32_FLOAT:
   case DXGI_FORMAT_B5G6R5_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return Vertex_input_type::float3;
   case DXGI_FORMAT_R32G32_FLOAT:
   case DXGI_FORMAT_R16G16_FLOAT:
   case DXGI_FORMAT_R16G16_UNORM:
   case DXGI_FORMAT_R16G16_SNORM:
   case DXGI_FORMAT_R8G8_UNORM:
   case DXGI_FORMAT_R8G8_SNORM:
      return Vertex_input_type::float2;
   case DXGI_FORMAT_R32_FLOAT:
   case DXGI_FORMAT_R16_FLOAT:
   case DXGI_FORMAT_R16_UNORM:
   case DXGI_FORMAT_R16_SNORM:
   case DXGI_FORMAT_R8_UNORM:
   case DXGI_FORMAT_R8_SNORM:
      return Vertex_input_type::float1;
   case DXGI_FORMAT_R32G32B32A32_UINT:
   case DXGI_FORMAT_R16G16B16A16_UINT:
   case DXGI_FORMAT_R10G10B10A2_UINT:
   case DXGI_FORMAT_R8G8B8A8_UINT:
      return Vertex_input_type::uint4;
   case DXGI_FORMAT_R32G32B32_UINT:
      return Vertex_input_type::uint3;
   case DXGI_FORMAT_R32G32_UINT:
   case DXGI_FORMAT_R16G16_UINT:
   case DXGI_FORMAT_R8G8_UINT:
      return Vertex_input_type::uint2;
   case DXGI_FORMAT_R32_UINT:
   case DXGI_FORMAT_R16_UINT:
   case DXGI_FORMAT_R8_UINT:
      return Vertex_input_type::uint1;
   case DXGI_FORMAT_R32G32B32A32_SINT:
   case DXGI_FORMAT_R16G16B16A16_SINT:
   case DXGI_FORMAT_R8G8B8A8_SINT:
      return Vertex_input_type::sint4;
   case DXGI_FORMAT_R32G32B32_SINT:
      return Vertex_input_type::sint3;
   case DXGI_FORMAT_R32G32_SINT:
   case DXGI_FORMAT_R16G16_SINT:
   case DXGI_FORMAT_R8G8_SINT:
      return Vertex_input_type::sint2;
   case DXGI_FORMAT_R32_SINT:
   case DXGI_FORMAT_R16_SINT:
   case DXGI_FORMAT_R8_SINT:
      return Vertex_input_type::sint1;
   default:
      std::terminate();
   }
}

auto input_type_to_dxgi_format(const Vertex_input_type type) noexcept -> DXGI_FORMAT
{
   switch (type) {
   case Vertex_input_type::float4:
      return DXGI_FORMAT_R32G32B32A32_FLOAT;
   case Vertex_input_type::float3:
      return DXGI_FORMAT_R32G32B32_FLOAT;
   case Vertex_input_type::float2:
      return DXGI_FORMAT_R32G32_FLOAT;
   case Vertex_input_type::float1:
      return DXGI_FORMAT_R32_FLOAT;
   case Vertex_input_type::uint4:
      return DXGI_FORMAT_R32G32B32A32_UINT;
   case Vertex_input_type::uint3:
      return DXGI_FORMAT_R32G32B32_UINT;
   case Vertex_input_type::uint2:
      return DXGI_FORMAT_R32G32_UINT;
   case Vertex_input_type::uint1:
      return DXGI_FORMAT_R32_UINT;
   case Vertex_input_type::sint4:
      return DXGI_FORMAT_R32G32B32A32_SINT;
   case Vertex_input_type::sint3:
      return DXGI_FORMAT_R32G32B32_SINT;
   case Vertex_input_type::sint2:
      return DXGI_FORMAT_R32G32_SINT;
   case Vertex_input_type::sint1:
      return DXGI_FORMAT_R32_SINT;
   default:
      std::terminate();
   }
}

auto get_vertex_input_layout(const Vertex_generic_input_state state,
                             const Vertex_shader_flags flags) -> Vertex_input_layout
{
   const auto position_type = Vertex_input_type::sint4;
   const auto blendweight_type = Vertex_input_type::float4;
   const auto blendindices_type = Vertex_input_type::float4;
   const auto normal_type = Vertex_input_type::float4;
   const auto color_type = Vertex_input_type::float4;
   const auto texcoords_type = Vertex_input_type::sint2;

   const bool color = state.color && (flags & Vertex_shader_flags::color) !=
                                        Vertex_shader_flags::none;
   const bool hard_skinned =
      state.skinned &&
      (flags & Vertex_shader_flags::hard_skinned) != Vertex_shader_flags::none;

   Vertex_input_layout layout;

   if (state.position) {
      layout.push_back({.semantic_name = "POSITION"s, .input_type = position_type});
   }

   if (hard_skinned) {
      layout.push_back({.semantic_name = "BLENDWEIGHT"s, .input_type = blendweight_type});

      layout.push_back(
         {.semantic_name = "BLENDINDICES"s, .input_type = blendindices_type});
   }

   if (state.normal) {
      layout.push_back({.semantic_name = "NORMAL"s, .input_type = normal_type});
   }

   if (state.tangents) {
      layout.push_back({.semantic_name = "TANGENT"s, .input_type = normal_type});
      layout.push_back({.semantic_name = "BINORMAL"s, .input_type = normal_type});
   }

   if (color) {
      layout.push_back({.semantic_name = "COLOR"s, .input_type = color_type});
   }

   if (state.texture_coords) {
      layout.push_back({.semantic_name = "TEXCOORD"s, .input_type = texcoords_type});
   }

   return layout;
}

}
