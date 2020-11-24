#pragma once

#include "common.hpp"
#include "preprocessor_defines.hpp"
#include "shader_flags.hpp"
#include "static_flags.hpp"
#include "vertex_input_layout.hpp"

#include <string>
#include <vector>

namespace sp::shader {

struct Entrypoint_vertex_state {
   bool use_custom_input_layout = false;
   std::vector<Vertex_input_element> custom_input_layout;

   Vertex_generic_input_state generic_input_state;
};

struct Entrypoint_description {
   std::string function_name;
   std::string source_name;

   Stage stage;

   Entrypoint_vertex_state vertex_state;

   Static_flags static_flags;
   std::vector<Preprocessor_define> preprocessor_defines;
};

}
