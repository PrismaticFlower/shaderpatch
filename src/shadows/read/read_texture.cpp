#include "read_texture.hpp"

#include "../shadow_world.hpp"

#include <optional>

#pragma warning(disable : 4063)

namespace sp::shadows {

namespace {

enum class Texture_type : std::uint32_t { _2d = 1, cube = 2, _3d = 3 };

struct Texture_info {
   std::uint32_t format;
   std::uint16_t width;
   std::uint16_t height;
   std::uint16_t depth;
   std::uint16_t mipmap_count;
   std::uint32_t type_detail_bias;
};

static_assert(sizeof(Texture_info) == 16);

auto read_body(ucfb::File_reader_child& body, std::uint32_t body_size) -> Texture_hash
{
   std::array<std::byte, 1024> chunk_buffer{};

   const std::uint32_t chunk_count = body_size / chunk_buffer.size();

   Texture_hasher hasher;

   for (std::uint32_t i = 0; i < chunk_count; ++i) {
      body.read_bytes(chunk_buffer);
      hasher.process_bytes(chunk_buffer);
   }

   const std::uint32_t tail_size = body_size - (chunk_count * chunk_buffer.size());

   if (tail_size != 0) {
      std::span<std::byte> tail{chunk_buffer};

      tail = tail.subspan(0, tail_size);

      body.read_bytes(tail);
      hasher.process_bytes(tail);
   }

   return hasher.result();
}

auto read_face(ucfb::File_reader_child& face) -> std::optional<Texture_hash>
{
   while (face) {
      auto child = face.read_child();

      switch (child.magic_number()) {
      case "LVL_"_mn: {
         std::uint32_t body_size = 0;

         while (child) {
            auto lvl_child = child.read_child();

            switch (lvl_child.magic_number()) {
            case "INFO"_mn: {
               const std::uint32_t mip_level = lvl_child.read<std::uint32_t>();

               if (mip_level != 0) continue;

               body_size = lvl_child.read<std::uint32_t>();
            } break;
            case "BODY"_mn: {
               return read_body(lvl_child, body_size);
            } break;
            }
         }

      } break;
      }
   }

   return std::nullopt;
}

auto read_format(ucfb::File_reader_child& fmt) -> std::optional<Texture_hash>
{
   while (fmt) {
      auto child = fmt.read_child();

      switch (child.magic_number()) {
      case "INFO"_mn: {
         const Texture_info info = child.read<Texture_info>();
         const Texture_type type{info.type_detail_bias & 0xffu};

         if (type != Texture_type::_2d) return std::nullopt;
      } break;
      case "FACE"_mn: {
         return read_face(child);
      } break;
      }
   }

   return std::nullopt;
}

}

void read_texture(ucfb::File_reader_child& tex)
{
   std::string name;

   while (tex) {
      auto child = tex.read_child();

      switch (child.magic_number()) {
      case "NAME"_mn: {
         name = child.read_string();
      } break;
      case "FMT_"_mn: {
         const std::optional<Texture_hash> hash = read_format(child);

         if (hash) {
            shadow_world.add_texture({
               .name = name,
               .hash = *hash,
            });
         }
      } break;
      }
   }
}

}