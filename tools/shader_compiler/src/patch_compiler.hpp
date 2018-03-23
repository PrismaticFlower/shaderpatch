#pragma once

#include "shader_flags.hpp"
#include "shader_variations.hpp"

#include <string_view>
#include <unordered_map>
#include <vector>

#include <boost/filesystem.hpp>

#include <d3dcompiler.h>
#include <nlohmann/json.hpp>

namespace sp {

class Patch_compiler {
public:
   Patch_compiler() = default;

   Patch_compiler(std::string definition_path);

   void optimize_permutations();

   void save(std::string_view output_path) const;

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

   auto compile_state(const nlohmann::json& state_def) -> State;

   auto compile_vertex_shader(const std::string& entry_point,
                              const std::vector<D3D_SHADER_MACRO>& defines)
      -> std::size_t;

   auto compile_pixel_shader(const std::string& entry_point,
                             const std::vector<D3D_SHADER_MACRO>& defines)
      -> std::size_t;

   std::string _render_type;
   boost::filesystem::path _source_path;

   std::vector<Shader_variation> _variations;

   std::vector<std::vector<DWORD>> _vs_shaders;
   std::unordered_map<std::size_t, std::size_t> _vs_cache;

   std::vector<std::vector<DWORD>> _ps_shaders;
   std::unordered_map<std::size_t, std::size_t> _ps_cache;

   std::vector<State> _states;
};
}
