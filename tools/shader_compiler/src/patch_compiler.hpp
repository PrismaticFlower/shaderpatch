#pragma once

#include "shader_description.hpp"
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
   Patch_compiler(DWORD compiler_flags, nlohmann::json definition,
                  const std::filesystem::path& definition_path,
                  const std::filesystem::path& source_file_dir,
                  const std::filesystem::path& output_dir);

   Patch_compiler(const Patch_compiler&) = delete;
   Patch_compiler& operator=(const Patch_compiler&) = delete;

   Patch_compiler(Patch_compiler&&) = delete;
   Patch_compiler& operator=(Patch_compiler&&) = delete;

private:
   template<typename Stage_flag_type>
   struct Compiled_entrypoint {
      std::map<std::pair<Stage_flag_type, std::uint32_t>, Com_ptr<ID3DBlob>> variations;
      std::vector<std::string> static_flag_names;
   };

   using Compiled_vertex_entrypoint = Compiled_entrypoint<Vertex_shader_flags>;
   using Compiled_pixel_entrypoint = Compiled_entrypoint<Pixel_shader_flags>;

   struct Assembled_state {
      std::string vs_entrypoint;
      std::uint32_t vs_static_flags;

      std::string ps_entrypoint;
      std::uint32_t ps_static_flags;
   };

   struct Assembled_rendertype {
      std::unordered_map<std::string, Assembled_state> states;
   };

   void save(const std::filesystem::path& output_path) const;

   void compile_entrypoints(const std::unordered_map<std::string, shader::Entrypoint>& entrypoints);

   void compile_vertex_entrypoint(const std::pair<std::string, shader::Entrypoint>& entrypoint);

   void compile_pixel_entrypoint(const std::pair<std::string, shader::Entrypoint>& entrypoint);

   auto compile_vertex_shader(const std::string& entry_point,
                              Preprocessor_defines defines) -> Com_ptr<ID3DBlob>;

   auto compile_pixel_shader(const std::string& entry_point,
                             Preprocessor_defines defines) -> Com_ptr<ID3DBlob>;

   void assemble_rendertypes(
      const std::unordered_map<std::string, shader::Rendertype>& rendertypes);

   DWORD _compiler_flags;

   std::string _group_name;
   std::filesystem::path _source_path;

   std::unordered_map<std::string, Compiled_vertex_entrypoint> _vertex_entrypoints;
   std::unordered_map<std::string, Compiled_pixel_entrypoint> _pixel_entrypoints;

   std::unordered_map<std::string, Assembled_rendertype> _rendertypes;
};
}
