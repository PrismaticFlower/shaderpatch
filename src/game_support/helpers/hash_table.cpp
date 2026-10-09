#include "hash_table.hpp"

namespace sp::game_support {

auto hash_table_find(std::uint32_t name_hash, void* hash_table,
                     std::uint32_t total_size) -> void*
{
   if (name_hash == 0) return nullptr;

   const std::uint32_t size = total_size >> 1u;

   const std::uint32_t* hashes = reinterpret_cast<std::uint32_t*>(hash_table);
   void** values = reinterpret_cast<void**>(hash_table) + size;

   std::uint32_t index = name_hash & (size - 1u);

   while (true) {
      if (hashes[index] == name_hash) return values[index];
      if (hashes[index] == 0) return nullptr;

      if (index == 0) index = size;

      index -= 1;
   }
}

}
