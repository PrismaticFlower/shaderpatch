#pragma once

#include "shader_input_type.hpp"

#include <string>

#include <d3d11_1.h>

namespace sp::core {

struct Shader_input_element {
   std::string semantic_name;
   UINT semantic_index;
   Shader_input_type input_type;
};

}
