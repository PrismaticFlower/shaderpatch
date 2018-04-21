#pragma once

#include "com_ptr.hpp"
#include "compose_exception.hpp"
#include "ucfb_reader.hpp"
#include "ucfb_writer.hpp"
#include "volume_resource.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <boost/filesystem.hpp>
#include <gsl/gsl>

#include <d3d9.h>

namespace sp {

enum class Texture_version : std::uint32_t { _1 };

struct Texture_info {
   std::uint32_t _reserved{0};
   std::int32_t width;
   std::int32_t height;
   std::int32_t mip_count;
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

inline auto load_patch_texture(ucfb::Reader reader, IDirect3DDevice9& device, D3DPOOL pool)
   -> std::tuple<Com_ptr<IDirect3DBaseTexture9>, std::string, Sampler_info>
{
   using namespace std::literals;

   const auto name = reader.read_child_strict<"NAME"_mn>().read_string();

   const auto version =
      reader.read_child_strict<"VER_"_mn>().read_trivial<Texture_version>();

   if (version != Texture_version::_1) {
      throw compose_exception<std::runtime_error>("Unknown layout version in texture "sv,
                                                  std::quoted(name), '.');
   }

   const auto info =
      reader.read_child_strict<"INFO"_mn>().read_trivial<Texture_info>();

   if (info._reserved != 0) {
      throw compose_exception<std::runtime_error>("Reeserved value in texture "sv,
                                                  std::quoted(name),
                                                  " is non-zero."sv);
   }

   const auto sampler_info =
      reader.read_child_strict<"SAMP"_mn>().read_trivial<Sampler_info>();

   Com_ptr<IDirect3DTexture9> texture;

   if (FAILED(device.CreateTexture(info.width, info.height, info.mip_count, 0,
                                   info.format, pool,
                                   texture.clear_and_assign(), nullptr))) {
      throw compose_exception<std::runtime_error>("Direct3D failed to create texture "sv,
                                                  std::quoted(name), '.');
   }

   auto mips = reader.read_child_strict<"MIPS"_mn>();

   for (auto i = 0; i < info.mip_count; ++i) {
      auto level = mips.read_child_strict<"MIP_"_mn>();
      auto data = level.read_array_unaligned<std::byte>(level.size());

      D3DLOCKED_RECT locked;

      texture->LockRect(i, &locked, nullptr, D3DLOCK_DISCARD);

      std::memcpy(locked.pBits, data.data(), static_cast<std::size_t>(data.size()));

      texture->UnlockRect(i);
   }

   return {Com_ptr<IDirect3DBaseTexture9>{texture.release()}, std::string{name},
           sampler_info};
}

inline void write_patch_texture(boost::filesystem::path save_path,
                                const Texture_info& texture_info,
                                const Sampler_info& sampler_info,
                                const std::vector<std::vector<std::byte>>& mip_levels)
{
   using namespace std::literals;
   namespace fs = boost::filesystem;

   std::ostringstream ostream;

   // write sptx chunk
   {
      ucfb::Writer writer{ostream, "sptx"_mn};

      writer.emplace_child("NAME"_mn).write(save_path.stem().string());
      writer.emplace_child("VER_"_mn).write(Texture_version::_1);
      writer.emplace_child("INFO"_mn).write(texture_info);
      writer.emplace_child("SAMP"_mn).write(sampler_info);

      // write mip levels
      {
         auto mips_writer = writer.emplace_child("MIPS"_mn);

         for (const auto& level : mip_levels) {
            mips_writer.emplace_child("MIP_"_mn).write_unaligned(gsl::make_span(level));
         }
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
