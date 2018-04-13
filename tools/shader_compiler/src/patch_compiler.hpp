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
   Patch_compiler(const boost::filesystem::path& definition_path,
                  const boost::filesystem::path& output_path);

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

   void optimize_permutations();

   void save() const;

   auto compile_state(const nlohmann::json& state_def) -> State;

   auto compile_vertex_shader(const std::string& entry_point,
                              const std::vector<D3D_SHADER_MACRO>& defines)
      -> std::size_t;

   auto compile_pixel_shader(const std::string& entry_point,
                             const std::vector<D3D_SHADER_MACRO>& defines)
      -> std::size_t;

   std::string _render_type;
   boost::filesystem::path _source_path;
   boost::filesystem::path _output_path;

   std::vector<Shader_variation> _variations;

   std::vector<std::vector<DWORD>> _vs_shaders;
   std::unordered_map<std::size_t, std::size_t> _vs_cache;

   std::vector<std::vector<DWORD>> _ps_shaders;
   std::unordered_map<std::size_t, std::size_t> _ps_cache;

   std::vector<State> _states;
};
}
