#pragma once

#include "string_utilities.hpp"
#include "synced_io.hpp"

#include <cstddef>
#include <string_view>
#include <tuple>

#include <dxgiformat.h>

#include <Compressonator.h>

namespace sp {

inline CMP_FORMAT dxgi_format_to_cmp_format(DXGI_FORMAT dxgi_format)
{
   switch (dxgi_format) {
   case DXGI_FORMAT_R16G16B16A16_TYPELESS:
   case DXGI_FORMAT_R16G16B16A16_FLOAT:
   case DXGI_FORMAT_R16G16B16A16_UNORM:
   case DXGI_FORMAT_R16G16B16A16_UINT:
   case DXGI_FORMAT_R16G16B16A16_SNORM:
   case DXGI_FORMAT_R16G16B16A16_SINT:
      return CMP_FORMAT_RGBA_16;
   case DXGI_FORMAT_R8G8B8A8_TYPELESS:
   case DXGI_FORMAT_R8G8B8A8_UNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
   case DXGI_FORMAT_R8G8B8A8_UINT:
   case DXGI_FORMAT_R8G8B8A8_SNORM:
   case DXGI_FORMAT_R8G8B8A8_SINT:
      return CMP_FORMAT_RGBA_8888;
   case DXGI_FORMAT_R16G16_TYPELESS:
   case DXGI_FORMAT_R16G16_FLOAT:
   case DXGI_FORMAT_R16G16_UNORM:
   case DXGI_FORMAT_R16G16_UINT:
   case DXGI_FORMAT_R16G16_SNORM:
   case DXGI_FORMAT_R16G16_SINT:
      return CMP_FORMAT_RG_16;
   case DXGI_FORMAT_R8G8_TYPELESS:
   case DXGI_FORMAT_R8G8_UNORM:
   case DXGI_FORMAT_R8G8_UINT:
   case DXGI_FORMAT_R8G8_SNORM:
   case DXGI_FORMAT_R8G8_SINT:
      return CMP_FORMAT_RG_8;
   case DXGI_FORMAT_R16_TYPELESS:
   case DXGI_FORMAT_R16_FLOAT:
   case DXGI_FORMAT_D16_UNORM:
   case DXGI_FORMAT_R16_UNORM:
   case DXGI_FORMAT_R16_UINT:
   case DXGI_FORMAT_R16_SNORM:
   case DXGI_FORMAT_R16_SINT:
      return CMP_FORMAT_R_16;
   case DXGI_FORMAT_R8_TYPELESS:
   case DXGI_FORMAT_R8_UNORM:
   case DXGI_FORMAT_R8_UINT:
   case DXGI_FORMAT_R8_SNORM:
   case DXGI_FORMAT_R8_SINT:
   case DXGI_FORMAT_A8_UNORM:
      return CMP_FORMAT_R_8;
   case DXGI_FORMAT_BC1_TYPELESS:
   case DXGI_FORMAT_BC1_UNORM:
   case DXGI_FORMAT_BC1_UNORM_SRGB:
      return CMP_FORMAT_BC1;
   case DXGI_FORMAT_BC2_TYPELESS:
   case DXGI_FORMAT_BC2_UNORM:
   case DXGI_FORMAT_BC2_UNORM_SRGB:
      return CMP_FORMAT_BC2;
   case DXGI_FORMAT_BC3_TYPELESS:
   case DXGI_FORMAT_BC3_UNORM:
   case DXGI_FORMAT_BC3_UNORM_SRGB:
      return CMP_FORMAT_BC3;
   case DXGI_FORMAT_BC4_TYPELESS:
   case DXGI_FORMAT_BC4_UNORM:
   case DXGI_FORMAT_BC4_SNORM:
      return CMP_FORMAT_BC4;
   case DXGI_FORMAT_BC5_TYPELESS:
   case DXGI_FORMAT_BC5_UNORM:
   case DXGI_FORMAT_BC5_SNORM:
      return CMP_FORMAT_BC5;
   case DXGI_FORMAT_B8G8R8A8_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM:
   case DXGI_FORMAT_B8G8R8A8_TYPELESS:
   case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
   case DXGI_FORMAT_B8G8R8X8_TYPELESS:
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return CMP_FORMAT_BGRA_8888;
   case DXGI_FORMAT_BC6H_TYPELESS:
   case DXGI_FORMAT_BC6H_UF16:
      return CMP_FORMAT_BC6H;
   case DXGI_FORMAT_BC6H_SF16:
      return CMP_FORMAT_BC6H_SF;
   case DXGI_FORMAT_BC7_TYPELESS:
   case DXGI_FORMAT_BC7_UNORM:
   case DXGI_FORMAT_BC7_UNORM_SRGB:
      return CMP_FORMAT_BC7;
   case DXGI_FORMAT_UNKNOWN:
   case DXGI_FORMAT_R32G32B32A32_TYPELESS:
   case DXGI_FORMAT_R32G32B32A32_FLOAT:
   case DXGI_FORMAT_R32G32B32A32_UINT:
   case DXGI_FORMAT_R32G32B32A32_SINT:
   case DXGI_FORMAT_R32G32B32_TYPELESS:
   case DXGI_FORMAT_R32G32B32_FLOAT:
   case DXGI_FORMAT_R32G32B32_UINT:
   case DXGI_FORMAT_R32G32B32_SINT:
   case DXGI_FORMAT_R32G32_TYPELESS:
   case DXGI_FORMAT_R32G32_FLOAT:
   case DXGI_FORMAT_R32G32_UINT:
   case DXGI_FORMAT_R32G32_SINT:
   case DXGI_FORMAT_R32G8X24_TYPELESS:
   case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
   case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
   case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
   case DXGI_FORMAT_R10G10B10A2_TYPELESS:
   case DXGI_FORMAT_R10G10B10A2_UNORM:
   case DXGI_FORMAT_R10G10B10A2_UINT:
   case DXGI_FORMAT_R11G11B10_FLOAT:
   case DXGI_FORMAT_R32_TYPELESS:
   case DXGI_FORMAT_D32_FLOAT:
   case DXGI_FORMAT_R32_FLOAT:
   case DXGI_FORMAT_R32_UINT:
   case DXGI_FORMAT_R32_SINT:
   case DXGI_FORMAT_R24G8_TYPELESS:
   case DXGI_FORMAT_D24_UNORM_S8_UINT:
   case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
   case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
   case DXGI_FORMAT_R1_UNORM:
   case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
   case DXGI_FORMAT_R8G8_B8G8_UNORM:
   case DXGI_FORMAT_G8R8_G8B8_UNORM:
   case DXGI_FORMAT_B5G6R5_UNORM:
   case DXGI_FORMAT_B5G5R5A1_UNORM:
   case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
   case DXGI_FORMAT_AYUV:
   case DXGI_FORMAT_Y410:
   case DXGI_FORMAT_Y416:
   case DXGI_FORMAT_NV12:
   case DXGI_FORMAT_P010:
   case DXGI_FORMAT_P016:
   case DXGI_FORMAT_420_OPAQUE:
   case DXGI_FORMAT_YUY2:
   case DXGI_FORMAT_Y210:
   case DXGI_FORMAT_Y216:
   case DXGI_FORMAT_NV11:
   case DXGI_FORMAT_AI44:
   case DXGI_FORMAT_IA44:
   case DXGI_FORMAT_P8:
   case DXGI_FORMAT_A8P8:
   case DXGI_FORMAT_B4G4R4A4_UNORM:
   case DXGI_FORMAT_P208:
   case DXGI_FORMAT_V208:
   case DXGI_FORMAT_V408:
   default:
      return CMP_FORMAT_Unknown;
   }
}

inline auto read_config_format(std::string_view format)
   -> std::tuple<CMP_FORMAT, DXGI_FORMAT>
{
   using namespace std::literals;

   if (format == "BC1"_svci) {
      synced_print("Found texture with BC1 format, consider switching to BC7 "
                   "for potentially higher quality.");

      return {CMP_FORMAT_BC1, DXGI_FORMAT_BC1_UNORM};
   }
   else if (format == "BC3"_svci) {
      synced_print("Found texture with BC3 format, consider switching to BC7 "
                   "for potentially higher quality.");

      return {CMP_FORMAT_BC3, DXGI_FORMAT_BC3_UNORM};
   }
   else if (format == "BC4"_svci) {
      return {CMP_FORMAT_BC4, DXGI_FORMAT_BC4_UNORM};
   }
   else if (format == "BC5"_svci) {
      return {CMP_FORMAT_BC5, DXGI_FORMAT_BC5_UNORM};
   }
   else if (format == "BC6H_UF16"_svci) {
      return {CMP_FORMAT_BC6H, DXGI_FORMAT_BC6H_UF16};
   }
   else if (format == "BC6_SF16"_svci) {
      return {CMP_FORMAT_BC6H_SF, DXGI_FORMAT_BC6H_SF16};
   }
   else if (format == "BC7"_svci) {
      return {CMP_FORMAT_BC7, DXGI_FORMAT_BC7_UNORM};
   }
   else if (format == "ATI2"_svci) {
      synced_print("Found texture with ATI2 format, forcing to BC5.");

      return {CMP_FORMAT_BC5, DXGI_FORMAT_BC5_UNORM};
   }

   throw std::invalid_argument{"Invalid config format."s};
}
}
