#pragma once

#include <cstddef>

namespace sp::shadows {

struct Game_model {
   std::size_t lod0_index = 0;
   std::size_t lod1_index = 0;
   std::size_t lod2_index = 0;
   std::size_t lowd_index = 0;
};

}