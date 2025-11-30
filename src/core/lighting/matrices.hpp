#pragma once

#include <glm/glm.hpp>

namespace sp::core {

struct Light_cluster_matrices {
   glm::mat4 view_matrix;
   glm::mat4 proj_matrix;
   glm::mat4 view_proj_matrix;
};

}