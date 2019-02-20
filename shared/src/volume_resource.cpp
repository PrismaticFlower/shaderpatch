
#include "volume_resource.hpp"
#include "ucfb_writer.hpp"

#include <cmath>
#include <utility>

#include <d3d9.h>

namespace sp {

const std::uint32_t volume_resource_format = D3DFMT_R3G3B2;

namespace {

constexpr auto resource_block_size = 0x4000;
constexpr auto resource_max_blocks = 0x7fff;
constexpr auto resource_max_byte_size = resource_block_size * resource_max_blocks;

auto pack_size(const std::uint32_t size) noexcept
   -> std::pair<std::uint32_t, std::array<std::int16_t, 2>>
{
   Expects(size <= resource_max_byte_size);

   if (size <= resource_block_size)
      return {size, {gsl::narrow_cast<std::int16_t>(size), 1}};

   const auto blocks = gsl::narrow_cast<std::int16_t>(
      std::ceil(static_cast<double>(size) / resource_block_size));

   return {static_cast<std::uint32_t>(resource_block_size * blocks),
           {resource_block_size, blocks}};
}

}

void save_volume_resource(const std::string& output_path, std::string_view name,
                          Volume_resource_type type, gsl::span<const std::byte> data)
{
   if (data.size() > resource_max_byte_size) {
      throw std::runtime_error{"Resource is too large to store!"};
   }

   using namespace std::literals;

   auto file = ucfb::open_file_for_output(output_path);

   ucfb::Writer root_writer{file};
   auto writer = root_writer.emplace_child("tex_"_mn);

   std::string prefix;

   if (type == Volume_resource_type::texture) prefix = "_SP_TEXTURE_"s;
   if (type == Volume_resource_type::fx_config) prefix = "_SP_FXCFG_"s;

   writer.emplace_child("NAME"_mn).write(prefix += name);

   // write texture format list
   {
      auto info_writer = writer.emplace_child("INFO"_mn);
      info_writer.write(std::uint32_t{1}, volume_resource_format);
   }

   // write texture format chunk
   {
      auto fmt_writer = writer.emplace_child("FMT_"_mn);

      const auto [data_padded_size, resolution] =
         pack_size(gsl::narrow_cast<std::uint32_t>(data.size()));

      // write texture format info
      {
         auto info_writer = fmt_writer.emplace_child("INFO"_mn);

         constexpr std::uint32_t volume_texture_id = 1795;

         info_writer.write(volume_resource_format);
         info_writer.write_unaligned(type);
         info_writer.write_unaligned(resolution);
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

            info_writer.write(std::uint32_t{0}); // Write mip level index.
            info_writer.write(data_padded_size); // Write mip level size.
         }

         // write level body
         {
            auto body_writer = lvl_writer.emplace_child("BODY"_mn);

            body_writer.write_unaligned(data);
            body_writer.pad(
               gsl::narrow_cast<std::uint32_t>(data_padded_size - data.size()));
         }
      }
   }
}

}
