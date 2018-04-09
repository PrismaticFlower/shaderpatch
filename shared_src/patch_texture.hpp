#pragma once

#include "ucfb_writer.hpp"
#include "volume_resource.hpp"

#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include <d3d9.h>

namespace sp {

enum class Texture_version : std::uint32_t { _1 };

struct Texture_info {
   std::uint32_t _reserved{0};
   std::uint32_t width;
   std::uint32_t height;
   std::uint32_t mip_count;
   D3DFORMAT format;
};

struct Sampler_info {
   D3DTEXTUREADDRESS address_mode_u;
   D3DTEXTUREADDRESS address_mode_v;
   D3DTEXTUREADDRESS address_mode_w;
   D3DTEXTUREFILTERTYPE mag_filter;
   D3DTEXTUREFILTERTYPE min_filter;
   D3DTEXTUREFILTERTYPE mip_filter;
   float mip_lod_bias;
   BOOL srgb;
};

inline void write_patch_texture(boost::filesystem::path save_path,
                                const Texture_info& texture_info,
                                const Sampler_info& sampler_info,
                                const std::vector<std::vector<std::byte>>& mip_levels)
{
   using namespace std::literals;
   namespace fs = boost::filesystem;

   std::ostringstream ostream;

   ucfb::Writer writer{ostream, "sptx"_mn};

   writer.emplace_child("VER_"_mn).write(Texture_version::_1);
   writer.emplace_child("NAME"_mn).write(save_path.stem().string());
   writer.emplace_child("INFO"_mn).write(texture_info);
   writer.emplace_child("INFO"_mn).write(sampler_info);

   // write mip levels
   {
      auto mips_writer = writer.emplace_child("MIPS"_mn);

      for (const auto& level : mip_levels) {
         mips_writer.emplace_child("MIP_"_mn).write_unaligned(gsl::make_span(level));
      }
   }

   const auto sptx_data = ostream.str();
   const auto sptx_span =
      gsl::span<const std::byte>(reinterpret_cast<const std::byte*>(sptx_data.data()),
                                 sptx_data.size());

   save_volume_resource(save_path.string(), save_path.stem().string(),
                        Volume_resource_type::texture, sptx_span);
}
}
