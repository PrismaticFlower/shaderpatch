#pragma once

#include "static_flags.hpp"
#include "vertex_input_layout.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace sp::shader {

struct Rendertype_state_description {
   std::string group_name;

   std::string vs_entrypoint;
   std::uint64_t vs_static_flags;
   Vertex_generic_input_state vs_input_state;
   Static_flags vs_static_flag_names;

   std::string ps_entrypoint;
   std::optional<std::string> ps_al_entrypoint;
   std::uint64_t ps_static_flags;
   Static_flags ps_static_flag_names;

   std::optional<std::string> ps_oit_entrypoint;
   std::optional<std::string> ps_al_oit_entrypoint;
   std::uint64_t ps_oit_static_flags;
   Static_flags ps_oit_static_flag_names;
};

}
