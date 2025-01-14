#pragma once

#include <glm/glm.hpp>

namespace sp::shadows {

struct alignas(16) Draw_view_gpu_constants {
   glm::mat4 projection_matrix;
};

}