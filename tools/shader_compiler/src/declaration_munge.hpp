#pragma once

#include "com_ptr.hpp"
#include "compiler_helpers.hpp"
#include "shader_flags.hpp"
#include "shader_metadata.hpp"

#include <array>
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace sp {

struct Shader_variation;

class Declaration_munge {
public:
   Declaration_munge(const std::filesystem::path& declaration_path,
                     const std::filesystem::path& output_dir);

   Declaration_munge(const Declaration_munge&) = delete;
   Declaration_munge& operator=(const Declaration_munge&) = delete;

   Declaration_munge(Declaration_munge&&) = delete;
   Declaration_munge& operator=(Declaration_munge&&) = delete;

private:
   struct Shader_ref {
      std::uint32_t index;
      Shader_flags flags;
   };

   struct Pass {
      Pass_flags flags;
      std::vector<Shader_ref> shader_refs;
   };

   struct State {
      std::uint32_t id{};
      std::vector<Pass> passes;
   };

   void save(const std::filesystem::path& output_path, const std::string& input_filename);

   State munge_state(const nlohmann::json& state_def, std::array<bool, 4> srgb_state);

   Pass munge_pass(const nlohmann::json& pass_def, std::array<bool, 4> srgb_state);

   Rendertype _rendertype;
   std::string _rendertype_name;

   std::vector<Shader_metadata> _shaders;
   std::vector<State> _states;
};
}
