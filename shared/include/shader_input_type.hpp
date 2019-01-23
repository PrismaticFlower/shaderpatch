#pragma once

#include <cstdint>

#include <dxgiformat.h>

namespace sp {

enum class Shader_input_type : std::int32_t {
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

inline auto dxgi_format_to_input_type(const DXGI_FORMAT format) noexcept -> Shader_input_type
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
      return Shader_input_type::float4;
   case DXGI_FORMAT_R11G11B10_FLOAT:
   case DXGI_FORMAT_R32G32B32_FLOAT:
   case DXGI_FORMAT_B5G6R5_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return Shader_input_type::float3;
   case DXGI_FORMAT_R32G32_FLOAT:
   case DXGI_FORMAT_R16G16_FLOAT:
   case DXGI_FORMAT_R16G16_UNORM:
   case DXGI_FORMAT_R16G16_SNORM:
   case DXGI_FORMAT_R8G8_UNORM:
   case DXGI_FORMAT_R8G8_SNORM:
      return Shader_input_type::float2;
   case DXGI_FORMAT_R32_FLOAT:
   case DXGI_FORMAT_R16_FLOAT:
   case DXGI_FORMAT_R16_UNORM:
   case DXGI_FORMAT_R16_SNORM:
   case DXGI_FORMAT_R8_UNORM:
   case DXGI_FORMAT_R8_SNORM:
      return Shader_input_type::float1;
   case DXGI_FORMAT_R32G32B32A32_UINT:
   case DXGI_FORMAT_R16G16B16A16_UINT:
   case DXGI_FORMAT_R10G10B10A2_UINT:
   case DXGI_FORMAT_R8G8B8A8_UINT:
      return Shader_input_type::uint4;
   case DXGI_FORMAT_R32G32B32_UINT:
      return Shader_input_type::uint3;
   case DXGI_FORMAT_R32G32_UINT:
   case DXGI_FORMAT_R16G16_UINT:
   case DXGI_FORMAT_R8G8_UINT:
      return Shader_input_type::uint2;
   case DXGI_FORMAT_R32_UINT:
   case DXGI_FORMAT_R16_UINT:
   case DXGI_FORMAT_R8_UINT:
      return Shader_input_type::uint1;
   case DXGI_FORMAT_R32G32B32A32_SINT:
   case DXGI_FORMAT_R16G16B16A16_SINT:
   case DXGI_FORMAT_R8G8B8A8_SINT:
      return Shader_input_type::sint4;
   case DXGI_FORMAT_R32G32B32_SINT:
      return Shader_input_type::sint3;
   case DXGI_FORMAT_R32G32_SINT:
   case DXGI_FORMAT_R16G16_SINT:
   case DXGI_FORMAT_R8G8_SINT:
      return Shader_input_type::sint2;
   case DXGI_FORMAT_R32_SINT:
   case DXGI_FORMAT_R16_SINT:
   case DXGI_FORMAT_R8_SINT:
      return Shader_input_type::sint1;
   default:
      std::terminate();
   }
}

inline auto input_type_to_dxgi_format(const Shader_input_type type) noexcept -> DXGI_FORMAT
{
   switch (type) {
   case Shader_input_type::float4:
      return DXGI_FORMAT_R32G32B32A32_FLOAT;
   case Shader_input_type::float3:
      return DXGI_FORMAT_R32G32B32_FLOAT;
   case Shader_input_type::float2:
      return DXGI_FORMAT_R32G32_FLOAT;
   case Shader_input_type::float1:
      return DXGI_FORMAT_R32_FLOAT;
   case Shader_input_type::uint4:
      return DXGI_FORMAT_R32G32B32A32_UINT;
   case Shader_input_type::uint3:
      return DXGI_FORMAT_R32G32B32_UINT;
   case Shader_input_type::uint2:
      return DXGI_FORMAT_R32G32_UINT;
   case Shader_input_type::uint1:
      return DXGI_FORMAT_R32_UINT;
   case Shader_input_type::sint4:
      return DXGI_FORMAT_R32G32B32A32_SINT;
   case Shader_input_type::sint3:
      return DXGI_FORMAT_R32G32B32_SINT;
   case Shader_input_type::sint2:
      return DXGI_FORMAT_R32G32_SINT;
   case Shader_input_type::sint1:
      return DXGI_FORMAT_R32_SINT;
   default:
      std::terminate();
   }
}

}
