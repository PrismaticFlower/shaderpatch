#pragma once

#include "shader_flags.hpp"

#include <tuple>
#include <vector>

namespace sp::shader {

inline auto get_base_declaration_vertex_shader_flags(const Pass_flags pass_flags) noexcept
{
   Vertex_shader_flags flags{};

   if (pass_flag_set(pass_flags, Pass_flags::position)) {
      flags |= Vertex_shader_flags::position;
   }

   if (pass_flag_set(pass_flags, Pass_flags::normal)) {
      flags |= Vertex_shader_flags::normal;
   }

   if (pass_flag_set(pass_flags, Pass_flags::tangents)) {
      flags |= Vertex_shader_flags::tangents;
   }

   if (pass_flag_set(pass_flags, Pass_flags::texcoords)) {
      flags |= Vertex_shader_flags::texcoords;
   }

   return flags;
}

inline auto get_declaration_shader_variations(const Pass_flags flags,
                                              const bool skinned) noexcept
   -> std::vector<std::tuple<Shader_flags, Vertex_shader_flags>>
{
   using namespace std::literals;

   std::vector<std::tuple<Shader_flags, Vertex_shader_flags>> variations;
   variations.reserve(36);

   const auto base_vertex_flags = get_base_declaration_vertex_shader_flags(flags);
   const bool vertex_color = pass_flag_set(flags, Pass_flags::vertex_color);
   const bool lighting = pass_flag_set(flags, Pass_flags::lighting);

   // Unskinned
   variations.emplace_back(Shader_flags::none, base_vertex_flags);

   // Unskinned Vertex colored
   if (vertex_color) {
      variations.emplace_back(Shader_flags::vertexcolor,
                              Vertex_shader_flags::color | base_vertex_flags);
   }

   // Skinned Defintions
   if (skinned) {
      // Skinned
      variations.emplace_back(Shader_flags::skinned,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags);
   }

   // Skinned Vertex colored
   if (skinned && vertex_color) {
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags);
   }

   // Unskinned Lighting Definitions
   if (lighting) {
      // Directional Light
      variations.emplace_back(Shader_flags::light_dir, base_vertex_flags);

      // Directional Lights + Point Light
      variations.emplace_back(Shader_flags::light_dir_point, base_vertex_flags);

      // Directional Lights + 2 Point Lights
      variations.emplace_back(Shader_flags::light_dir_point2, base_vertex_flags);

      // Directional Lights + 4 Point Lights
      variations.emplace_back(Shader_flags::light_dir_point4, base_vertex_flags);

      // Direction Lights + Spot Light
      variations.emplace_back(Shader_flags::light_dir_spot, base_vertex_flags);

      // Directional Lights + Point Light + Spot Light
      variations.emplace_back(Shader_flags::light_dir_point_spot, base_vertex_flags);

      // Directional Lights + 2 Point Lights + Spot Light
      variations.emplace_back(Shader_flags::light_dir_point2_spot, base_vertex_flags);
   }

   // Unskinned Lighting Vertexcolor Definitions
   if (lighting && vertex_color) {
      // Directional Light
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir,
                              Vertex_shader_flags::color | base_vertex_flags);

      // Directional Lights + Point Light
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir_point,
                              Vertex_shader_flags::color | base_vertex_flags);

      // Directional Lights + 2 Point Lights
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir_point2,
                              Vertex_shader_flags::color | base_vertex_flags);

      // Directional Lights + 4 Point Lights
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir_point4,
                              Vertex_shader_flags::color | base_vertex_flags);

      // Direction Lights + Spot Light
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir_spot,
                              Vertex_shader_flags::color | base_vertex_flags);

      // Directional Lights + Point Light + Spot Light
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir_point_spot,
                              Vertex_shader_flags::color | base_vertex_flags);

      // Directional Lights + 2 Point Lights + Spot Light
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir_point2_spot,
                              Vertex_shader_flags::color | base_vertex_flags);
   }

   // Skinned Lighting Definitions
   if (lighting && skinned) {
      // Directional Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags);

      // Directional Lights + Point Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir_point,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags);

      // Directional Lights + 2 Point Lights
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir_point2,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags);

      // Directional Lights + 4 Point Lights
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir_point4,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags);

      // Direction Lights + Spot Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir_spot,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags);

      // Directional Lights + Point Light + Spot Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir_point_spot,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags);

      // Directional Lights + 2 Point Lights + Spot Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir_point2_spot,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags);
   }

   // Skinned Lighting Vertexcolor Definitions
   if (lighting && skinned && vertex_color) {
      // Directional Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags);

      // Directional Lights + Point Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir_point,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags);

      // Directional Lights + 2 Point Lights
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir_point2,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags);

      // Directional Lights + 4 Point Lights
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir_point4,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags);

      // Direction Lights + Spot Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir_spot,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags);

      // Directional Lights + Point Light + Spot Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir_point_spot,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags);

      // Directional Lights + 2 Point Lights + Spot Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir_point2_spot,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags);
   }

   return variations;
}

}
