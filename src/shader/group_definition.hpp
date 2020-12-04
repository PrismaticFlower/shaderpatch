#pragma once

#include "common.hpp"
#include "preprocessor_defines.hpp"
#include "static_flags.hpp"
#include "vertex_input_layout.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <absl/container/flat_hash_map.h>

namespace sp::shader {

struct Group_definition {
   struct Entrypoint {
      struct Vertex_state {
         std::string input_layout;
         Vertex_generic_input_state generic_input;
      };

      std::optional<std::string> function_name;

      Stage stage;

      Vertex_state vertex_state;

      std::vector<std::string> static_flags;
      std::vector<Preprocessor_define> preprocessor_defines;
   };

   struct Rendertype {
      absl::flat_hash_map<std::string, bool> static_flags;
   };

   struct State {
      std::string vs_entrypoint;
      absl::flat_hash_map<std::string, bool> vs_static_flags;

      std::string ps_entrypoint;
      absl::flat_hash_map<std::string, bool> ps_static_flags;

      std::optional<std::string> ps_oit_entrypoint;
      absl::flat_hash_map<std::string, bool> ps_oit_static_flags;

      std::optional<std::string> hs_entrypoint;
      absl::flat_hash_map<std::string, bool> hs_static_flags;

      std::optional<std::string> ds_entrypoint;
      absl::flat_hash_map<std::string, bool> ds_static_flags;

      std::optional<std::string> gs_entrypoint;
      absl::flat_hash_map<std::string, bool> gs_static_flags;
   };

   std::string group_name;
   std::string source_name;
   std::filesystem::file_time_type last_write_time;

   absl::flat_hash_map<std::string, std::vector<Vertex_input_element>> input_layouts;
   absl::flat_hash_map<std::string, Entrypoint> entrypoints;
   absl::flat_hash_map<std::string, Rendertype> rendertypes;
   absl::flat_hash_map<std::string, State> states;
};

auto load_group_definitions(const std::filesystem::path& source_path) noexcept
   -> std::vector<Group_definition>;

}
