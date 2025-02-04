#pragma once

namespace sp::game_support {

namespace structures {

struct LeafPatchListNode;

}

struct Game_memory {
   structures::LeafPatchListNode* leaf_patch_list = nullptr;

   /// @brief Pointer to float
   float* view_near_plane = nullptr;

   /// @brief Pointer to float[2]
   float* near_scene_fade_start = nullptr;

   /// @brief Pointer to float[2]
   float* near_scene_fade_end = nullptr;

   /// @brief Pointer to float[2]
   float* far_scene_range = nullptr;

   /// @brief Pointer to bool
   bool* far_scene_enabled = nullptr;

   bool is_debug_executable = false;
};

auto get_game_memory() noexcept -> const Game_memory&;

}