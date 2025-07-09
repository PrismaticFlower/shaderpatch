#pragma once

#include "../game_support/light_list.hpp"

#include <glm/glm.hpp>

namespace sp::core {

struct Advanced_lighting {
   glm::vec3 global_light1_dir;

   void update();
};

}
