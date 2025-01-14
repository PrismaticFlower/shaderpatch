#include "bounding_box.hpp"

#include <glm/gtc/quaternion.hpp>

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

   // Convert the rotation to a normalized quaternion, this fixes some issues
   // with entities that have a bad rotation matrix.

   glm::quat rotation{glm::mat3{
      glm::vec3(transform[0]),
      glm::vec3(transform[1]),
      glm::vec3(transform[2]),
   }};

   rotation = glm::normalize(rotation);

   for (glm::vec3& v : vertices) {
      v = v * rotation;
   }

   Bounding_box new_box{vertices[0], vertices[0]};

   for (std::size_t i = 1; i < vertices.size(); ++i) {
      new_box.min = glm::min(new_box.min, vertices[i]);
      new_box.max = glm::max(new_box.max, vertices[i]);
   }

   new_box.min += glm::vec3{transform[0].w, transform[1].w, transform[2].w};
   new_box.max += glm::vec3{transform[0].w, transform[1].w, transform[2].w};

   return new_box;
}
}