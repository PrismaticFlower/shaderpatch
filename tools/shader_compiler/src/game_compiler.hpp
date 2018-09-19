#pragma once

#include "com_ptr.hpp"
#include "compiler_helpers.hpp"
#include "preprocessor_defines.hpp"
#include "shader_flags.hpp"

#include <array>
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <d3dcompiler.h>
#include <nlohmann/json.hpp>

namespace sp {

struct Shader_variation;

class Game_compiler {
public:
   Game_compiler(DWORD compiler_flags, nlohmann::json definition,
                 const std::filesystem::path& definition_path,
                 const std::filesystem::path& source_file_dir,
                 const std::filesystem::path& output_dir);

   Game_compiler(const Game_compiler&) = delete;
   Game_compiler& operator=(const Game_compiler&) = delete;

   Game_compiler(Game_compiler&&) = delete;
   Game_compiler& operator=(Game_compiler&&) = delete;

private:
   struct Vertex_shader_ref {
      Shader_flags flags;
      std::uint32_t index;
   };

   using Pixel_shader_ref = std::uint32_t;

   struct Pass {
      Pass_flags flags;
      std::vector<Vertex_shader_ref> vs_shaders;
      std::uint32_t ps_index;
   };

   struct State {
      std::uint32_t id{};
      std::vector<Pass> passes;
   };

   void save(const std::filesystem::path& output_path);

   State compile_state(const nlohmann::json& state_def,
                       const Preprocessor_defines& global_defines,
                       std::array<bool, 4> srgb_state);

   Pass compile_pass(const nlohmann::json& pass_def, std::string_view state_name,
                     const Preprocessor_defines& state_defines,
                     std::array<bool, 4> srgb_state);

   auto compile_vertex_shader(const std::string& entry_point, const std::string& target,
                              const Shader_variation& variation,
                              const Preprocessor_defines& pass_defines,
                              std::string_view state_name,
                              std::array<bool, 4> srgb_state) -> Vertex_shader_ref;

   auto compile_pixel_shader(const std::string& entry_point, const std::string& target,
                             const Preprocessor_defines& pass_defines,
                             std::string_view state_name,
                             std::array<bool, 4> srgb_state) -> Pixel_shader_ref;

   std::filesystem::path _source_path;
   DWORD _compiler_flags;

   std::string _render_type;
   std::string _source;

   std::vector<std::vector<DWORD>> _vs_shaders;
   std::vector<std::vector<DWORD>> _ps_shaders;

   std::vector<State> _states;
};
}
