#pragma once

#include "com_ptr.hpp"
#include "shader_flags.hpp"

#include <string_view>
#include <unordered_map>
#include <vector>

#include <d3dcompiler.h>
#include <nlohmann/json.hpp>

namespace sp {

struct Shader_variation;

class Game_compiler {
public:
   Game_compiler() = default;

   Game_compiler(std::string definition_path, std::string source_path);

   void save(std::string_view output_path);

private:
   enum class Pass_flags : std::uint32_t {
      none = 0,
      nolighting = 1,
      lighting = 2,

      notransform = 16,
      position = 32,
      normals = 64,       // position,normals
      binormals = 128,    // position, normals, binormals
      vertex_color = 256, // vertex color
      texcoords = 512     // texcoords
   };

   friend constexpr Pass_flags& operator|=(Pass_flags& l, const Pass_flags r) noexcept
   {
      return l = Game_compiler::Pass_flags{static_cast<std::uint32_t>(l) |
                                           static_cast<std::uint32_t>(r)};
   }

   static auto get_pass_flags(const nlohmann::json& pass_def) -> Pass_flags;

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

   State compile_state(const nlohmann::json& state_def,
                       const nlohmann::json& parent_metadata);

   Pass compile_pass(const nlohmann::json& pass_def,
                     const nlohmann::json& parent_metadata);

   auto compile_vertex_shader(const nlohmann::json& parent_metadata,
                              std::string_view entry_point, std::string_view target,
                              const Shader_variation& variation) -> Vertex_shader_ref;

   auto compile_pixel_shader(const nlohmann::json& parent_metadata,
                             std::string_view entry_point,
                             std::string_view target) -> Pixel_shader_ref;

   std::string _definition_path;
   std::string _source_path;
   std::string _render_type;
   std::string _source;

   std::vector<std::vector<DWORD>> _vs_shaders;
   std::unordered_map<std::size_t, Vertex_shader_ref> _vs_cache;

   std::vector<std::vector<DWORD>> _ps_shaders;
   std::unordered_map<std::size_t, Pixel_shader_ref> _ps_cache;

   std::vector<State> _states;
};
}
