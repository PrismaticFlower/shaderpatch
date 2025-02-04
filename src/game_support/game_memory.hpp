#pragma once

namespace sp::game_support {

struct Game_memory {
   void* leaf_patch_list = nullptr;

   bool is_debug_executable = false;
};

auto get_game_memory() noexcept -> const Game_memory&;

}