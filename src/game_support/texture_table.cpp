#include "texture_table.hpp"
#include "game_memory.hpp"

#include "helpers/hash_table.hpp"

#include "../direct3d/base_texture.hpp"

namespace sp::game_support {

namespace {

const std::ptrdiff_t name_hash_offset = 0x4;
const std::ptrdiff_t d3d_texture_offset = 0x20;
const std::ptrdiff_t d3d_texture_offset_debug = 0x28;

}

auto lookup_texture(std::uint32_t name_hash, const bool srgb_view)
   -> ID3D11ShaderResourceView*
{
   const Game_memory& game_memory = get_game_memory();

   if (not game_memory.texture_table_total_size or not game_memory.texture_table) {
      return nullptr;
   }

   const char* texture = reinterpret_cast<const char*>(
      hash_table_find(name_hash, *game_memory.texture_table,
                      *game_memory.texture_table_total_size));

   if (not texture) return nullptr;

#ifndef NDEBUG
   std::uint32_t read_name_hash = 0;

   std::memcpy(&read_name_hash, texture + name_hash_offset, sizeof(read_name_hash));

   if (read_name_hash != name_hash) std::terminate();
#endif

   const std::ptrdiff_t texture_offset =
      game_memory.is_debug_executable ? d3d_texture_offset_debug : d3d_texture_offset;
   const d3d9::Base_texture* d3d_texture = nullptr;

   std::memcpy(&d3d_texture, texture + texture_offset, sizeof(d3d_texture));

   const core::Game_texture& game_texture = d3d_texture->get<core::Game_texture>();

   return srgb_view ? game_texture.srgb_srv.get() : game_texture.srv.get();
}

}
