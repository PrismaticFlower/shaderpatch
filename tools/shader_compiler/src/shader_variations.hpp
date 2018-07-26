#pragma once

#include "compiler_helpers.hpp"
#include "preprocessor_defines.hpp"
#include "shader_flags.hpp"

#include <initializer_list>
#include <unordered_set>
#include <vector>

#include <boost/algorithm/cxx11/copy_if.hpp>

#include <d3dcompiler.h>

namespace sp {

struct Shader_variation {
   Shader_flags flags;
   Preprocessor_defines defines;
   Preprocessor_defines ps_defines;
};

inline auto get_shader_variations(const bool skinned, const bool lighting,
                                  const bool vertex_color)
   -> std::vector<Shader_variation>
{
   using namespace std::literals;

   std::vector<Shader_variation> variations;
   variations.reserve(36);

   const static std::unordered_set ps_undefines{"TRANSFORM_UNSKINNED"s,
                                                "TRANSFORM_SOFT_SKINNED"s};

   const auto add_variation = [&](const Shader_flags flags,
                                  std::initializer_list<Shader_define> defines) {
      auto& vari = variations.emplace_back();
      vari.flags = flags;
      vari.defines.add_defines(defines);
      vari.ps_defines.add_defines(defines);
      vari.ps_defines.add_undefines(ps_undefines);
   };

   const Shader_define unskinned_def = {"TRANSFORM_UNSKINNED", ""};
   const Shader_define skinned_def = {"TRANSFORM_SOFT_SKINNED", ""};
   const Shader_define color_def = {"USE_VERTEX_COLOR", ""};

   const Shader_define light_dir_def = {"LIGHTING_DIRECTIONAL", ""};
   const Shader_define light_point_def = {"LIGHTING_POINT_0", ""};
   const Shader_define light_point_1_def = {"LIGHTING_POINT_1", ""};
   const Shader_define light_point_23_def = {"LIGHTING_POINT_23", ""};
   const Shader_define light_spot_def = {"LIGHTING_SPOT_0", ""};

   // Unskinned
   add_variation(Shader_flags::none, {unskinned_def});

   // Unskinned Vertex colored
   if (vertex_color) {
      add_variation(Shader_flags::vertexcolor, {unskinned_def, color_def});
   }

   // Skinned Defintions
   if (skinned) {
      // Skinned
      add_variation(Shader_flags::skinned, {skinned_def});
   }

   // Skinned Vertex colored
   if (skinned && vertex_color) {
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor,
                    {skinned_def, color_def});
   }

   // Unskinned Lighting Definitions
   if (lighting) {
      // Directional Light
      add_variation(Shader_flags::light_dir, {unskinned_def, light_dir_def});

      // Directional Lights + Point Light
      add_variation(Shader_flags::light_dir_point,
                    {unskinned_def, light_dir_def, light_point_def});

      // Directional Lights + 2 Point Lights
      add_variation(Shader_flags::light_dir_point2,
                    {unskinned_def, light_dir_def, light_point_def, light_point_1_def});

      // Directional Lights + 4 Point Lights
      add_variation(Shader_flags::light_dir_point4,
                    {unskinned_def, light_dir_def, light_point_def,
                     light_point_1_def, light_point_23_def});

      // Direction Lights + Spot Light
      add_variation(Shader_flags::light_dir_spot,
                    {unskinned_def, light_dir_def, light_spot_def});

      // Directional Lights + Point Light + Spot Light
      add_variation(Shader_flags::light_dir_point_spot,
                    {unskinned_def, light_dir_def, light_point_def, light_spot_def});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Shader_flags::light_dir_point2_spot,
                    {unskinned_def, light_dir_def, light_point_def,
                     light_spot_def, light_point_1_def});
   }

   // Unskinned Lighting Vertexcolor Definitions
   if (lighting && vertex_color) {
      // Directional Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir,
                    {unskinned_def, color_def, light_dir_def});

      // Directional Lights + Point Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point,
                    {unskinned_def, color_def, light_dir_def, light_point_def});

      // Directional Lights + 2 Point Lights
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point2,
                    {unskinned_def, color_def, light_dir_def, light_point_def,
                     light_point_1_def});

      // Directional Lights + 4 Point Lights
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point4,
                    {unskinned_def, color_def, light_dir_def, light_point_def,
                     light_point_1_def, light_point_23_def});

      // Direction Lights + Spot Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_spot,
                    {unskinned_def, color_def, light_dir_def, light_spot_def});

      // Directional Lights + Point Light + Spot Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point_spot,
                    {unskinned_def, color_def, light_dir_def, light_point_def,
                     light_spot_def});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point2_spot,
                    {unskinned_def, color_def, light_dir_def, light_point_def,
                     light_spot_def, light_point_1_def});
   }

   // Skinned Lighting Definitions
   if (lighting && skinned) {
      // Directional Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir,
                    {skinned_def, light_dir_def});

      // Directional Lights + Point Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point,
                    {skinned_def, light_dir_def, light_point_def});

      // Directional Lights + 2 Point Lights
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point2,
                    {skinned_def, light_dir_def, light_point_def, light_point_1_def});

      // Directional Lights + 4 Point Lights
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point4,
                    {skinned_def, light_dir_def, light_point_def,
                     light_point_1_def, light_point_23_def});

      // Direction Lights + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_spot,
                    {skinned_def, light_dir_def, light_spot_def});

      // Directional Lights + Point Light + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point_spot,
                    {skinned_def, light_dir_def, light_point_def, light_spot_def});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point2_spot,
                    {skinned_def, light_dir_def, light_point_def,
                     light_spot_def, light_point_1_def});
   }

   // Skinned Lighting Vertexcolor Definitions
   if (lighting && skinned && vertex_color) {
      // Directional Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir,
                    {skinned_def, color_def, light_dir_def});

      // Directional Lights + Point Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point,
                    {skinned_def, color_def, light_dir_def, light_point_def});

      // Directional Lights + 2 Point Lights
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point2,
                    {skinned_def, color_def, light_dir_def, light_point_def,
                     light_point_1_def});

      // Directional Lights + 4 Point Lights
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point4,
                    {skinned_def, color_def, light_dir_def, light_point_def,
                     light_point_1_def, light_point_23_def});

      // Direction Lights + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_spot,
                    {skinned_def, color_def, light_dir_def, light_spot_def});

      // Directional Lights + Point Light + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point_spot,
                    {skinned_def, color_def, light_dir_def, light_point_def,
                     light_spot_def});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point2_spot,
                    {skinned_def, color_def, light_dir_def, light_point_def,
                     light_spot_def, light_point_1_def});
   }

   return variations;
}
}
