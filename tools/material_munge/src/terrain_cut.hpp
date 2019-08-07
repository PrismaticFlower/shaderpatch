#pragma once

#include <array>
#include <optional>
#include <vector>

#include <glm/glm.hpp>

namespace sp {

struct Terrain_vertex;

struct Terrain_cut {
   glm::vec3 centre;
   float radius;
   std::vector<glm::vec4> planes;

   void apply(std::vector<std::array<Terrain_vertex, 3>>& tris) const noexcept;
};

}
