#include "bounding_box.hpp"

namespace sp::shadows {

auto operator*(const Bounding_box& box,
               const std::array<glm::vec4, 3>& transform) noexcept -> Bounding_box
{
   std::array<glm::vec3, 8> vertices = {
      // top corners
      glm::vec3{box.max.x, box.max.y, box.max.z},
      glm::vec3{box.min.x, box.max.y, box.max.z},
      glm::vec3{box.min.x, box.max.y, box.min.z},
      glm::vec3{box.max.x, box.max.y, box.min.z},
      // bottom corners
      glm::vec3{box.max.x, box.min.y, box.max.z},
      glm::vec3{box.min.x, box.min.y, box.max.z},
      glm::vec3{box.min.x, box.min.y, box.min.z},
      glm::vec3{box.max.x, box.min.y, box.min.z},
   };

   for (glm::vec3& v : vertices) {
      v.x = glm::dot(transform[0], glm::vec4{v, 1.0f});
      v.y = glm::dot(transform[1], glm::vec4{v, 1.0f});
      v.z = glm::dot(transform[2], glm::vec4{v, 1.0f});
   }

   Bounding_box new_box{vertices[0], vertices[0]};

   for (std::size_t i = 1; i < vertices.size(); ++i) {
      new_box.min = glm::min(new_box.min, vertices[i]);
      new_box.max = glm::max(new_box.max, vertices[i]);
   }

   return new_box;
}
}