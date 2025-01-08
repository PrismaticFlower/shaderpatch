#pragma once

#include <array>
#include <cstddef>

#include <glm/glm.hpp>

namespace sp::shadows {

struct Object_instance {
   std::uint32_t name_hash = 0;
   std::uint32_t layer_name_hash = 0;

   std::size_t game_model_index = 0;

   bool from_child_lvl = false;

   std::array<glm::vec3, 3> rotation;
   glm::vec3 positionWS;

   bool operator==(const Object_instance&) const noexcept = default;
};

}