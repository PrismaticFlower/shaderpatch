#pragma once

#include "string_utilities.hpp"
#include "synced_io.hpp"

#include <cstddef>
#include <string_view>
#include <tuple>

#include <dxgiformat.h>

namespace sp {

enum class Compression_format {
   BC3,
   BC4,
   BC5,
   BC6H_UF16,
   // BC6H_SF16,
   BC7,
   BC7_ALPHA
};

inline auto read_config_format(std::string_view format)
   -> std::pair<Compression_format, DXGI_FORMAT>
{
   if (format == "BC1"_svci) {
      synced_print("Found texture with BC1 format, forcing to BC3, consider "
                   "switching to BC7 "
                   "for potentially higher quality.");

      return {Compression_format::BC3, DXGI_FORMAT_BC3_UNORM};
   }
   else if (format == "BC3"_svci) {
      synced_print("Found texture with BC3 format, consider switching to BC7 "
                   "for potentially higher quality.");

      return {Compression_format::BC3, DXGI_FORMAT_BC3_UNORM};
   }
   else if (format == "BC4"_svci) {
      return {Compression_format::BC4, DXGI_FORMAT_BC4_UNORM};
   }
   else if (format == "BC5"_svci) {
      return {Compression_format::BC5, DXGI_FORMAT_BC5_UNORM};
   }
   else if (format == "BC6H_UF16"_svci) {
      return {Compression_format::BC6H_UF16, DXGI_FORMAT_BC6H_UF16};
   }
   else if (format == "BC6_SF16"_svci) {
      throw std::invalid_argument{
         "Signed BC6 compression is currently unimplemented."};
   }
   else if (format == "BC7"_svci) {
      return {Compression_format::BC7, DXGI_FORMAT_BC7_UNORM};
   }
   else if (format == "BC7_ALPHA"_svci) {
      return {Compression_format::BC7_ALPHA, DXGI_FORMAT_BC7_UNORM};
   }
   else if (format == "ATI2"_svci) {
      synced_print("Found texture with ATI2 format, forcing to BC5.");

      return {Compression_format::BC5, DXGI_FORMAT_BC5_UNORM};
   }

   throw std::invalid_argument{"Invalid config format."};
}

inline auto make_non_srgb(const DXGI_FORMAT format) noexcept -> DXGI_FORMAT
{
   switch (format) {

   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
   case DXGI_FORMAT_BC1_UNORM_SRGB:
      return DXGI_FORMAT_BC1_UNORM;
   case DXGI_FORMAT_BC2_UNORM_SRGB:
      return DXGI_FORMAT_BC2_UNORM;
   case DXGI_FORMAT_BC3_UNORM_SRGB:
      return DXGI_FORMAT_BC3_UNORM;
   case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      return DXGI_FORMAT_B8G8R8A8_UNORM;
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return DXGI_FORMAT_B8G8R8X8_UNORM;
   case DXGI_FORMAT_BC7_UNORM_SRGB:
      return DXGI_FORMAT_BC7_UNORM;
   }

   return format;
}

inline bool is_srgb(const DXGI_FORMAT format) noexcept
{
   switch (format) {
   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
   case DXGI_FORMAT_BC1_UNORM_SRGB:
   case DXGI_FORMAT_BC2_UNORM_SRGB:
   case DXGI_FORMAT_BC3_UNORM_SRGB:
   case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
   case DXGI_FORMAT_BC7_UNORM_SRGB:
      return true;
   }

   return false;
}

}
