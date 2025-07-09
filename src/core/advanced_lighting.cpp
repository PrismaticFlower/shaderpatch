#include "advanced_lighting.hpp"

namespace sp::core {

void Advanced_lighting::update()
{
   glm::vec3 global_light2_dir;

   game_support::acquire_global_lights(global_light1_dir, global_light2_dir);
}

}