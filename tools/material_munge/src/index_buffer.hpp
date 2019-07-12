#pragma once

#include "ucfb_reader.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace sp {

template<typename Index>
using Basic_index_buffer = std::vector<std::array<Index, 3>>;

using Index_buffer_16 = Basic_index_buffer<std::uint16_t>;

using Index_buffer_32 = Basic_index_buffer<std::uint32_t>;

auto create_index_buffer(ucfb::Reader_strict<"IBUF"_mn> ibuf) -> Index_buffer_16;

auto shrink_index_buffer(const Index_buffer_32& fat_ibuf) -> Index_buffer_16;

}
