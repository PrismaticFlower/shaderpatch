#include "read_texture.hpp"

#include "../shadow_world.hpp"

#include <optional>

#pragma warning(disable : 4063)

#include <d3d9.h>

namespace sp::shadows {

namespace {

enum class Texture_type : std::uint32_t { _2d = 1, cube = 2, _3d = 3 };

struct Texture_info {
   D3DFORMAT format;
   std::uint16_t width;
   std::uint16_t height;
   std::uint16_t depth;
   std::uint16_t mipmap_count;
   std::uint32_t type_detail_bias;
};

static_assert(sizeof(Texture_info) == 16);

enum class Body_type { normal, l8, a8l8 };

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

auto read_body_l8(ucfb::File_reader_child& body, std::uint32_t body_size) -> Texture_hash
{
   std::array<std::byte, 1024> chunk_buffer{};
   std::array<std::array<std::byte, 4>, 1024> patched_buffer{};

   const std::uint32_t chunk_count = body_size / chunk_buffer.size();

   Texture_hasher hasher;

   for (std::uint32_t chunk_index = 0; chunk_index < chunk_count; ++chunk_index) {
      body.read_bytes(chunk_buffer);

      for (std::size_t i = 0; i < chunk_buffer.size(); ++i) {
         patched_buffer[i][0] = patched_buffer[i][1] = patched_buffer[i][2] =
            chunk_buffer[i];
         patched_buffer[i][3] = std::byte{0xffu};
      }

      hasher.process_bytes(std::as_bytes(std::span{patched_buffer}));
   }

   const std::uint32_t tail_size = body_size - (chunk_count * chunk_buffer.size());

   if (tail_size != 0) {
      std::span<std::byte> tail{chunk_buffer};
      std::span<std::array<std::byte, 4>> tail_patched{patched_buffer};

      tail = tail.subspan(0, tail_size);
      tail_patched = tail_patched.subspan(0, tail_size);

      body.read_bytes(tail);

      for (std::size_t i = 0; i < tail.size(); ++i) {
         tail_patched[i][0] = tail_patched[i][1] = tail_patched[i][2] = tail[i];
         tail_patched[i][3] = std::byte{0xffu};
      }

      hasher.process_bytes(std::as_bytes(tail_patched));
   }

   return hasher.result();
}

auto read_body_a8l8(ucfb::File_reader_child& body, std::uint32_t body_size) -> Texture_hash
{
   std::array<std::array<std::byte, 2>, 1024> chunk_buffer{};
   std::array<std::array<std::byte, 4>, 1024> patched_buffer{};

   const std::uint32_t chunk_count = (body_size / 2) / chunk_buffer.size();

   Texture_hasher hasher;

   for (std::uint32_t chunk_index = 0; chunk_index < chunk_count; ++chunk_index) {
      body.read_bytes(std::as_writable_bytes(std::span{chunk_buffer}));

      for (std::size_t i = 0; i < chunk_buffer.size(); ++i) {
         patched_buffer[i][0] = patched_buffer[i][1] = patched_buffer[i][2] =
            chunk_buffer[i][0];
         patched_buffer[i][3] = chunk_buffer[i][1];
      }

      hasher.process_bytes(std::as_bytes(std::span{patched_buffer}));
   }

   const std::uint32_t tail_size =
      (body_size / 2) - (chunk_count * chunk_buffer.size());

   if (tail_size != 0) {
      std::span<std::array<std::byte, 2>> tail{chunk_buffer};
      std::span<std::array<std::byte, 4>> tail_patched{patched_buffer};

      tail = tail.subspan(0, tail_size);
      tail_patched = tail_patched.subspan(0, tail_size);

      body.read_bytes(std::as_writable_bytes(tail));

      for (std::size_t i = 0; i < tail.size(); ++i) {
         tail_patched[i][0] = tail_patched[i][1] = tail_patched[i][2] =
            chunk_buffer[i][0];
         tail_patched[i][3] = chunk_buffer[i][1];
      }

      hasher.process_bytes(std::as_bytes(tail_patched));
   }

   return hasher.result();
}

auto read_face(ucfb::File_reader_child& face, const Body_type body_type)
   -> std::optional<Texture_hash>
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
               switch (body_type) {
               case Body_type::normal:
                  return read_body(lvl_child, body_size);
               case Body_type::l8:
                  return read_body_l8(lvl_child, body_size);
               case Body_type::a8l8:
                  return read_body_a8l8(lvl_child, body_size);
               }

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
   Body_type body_type = Body_type::normal;

   while (fmt) {
      auto child = fmt.read_child();

      switch (child.magic_number()) {
      case "INFO"_mn: {
         const Texture_info info = child.read<Texture_info>();
         const Texture_type type{info.type_detail_bias & 0xffu};

         if (type != Texture_type::_2d) {
            return std::nullopt;
         }

         if (info.format == D3DFMT_L8)
            body_type = Body_type::l8;
         else if (info.format == D3DFMT_A8L8)
            body_type = Body_type::a8l8;

      } break;
      case "FACE"_mn: {
         return read_face(child, body_type);
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