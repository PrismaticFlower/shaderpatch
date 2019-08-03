
#include "volume_resource.hpp"
#include "memory_mapped_file.hpp"
#include "ucfb_reader.hpp"
#include "ucfb_writer.hpp"

#include <cmath>
#include <utility>

#include <d3d9.h>

namespace sp {

const std::int32_t volume_resource_format = D3DFMT_L8;

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

void save_volume_resource(const std::filesystem::path& output_path,
                          const std::string_view name, const Volume_resource_type type,
                          gsl::span<const std::byte> data)
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

      const auto payload_size = sizeof(Volume_resource_header) + data.size();
      const auto [padded_payload_size, resolution] =
         pack_size(gsl::narrow_cast<std::uint32_t>(payload_size));

      // write texture format info
      {
         auto info_writer = fmt_writer.emplace_child("INFO"_mn);

         constexpr std::uint32_t volume_texture_id = 1795;

         info_writer.write(volume_resource_format);      // Format
         info_writer.write_unaligned(resolution[0]);     // Width
         info_writer.write_unaligned(resolution[1]);     // Height
         info_writer.write_unaligned(std::uint16_t{1});  // Depth
         info_writer.write_unaligned(std::uint16_t{1});  // Mipcount
         info_writer.write_unaligned(volume_texture_id); // Texturetype
      }

      // write texture level
      {
         auto face_writer = fmt_writer.emplace_child("FACE"_mn);
         auto lvl_writer = face_writer.emplace_child("LVL_"_mn);

         // write level info
         {
            auto info_writer = lvl_writer.emplace_child("INFO"_mn);

            info_writer.write(std::uint32_t{0}); // Mip level index.
            info_writer.write(payload_size);     // Mip level size.
         }

         // write level body
         {
            auto body_writer = lvl_writer.emplace_child("BODY"_mn);

            Volume_resource_header header;
            header.type = type;
            header.payload_size = gsl::narrow_cast<std::uint32_t>(data.size());

            body_writer.write_unaligned(header);
            body_writer.write_unaligned(data);
            body_writer.pad(gsl::narrow_cast<std::uint32_t>(padded_payload_size -
                                                            payload_size));
         }
      }
   }
}

auto load_volume_resource(const std::filesystem::path& path)
   -> std::pair<Volume_resource_header, std::vector<std::byte>>
{
   const auto file_mapping =
      win32::Memeory_mapped_file{path, win32::Memeory_mapped_file::Mode::read};

   ucfb::Reader reader{file_mapping.bytes()};

   auto tex = reader.read_child_strict<"tex_"_mn>();
   tex.read_child_strict<"NAME"_mn>();
   tex.read_child_strict<"INFO"_mn>();

   {
      auto fmt = tex.read_child_strict<"FMT_"_mn>();
      fmt.read_child_strict<"INFO"_mn>();

      {
         auto face = fmt.read_child_strict<"FACE"_mn>();

         {
            auto lvl = face.read_child_strict<"LVL_"_mn>();

            lvl.read_child_strict<"INFO"_mn>();

            {
               auto body = lvl.read_child_strict<"BODY"_mn>();

               const auto header = body.read_unaligned<Volume_resource_header>();
               const auto data =
                  body.read_array_unaligned<std::byte>(header.payload_size);

               return {header, {data.cbegin(), data.cend()}};
            }
         }
      }
   }
}
}
