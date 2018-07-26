#pragma once

#include "shader_flags.hpp"
#include "shader_variations.hpp"

#include <filesystem>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <d3dcompiler.h>
#include <nlohmann/json.hpp>

namespace sp {

class Patch_compiler {
public:
   Patch_compiler(nlohmann::json definition, const std::filesystem::path& definition_path,
                  const std::filesystem::path& source_file_dir,
                  const std::filesystem::path& output_dir);

   Patch_compiler(const Patch_compiler&) = delete;
   Patch_compiler& operator=(const Patch_compiler&) = delete;

   Patch_compiler(Patch_compiler&&) = delete;
   Patch_compiler& operator=(Patch_compiler&&) = delete;

private:
   struct Shader {
      Shader_flags flags;

      std::size_t vs_index;
      std::size_t ps_index;
   };

   struct State {
      std::string name;

      std::vector<Shader> shaders;
   };

   void optimize_and_linearize_permutations();

   void save(const std::filesystem::path& output_path) const;

   auto compile_state(const nlohmann::json& state_def,
                      const Preprocessor_defines& global_defines) -> State;

   auto compile_vertex_shader(const std::string& entry_point,
                              const std::vector<D3D_SHADER_MACRO>& defines)
      -> std::size_t;

   auto compile_pixel_shader(const std::string& entry_point,
                             const std::vector<D3D_SHADER_MACRO>& defines)
      -> std::size_t;

   std::string _render_type;
   std::filesystem::path _source_path;

   std::vector<Shader_variation> _variations;
   std::vector<Shader_variation> _ps_variations;

   std::mutex _vs_mutex;
   std::vector<std::pair<std::size_t, std::vector<DWORD>>> _vs_shaders;
   std::unordered_map<Shader_cache_index, std::size_t> _vs_cache;

   std::mutex _ps_mutex;
   std::vector<std::pair<std::size_t, std::vector<DWORD>>> _ps_shaders;
   std::unordered_map<Shader_cache_index, std::size_t> _ps_cache;

   std::mutex _states_mutex;
   std::vector<State> _states;
};
}
