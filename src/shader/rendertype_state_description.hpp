#pragma once

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

   std::string ps_entrypoint;
   std::uint64_t ps_static_flags;

   std::optional<std::string> ps_oit_entrypoint;
   std::uint64_t ps_oit_static_flags;

   std::optional<std::string> hs_entrypoint;
   std::uint64_t hs_static_flags;

   std::optional<std::string> ds_entrypoint;
   std::uint64_t ds_static_flags;

   std::optional<std::string> gs_entrypoint;
   std::uint64_t gs_static_flags;
};

}
