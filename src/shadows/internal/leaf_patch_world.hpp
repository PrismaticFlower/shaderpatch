#pragma once

#include "com_ptr.hpp"
#include "texture_table.hpp"

#include "../../game_support/leaf_patches.hpp"
#include "../frustum.hpp"

#include <span>
#include <vector>

#include <d3d11_2.h>

namespace sp::shadows {

struct Leaf_patch_class {
   std::vector<Bounding_sphere> bounding_spheres;
   std::vector<std::array<glm::vec4, 3>> transforms;

   UINT instance_count = 0;
   UINT index_count = 0;
   INT base_vertex = 0;
   UINT start_instance = 0;

   std::uint32_t texture_index = 0;
};

struct Leaf_patch_world {
   explicit Leaf_patch_world(ID3D11Device& device);

   UINT max_total_particles = 0;
   UINT max_instance_particles = 0;
   UINT max_total_instances = 0;

   Com_ptr<ID3D11Buffer> index_buffer;
   Com_ptr<ID3D11Buffer> vertex_buffer;
   Com_ptr<ID3D11Buffer> constants_buffer;
   Com_ptr<ID3D11Buffer> instances_buffer;

   std::vector<Leaf_patch_class> classes;

   void update_from_game(ID3D11Device& device, ID3D11DeviceContext2& dc,
                         Texture_table& textures) noexcept;

private:
   std::vector<game_support::Leaf_patch> game_leaf_patches;
   std::vector<game_support::structures::LeafPatchClass_data*> game_leaf_patch_classes;
   std::vector<game_support::structures::LeafPatchClass_data*> previous_game_leaf_patch_classes;
};

};