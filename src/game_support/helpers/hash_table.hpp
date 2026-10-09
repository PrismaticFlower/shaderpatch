#pragma once

#include <cstdint>

namespace sp::game_support {

auto hash_table_find(std::uint32_t name_hash, void* hash_table,
                     std::uint32_t total_size) -> void*;

}
