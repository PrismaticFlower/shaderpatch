#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

namespace sp::game_support {

namespace structures {

struct LeafPatchClass_data;

}

struct Leaf_patch {
   std::size_t class_index;
   float bounds_radius;
   std::array<glm::vec4, 3> transform;
};

/// @brief Acquire a list of leaf patch entities from the game. This directly reads the game's data structures and is **NOT** thread safe.
/// @param leaf_patches The output list of leaf patch entities.
/// @param leaf_patch_classes The leaf patch classes referenced by the the leaf patches.
void acquire_leaf_patches(std::vector<Leaf_patch>& leaf_patches,
                          std::vector<structures::LeafPatchClass_data*>& leaf_patch_classes) noexcept;

/// @brief Show a window that allows exploring the game's leaf patches. This directly reads the game's data structures and is **NOT** thread safe.
void show_leaf_patches_imgui() noexcept;

}
