#pragma once

#include "compiler_helpers.hpp"
#include "preprocessor_defines.hpp"
#include "shader_flags.hpp"

#include <initializer_list>
#include <string>
#include <unordered_set>
#include <vector>

namespace sp {

struct Shader_variation {
   Shader_flags flags;
   Preprocessor_defines defines;
   Preprocessor_defines ps_defines;
};

inline auto get_common_base_defines(Pass_flags flags) noexcept
   -> std::vector<Shader_define>
{
   using namespace std::literals;

   if (pass_flag_set(flags, Pass_flags::notransform)) return {};

   std::vector<Shader_define> base_defines;

   if (pass_flag_set(flags, Pass_flags::position)) {
      base_defines.emplace_back("__VERTEX_INPUT_POSITION__"s, "1"s);
   }

   if (pass_flag_set(flags, Pass_flags::normal)) {
      base_defines.emplace_back("__VERTEX_INPUT_NORMAL__"s, "1"s);
   }

   if (pass_flag_set(flags, Pass_flags::tangents)) {
      base_defines.emplace_back("__VERTEX_INPUT_TANGENTS__"s, "1"s);
   }

   if (pass_flag_set(flags, Pass_flags::texcoords)) {
      base_defines.emplace_back("__VERTEX_INPUT_TEXCOORDS__"s, "1"s);
   }

   return base_defines;
}

inline auto get_shader_variations(Pass_flags flags, bool skinned)
   -> std::vector<Shader_variation>
{
   using namespace std::literals;

   std::vector<Shader_variation> variations;
   variations.reserve(36);

   const auto base_defines = get_common_base_defines(flags);

   const auto add_variation = [&](const Shader_flags flags,
                                  std::initializer_list<Shader_define> vs_defines,
                                  std::initializer_list<Shader_define> ps_defines) {
      auto& vari = variations.emplace_back();
      vari.flags = flags;
      vari.defines.add_defines(base_defines);
      vari.defines.add_defines(vs_defines);
      vari.ps_defines.add_defines(ps_defines);
   };

   const Shader_define unskinned_def = {"__VERTEX_TRANSFORM_UNSKINNED__", "1"};
   const Shader_define skinned_def = {"__VERTEX_TRANSFORM_HARD_SKINNED__", "1"};

   const Shader_define blend_indices_def = {"__VERTEX_INPUT_BLEND_INDICES__"s, "1"s};
   const Shader_define color_def = {"__VERTEX_INPUT_COLOR__", "1"};

   const Shader_define light_dir_def = {"__PS_LIGHT_ACTIVE_DIRECTIONAL__", "1"};
   const Shader_define light_point_def = {"__PS_LIGHT_ACTIVE_POINT_0__", "1"};
   const Shader_define light_point_1_def = {"__PS_LIGHT_ACTIVE_POINT_1__", "1"};
   const Shader_define light_point_23_def = {"__PS_LIGHT_ACTIVE_POINT_23__", "1"};
   const Shader_define light_spot_def = {"__PS_LIGHT_ACTIVE_SPOT__", "1"};

   const bool vertex_color = pass_flag_set(flags, Pass_flags::vertex_color);
   const bool lighting = pass_flag_set(flags, Pass_flags::lighting);

   // Unskinned
   add_variation(Shader_flags::none, {unskinned_def}, {});

   // Unskinned Vertex colored
   if (vertex_color) {
      add_variation(Shader_flags::vertexcolor, {unskinned_def, color_def}, {});
   }

   // Skinned Defintions
   if (skinned) {
      // Skinned
      add_variation(Shader_flags::skinned, {skinned_def, blend_indices_def}, {});
   }

   // Skinned Vertex colored
   if (skinned && vertex_color) {
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor,
                    {skinned_def, blend_indices_def, color_def}, {});
   }

   // Unskinned Lighting Definitions
   if (lighting) {
      // Directional Light
      add_variation(Shader_flags::light_dir, {unskinned_def}, {light_dir_def});

      // Directional Lights + Point Light
      add_variation(Shader_flags::light_dir_point, {unskinned_def},
                    {light_dir_def, light_point_def});

      // Directional Lights + 2 Point Lights
      add_variation(Shader_flags::light_dir_point2, {unskinned_def},
                    {light_dir_def, light_point_def, light_point_1_def});

      // Directional Lights + 4 Point Lights
      add_variation(Shader_flags::light_dir_point4, {unskinned_def},
                    {light_dir_def, light_point_def, light_point_1_def,
                     light_point_23_def});

      // Direction Lights + Spot Light
      add_variation(Shader_flags::light_dir_spot, {unskinned_def},
                    {light_dir_def, light_spot_def});

      // Directional Lights + Point Light + Spot Light
      add_variation(Shader_flags::light_dir_point_spot, {unskinned_def},
                    {light_dir_def, light_point_def, light_spot_def});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Shader_flags::light_dir_point2_spot, {unskinned_def},
                    {light_dir_def, light_point_def, light_spot_def, light_point_1_def});
   }

   // Unskinned Lighting Vertexcolor Definitions
   if (lighting && vertex_color) {
      // Directional Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir,
                    {unskinned_def, color_def}, {light_dir_def});

      // Directional Lights + Point Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point,
                    {unskinned_def, color_def}, {light_dir_def, light_point_def});

      // Directional Lights + 2 Point Lights
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point2,
                    {unskinned_def, color_def},
                    {light_dir_def, light_point_def, light_point_1_def});

      // Directional Lights + 4 Point Lights
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point4,
                    {unskinned_def, color_def},
                    {light_dir_def, light_point_def, light_point_1_def,
                     light_point_23_def});

      // Direction Lights + Spot Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_spot,
                    {unskinned_def, color_def}, {light_dir_def, light_spot_def});

      // Directional Lights + Point Light + Spot Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point_spot,
                    {unskinned_def, color_def},
                    {light_dir_def, light_point_def, light_spot_def});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point2_spot,
                    {unskinned_def, color_def},
                    {light_dir_def, light_point_def, light_spot_def, light_point_1_def});
   }

   // Skinned Lighting Definitions
   if (lighting && skinned) {
      // Directional Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir,
                    {skinned_def, blend_indices_def}, {light_dir_def});

      // Directional Lights + Point Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point,
                    {skinned_def, blend_indices_def},
                    {light_dir_def, light_point_def});

      // Directional Lights + 2 Point Lights
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point2,
                    {skinned_def, blend_indices_def},
                    {light_dir_def, light_point_def, light_point_1_def});

      // Directional Lights + 4 Point Lights
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point4,
                    {skinned_def, blend_indices_def},
                    {light_dir_def, light_point_def, light_point_1_def,
                     light_point_23_def});

      // Direction Lights + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_spot,
                    {skinned_def, blend_indices_def},
                    {light_dir_def, light_spot_def});

      // Directional Lights + Point Light + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point_spot,
                    {skinned_def, blend_indices_def},
                    {light_dir_def, light_point_def, light_spot_def});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point2_spot,
                    {skinned_def, blend_indices_def},
                    {light_dir_def, light_point_def, light_spot_def, light_point_1_def});
   }

   // Skinned Lighting Vertexcolor Definitions
   if (lighting && skinned && vertex_color) {
      // Directional Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir,
                    {skinned_def, blend_indices_def, color_def}, {light_dir_def});

      // Directional Lights + Point Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point,
                    {skinned_def, blend_indices_def, color_def},
                    {light_dir_def, light_point_def});

      // Directional Lights + 2 Point Lights
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point2,
                    {skinned_def, blend_indices_def, color_def},
                    {light_dir_def, light_point_def, light_point_1_def});

      // Directional Lights + 4 Point Lights
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point4,
                    {skinned_def, blend_indices_def, color_def},
                    {light_dir_def, light_point_def, light_point_1_def,
                     light_point_23_def});

      // Direction Lights + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_spot,
                    {skinned_def, blend_indices_def, color_def},
                    {light_dir_def, light_spot_def});

      // Directional Lights + Point Light + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point_spot,
                    {skinned_def, blend_indices_def, color_def},
                    {light_dir_def, light_point_def, light_spot_def});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point2_spot,
                    {skinned_def, blend_indices_def, color_def},
                    {light_dir_def, light_point_def, light_spot_def, light_point_1_def});
   }

   return variations;
}

}
