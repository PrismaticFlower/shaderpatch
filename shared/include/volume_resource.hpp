#pragma once

#include "ucfb_writer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <gsl/gsl>

namespace sp {

enum class Volume_resource_type : std::uint16_t {
   texture = 4096,
   material = 8192,
   fx_config = 16384
};

constexpr auto unpack_resource_size(std::uint32_t height, std::uint32_t depth) noexcept
{
   return height | (depth << 15u);
}

constexpr auto pack_size(std::uint32_t size) noexcept -> std::array<std::uint16_t, 2>
{
   const std::uint16_t lower = size & 0x7fffu;
   const std::uint16_t upper = (size >> 15u) & 0x7fffu;

   return {lower, upper};
}

inline void save_volume_resource(const std::string& output_path,
                                 std::string_view name, Volume_resource_type type,
                                 gsl::span<const std::byte> data)
{
   using namespace std::literals;

   auto file = ucfb::open_file_for_output(std::string{output_path});

   ucfb::Writer root_writer{file};
   auto writer = root_writer.emplace_child("tex_"_mn);

   std::string prefix;

   if (type == Volume_resource_type::texture) prefix = "_SP_TEXTURE_"s;
   if (type == Volume_resource_type::fx_config) prefix = "_SP_FXCFG_"s;

   writer.emplace_child("NAME"_mn).write(prefix += name);

   constexpr std::uint32_t fmt_l8 = 50;

   // write texture format list
   {
      auto info_writer = writer.emplace_child("INFO"_mn);
      info_writer.write(std::uint32_t{1}, fmt_l8);
   }

   // write texture format chunk
   {
      auto fmt_writer = writer.emplace_child("FMT_"_mn);

      // write texture format info
      {
         auto info_writer = fmt_writer.emplace_child("INFO"_mn);

         constexpr std::uint32_t volume_texture_id = 1795;

         info_writer.write(fmt_l8);
         info_writer.write_unaligned(type);
         info_writer.write_unaligned(pack_size(data.size()));
         info_writer.write_unaligned(std::uint16_t{1});
         info_writer.write_unaligned(volume_texture_id);
      }

      // write texture level
      {
         auto face_writer = fmt_writer.emplace_child("FACE"_mn);
         auto lvl_writer = face_writer.emplace_child("LVL_"_mn);

         // write level info
         {
            auto info_writer = lvl_writer.emplace_child("INFO"_mn);

            info_writer.write(std::uint32_t{0});
            info_writer.write(static_cast<std::uint32_t>(data.size()));
         }

         // write level body
         lvl_writer.emplace_child("BODY"_mn).write(data);
      }
   }
}
}
