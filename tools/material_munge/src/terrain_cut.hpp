#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>

namespace sp {

struct Terrain_cut {
   glm::vec3 centre;
   float radius;
   std::vector<glm::vec4> planes;
};
}
