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

namespace ucfb {
class Writer;
}

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
   struct Compiled_generic_entrypoint {
      std::map<std::uint32_t, Com_ptr<ID3DBlob>> variations;
      std::vector<std::string> static_flag_names;
   };

   struct Compiled_vertex_variation {
      std::vector<shader::Input_element> input_layout;
      Com_ptr<ID3DBlob> bytecode;
   };

   struct Compiled_vertex_entrypoint {
      std::map<std::pair<Vertex_shader_flags, std::uint32_t>, Compiled_vertex_variation> variations;
      std::vector<std::string> static_flag_names;
   };

   struct Compiled_pixel_entrypoint {
      std::map<std::pair<Pixel_shader_flags, std::uint32_t>, Com_ptr<ID3DBlob>> variations;
      std::vector<std::string> static_flag_names;
   };

   struct Assembled_state {
      std::string vs_entrypoint;
      std::uint32_t vs_static_flags;

      std::string ps_entrypoint;
      std::uint32_t ps_static_flags;

      std::string ps_oit_entrypoint;
      std::uint32_t ps_oit_static_flags;

      std::optional<std::string> hs_entrypoint;
      std::uint32_t hs_static_flags;

      std::optional<std::string> ds_entrypoint;
      std::uint32_t ds_static_flags;

      std::optional<std::string> gs_entrypoint;
      std::uint32_t gs_static_flags;
   };

   struct Assembled_rendertype {
      std::unordered_map<std::string, Assembled_state> states;
   };

   void save(const std::filesystem::path& output_path) const;

   void compile_entrypoints(
      const std::unordered_map<std::string, shader::Entrypoint>& entrypoints,
      const std::unordered_map<std::string, std::vector<shader::Input_element>>& input_layouts);

   void compile_generic_entrypoint(
      const std::pair<std::string, shader::Entrypoint>& entrypoint,
      const std::string& shader_define, const std::string& shader_profile,
      std::unordered_map<std::string, Compiled_generic_entrypoint>& out_entrypoints);

   void compile_vertex_entrypoint(
      const std::pair<std::string, shader::Entrypoint>& entrypoint,
      const std::unordered_map<std::string, std::vector<shader::Input_element>>& input_layouts);

   void compile_pixel_entrypoint(const std::pair<std::string, shader::Entrypoint>& entrypoint);

   auto compile_generic_shader(const std::string& entry_point,
                               Preprocessor_defines defines,
                               const std::string& shader_define,
                               const std::string& shader_profile) -> Com_ptr<ID3DBlob>;

   auto compile_vertex_shader(const std::string& entry_point,
                              Preprocessor_defines defines) -> Com_ptr<ID3DBlob>;

   auto compile_pixel_shader(const std::string& entry_point,
                             Preprocessor_defines defines) -> Com_ptr<ID3DBlob>;

   void assemble_rendertypes(
      const std::unordered_map<std::string, shader::Rendertype>& rendertypes);

   void write_entrypoints(
      ucfb::Writer& writer,
      const std::unordered_map<std::string, Compiled_generic_entrypoint>& entrypoints) const;

   void write_entrypoints(
      ucfb::Writer& writer,
      const std::unordered_map<std::string, Compiled_vertex_entrypoint>& entrypoints) const;

   void write_entrypoints(
      ucfb::Writer& writer,
      const std::unordered_map<std::string, Compiled_pixel_entrypoint>& entrypoints) const;

   DWORD _compiler_flags;

   std::string _group_name;
   std::filesystem::path _source_path;

   std::unordered_map<std::string, Compiled_generic_entrypoint> _compute_entrypoints;
   std::unordered_map<std::string, Compiled_vertex_entrypoint> _vertex_entrypoints;
   std::unordered_map<std::string, Compiled_pixel_entrypoint> _pixel_entrypoints;
   std::unordered_map<std::string, Compiled_generic_entrypoint> _hull_entrypoints;
   std::unordered_map<std::string, Compiled_generic_entrypoint> _domain_entrypoints;
   std::unordered_map<std::string, Compiled_generic_entrypoint> _geometry_entrypoints;

   std::unordered_map<std::string, Assembled_rendertype> _rendertypes;
};
}
