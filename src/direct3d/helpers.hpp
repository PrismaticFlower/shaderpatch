#pragma once

#include "com_ptr.hpp"

#include <cstddef>
#include <string>
#include <tuple>

#include <d3d11_1.h>
#include <d3d9.h>

#include <glm/glm.hpp>

namespace sp {
namespace core {
class Shader_patch;
}

namespace d3d9 {

inline auto d3d_to_dxgi_format(const D3DFORMAT format) noexcept -> DXGI_FORMAT
{
   switch (format) {
   case D3DFMT_UNKNOWN:
      return DXGI_FORMAT_UNKNOWN;
   case D3DFMT_A8R8G8B8:
      return DXGI_FORMAT_B8G8R8A8_UNORM;
   case D3DFMT_X8R8G8B8:
      return DXGI_FORMAT_B8G8R8X8_UNORM;
   case D3DFMT_R5G6B5:
      return DXGI_FORMAT_B5G6R5_UNORM;
   case D3DFMT_A1R5G5B5:
   case D3DFMT_X1R5G5B5:
      return DXGI_FORMAT_B5G5R5A1_UNORM;
   case D3DFMT_A4R4G4B4:
      return DXGI_FORMAT_B4G4R4A4_UNORM;
   case D3DFMT_A8:
      return DXGI_FORMAT_A8_UNORM;
   case D3DFMT_A2B10G10R10:
      return DXGI_FORMAT_R10G10B10A2_UNORM;
   case D3DFMT_A8B8G8R8:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
   case D3DFMT_G16R16:
      return DXGI_FORMAT_R16G16_UNORM;
   case D3DFMT_A16B16G16R16:
      return DXGI_FORMAT_R16G16B16A16_UNORM;
   case D3DFMT_V8U8:
      return DXGI_FORMAT_R8G8_SNORM;
   case D3DFMT_Q8W8V8U8:
      return DXGI_FORMAT_R8G8B8A8_SNORM;
   case D3DFMT_V16U16:
      return DXGI_FORMAT_R16G16_SNORM;
   case D3DFMT_R8G8_B8G8:
      return DXGI_FORMAT_G8R8_G8B8_UNORM;
   case D3DFMT_G8R8_G8B8:
      return DXGI_FORMAT_R8G8_B8G8_UNORM;
   case D3DFMT_DXT1:
      return DXGI_FORMAT_BC1_UNORM;
   case D3DFMT_DXT2:
   case D3DFMT_DXT3:
      return DXGI_FORMAT_BC2_UNORM;
   case D3DFMT_DXT4:
   case D3DFMT_DXT5:
      return DXGI_FORMAT_BC3_UNORM;
   case D3DFMT_D16_LOCKABLE:
   case D3DFMT_D16:
      return DXGI_FORMAT_D16_UNORM;
   case D3DFMT_D24S8:
      return DXGI_FORMAT_D24_UNORM_S8_UINT;
   case D3DFMT_D24X8:
   case D3DFMT_D32:
   case D3DFMT_D32F_LOCKABLE:
      return DXGI_FORMAT_D32_FLOAT;
   case D3DFMT_Q16W16V16U16:
      return DXGI_FORMAT_R16G16B16A16_SNORM;
   case D3DFMT_R16F:
      return DXGI_FORMAT_R16_FLOAT;
   case D3DFMT_G16R16F:
      return DXGI_FORMAT_R16G16_FLOAT;
   case D3DFMT_A16B16G16R16F:
      return DXGI_FORMAT_R16G16B16A16_FLOAT;
   case D3DFMT_R32F:
      return DXGI_FORMAT_R32_FLOAT;
   case D3DFMT_G32R32F:
      return DXGI_FORMAT_R32G32_FLOAT;
   case D3DFMT_A32B32G32R32F:
      return DXGI_FORMAT_R32G32B32A32_FLOAT;
   case D3DFMT_A2B10G10R10_XR_BIAS:
      return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
      // Luminance formats.
   case D3DFMT_L8:
      return DXGI_FORMAT_R8_UNORM;
   case D3DFMT_A8L8:
      return DXGI_FORMAT_R8G8_UNORM;
   case D3DFMT_L16:
      return DXGI_FORMAT_UNKNOWN;
   case D3DFMT_R8G8B8:
   case D3DFMT_P8:
   case D3DFMT_R3G3B2:
   case D3DFMT_A8R3G3B2:
   case D3DFMT_X4R4G4B4:
   case D3DFMT_X8B8G8R8:
   case D3DFMT_A2R10G10B10:
   case D3DFMT_A8P8:
   case D3DFMT_A4L4:
   case D3DFMT_L6V5U5:
   case D3DFMT_X8L8V8U8:
   case D3DFMT_A1:
   case D3DFMT_CxV8U8:
   case D3DFMT_UYVY:
   case D3DFMT_YUY2:
   case D3DFMT_D15S1:
   case D3DFMT_D24X4S4:
   case D3DFMT_D24FS8:
   case D3DFMT_D32_LOCKABLE:
   case D3DFMT_S8_LOCKABLE:
   case D3DFMT_BINARYBUFFER:
   case D3DFMT_MULTI2_ARGB8:
   case D3DFMT_VERTEXDATA:
   case D3DFMT_INDEX16:
   case D3DFMT_INDEX32:
      return DXGI_FORMAT_UNKNOWN;
   default:
      return DXGI_FORMAT_UNKNOWN;
   }
}

inline bool is_luminance_format(const D3DFORMAT format)
{
   switch (format) {
   case D3DFMT_L8:
   case D3DFMT_A8L8:
   case D3DFMT_L16:
   case D3DFMT_A4L4:
      return true;
   }

   return false;
}

inline auto d3d_decl_usage_to_string(const D3DDECLUSAGE usage) noexcept -> std::string
{
   using namespace std::literals;

   switch (usage) {
   case D3DDECLUSAGE_POSITION:
      return "POSITION"s;
   case D3DDECLUSAGE_BLENDWEIGHT:
      return "BLENDWEIGHT"s;
   case D3DDECLUSAGE_BLENDINDICES:
      return "BLENDINDICES"s;
   case D3DDECLUSAGE_NORMAL:
      return "NORMAL"s;
   case D3DDECLUSAGE_PSIZE:
      return "PSIZE"s;
   case D3DDECLUSAGE_TEXCOORD:
      return "TEXCOORD"s;
   case D3DDECLUSAGE_TANGENT:
      return "TANGENT"s;
   case D3DDECLUSAGE_BINORMAL:
      return "BINORMAL"s;
   case D3DDECLUSAGE_TESSFACTOR:
      return "TESSFACTOR"s;
   case D3DDECLUSAGE_POSITIONT:
      return "POSITIONT"s;
   case D3DDECLUSAGE_COLOR:
      return "COLOR"s;
   case D3DDECLUSAGE_FOG:
      return "FOG"s;
   case D3DDECLUSAGE_DEPTH:
      return "DEPTH"s;
   case D3DDECLUSAGE_SAMPLE:
      return "SAMPLE"s;
   default:
      std::terminate();
   }
}

inline auto d3d_decl_type_to_dxgi_format(const D3DDECLTYPE type) noexcept
{
   switch (type) {
   case D3DDECLTYPE_FLOAT1:
      return DXGI_FORMAT_R32_FLOAT;
   case D3DDECLTYPE_FLOAT2:
      return DXGI_FORMAT_R32G32_FLOAT;
   case D3DDECLTYPE_FLOAT3:
      return DXGI_FORMAT_R32G32B32_FLOAT;
   case D3DDECLTYPE_FLOAT4:
      return DXGI_FORMAT_R32G32B32A32_FLOAT;
   case D3DDECLTYPE_D3DCOLOR:
      return DXGI_FORMAT_B8G8R8A8_UNORM;
   case D3DDECLTYPE_UBYTE4:
      return DXGI_FORMAT_R8G8B8A8_UINT;
   case D3DDECLTYPE_SHORT2:
      return DXGI_FORMAT_R16G16_SINT;
   case D3DDECLTYPE_SHORT4:
      return DXGI_FORMAT_R16G16B16A16_SINT;
   case D3DDECLTYPE_UBYTE4N:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
   case D3DDECLTYPE_SHORT2N:
      return DXGI_FORMAT_R16G16_SNORM;
   case D3DDECLTYPE_SHORT4N:
      return DXGI_FORMAT_R16G16B16A16_SNORM;
   case D3DDECLTYPE_USHORT2N:
      return DXGI_FORMAT_R16G16_UNORM;
   case D3DDECLTYPE_USHORT4N:
      return DXGI_FORMAT_R16G16B16A16_UNORM;
   case D3DDECLTYPE_FLOAT16_2:
      return DXGI_FORMAT_R16G16_FLOAT;
   case D3DDECLTYPE_FLOAT16_4:
      return DXGI_FORMAT_R16G16B16A16_FLOAT;
   case D3DDECLTYPE_UDEC3:
   case D3DDECLTYPE_DEC3N:
   case D3DDECLTYPE_UNUSED:
   default:
      return DXGI_FORMAT_UNKNOWN;
   }
}

inline auto d3d_decl_type_size(const D3DDECLTYPE type) noexcept -> std::size_t
{
   switch (type) {
   case D3DDECLTYPE_FLOAT1:
   case D3DDECLTYPE_D3DCOLOR:
   case D3DDECLTYPE_UBYTE4:
   case D3DDECLTYPE_SHORT2:
   case D3DDECLTYPE_UBYTE4N:
   case D3DDECLTYPE_SHORT2N:
   case D3DDECLTYPE_USHORT2N:
   case D3DDECLTYPE_UDEC3:
   case D3DDECLTYPE_DEC3N:
   case D3DDECLTYPE_FLOAT16_2:
      return 4;
   case D3DDECLTYPE_FLOAT2:
   case D3DDECLTYPE_SHORT4:
   case D3DDECLTYPE_SHORT4N:
   case D3DDECLTYPE_USHORT4N:
   case D3DDECLTYPE_FLOAT16_4:
      return 8;
   case D3DDECLTYPE_FLOAT3:
      return 12;
   case D3DDECLTYPE_FLOAT4:
      return 16;
   case D3DDECLTYPE_UNUSED:
   default:
      return 0;
   }
}

inline bool is_d3d_decl_type_int(const D3DDECLTYPE type) noexcept
{
   switch (type) {
   case D3DDECLTYPE_FLOAT1:
   case D3DDECLTYPE_FLOAT2:
   case D3DDECLTYPE_FLOAT3:
   case D3DDECLTYPE_FLOAT4:
   case D3DDECLTYPE_D3DCOLOR:
   case D3DDECLTYPE_UBYTE4N:
   case D3DDECLTYPE_SHORT2N:
   case D3DDECLTYPE_SHORT4N:
   case D3DDECLTYPE_USHORT2N:
   case D3DDECLTYPE_USHORT4N:
   case D3DDECLTYPE_FLOAT16_2:
   case D3DDECLTYPE_FLOAT16_4:
   case D3DDECLTYPE_DEC3N:
      return false;
   case D3DDECLTYPE_UBYTE4:
   case D3DDECLTYPE_SHORT2:
   case D3DDECLTYPE_SHORT4:
   case D3DDECLTYPE_UDEC3:
   default:
      return true;
   }
}

inline auto d3d_primitive_type_to_d3d11_topology(const D3DPRIMITIVETYPE type) noexcept
   -> D3D11_PRIMITIVE_TOPOLOGY
{
   switch (type) {
   case D3DPT_POINTLIST:
      return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
   case D3DPT_LINELIST:
      return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
   case D3DPT_LINESTRIP:
      return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
   case D3DPT_TRIANGLELIST:
      return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
   case D3DPT_TRIANGLESTRIP:
      return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
   default:
      return D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
   }
}

inline auto d3d_primitive_count_to_vertex_count(const D3DPRIMITIVETYPE type,
                                                const UINT primitive_count) noexcept -> UINT
{
   switch (type) {
   case D3DPT_POINTLIST:
      return primitive_count;
   case D3DPT_LINELIST:
      return primitive_count * 2;
   case D3DPT_LINESTRIP:
      return primitive_count > 1 ? primitive_count + 1 : 2;
   case D3DPT_TRIANGLELIST:
      return primitive_count * 3;
   case D3DPT_TRIANGLESTRIP:
   case D3DPT_TRIANGLEFAN:
      return primitive_count > 1 ? primitive_count + 2 : 3;
   default:
      return primitive_count;
   }
}

inline auto unpack_d3dcolor(const D3DCOLOR d3dcolor) noexcept -> glm::vec4
{
   const auto color = glm::unpackUnorm4x8(d3dcolor);

   return {color.b, color.g, color.r, color.a};
}

auto create_triangle_fan_quad_ibuf(core::Shader_patch& shader_patch) noexcept
   -> Com_ptr<IDirect3DIndexBuffer9>;

}
}

inline bool operator<(const D3DVERTEXELEMENT9 left, const D3DVERTEXELEMENT9 right) noexcept
{
   return std::tie(left.Stream, left.Offset, left.Type, left.Method, left.Usage,
                   left.UsageIndex) < std::tie(right.Stream, right.Offset,
                                               right.Type, right.Method,
                                               right.Usage, right.UsageIndex);
}

inline bool operator==(const D3DVERTEXELEMENT9 left, const D3DVERTEXELEMENT9 right) noexcept
{
   return std::tie(left.Stream, left.Offset, left.Type, left.Method, left.Usage,
                   left.UsageIndex) == std::tie(right.Stream, right.Offset,
                                                right.Type, right.Method,
                                                right.Usage, right.UsageIndex);
}
