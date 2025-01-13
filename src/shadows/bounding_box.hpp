#pragma once

#include <array>

#include <glm/glm.hpp>

namespace sp::shadows {

struct Bounding_box {
   glm::vec3 min;
   glm::vec3 max;
};

auto operator*(const Bounding_box& box,
               const std::array<glm::vec4, 3>& transform) noexcept -> Bounding_box;

}