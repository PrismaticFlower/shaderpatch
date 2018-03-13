#pragma once

#include "shader_flags.hpp"

#include <initializer_list>
#include <vector>

#include <d3dcompiler.h>

namespace sp {

struct Shader_variation {
   Shader_flags flags;
   std::vector<D3D_SHADER_MACRO> definitions;
};

inline auto get_shader_variations(const bool skinned, const bool lighting,
                                  const bool vertex_color)
   -> std::vector<Shader_variation>
{
   std::vector<Shader_variation> variations;
   variations.reserve(36);

   const auto add_variation = [&](const Shader_flags flags,
                                  std::initializer_list<D3D_SHADER_MACRO> defines) {
      variations.emplace_back();
      variations.back().flags = flags;
      variations.back().definitions.assign(std::cbegin(defines), std::cend(defines));
   };

   const D3D_SHADER_MACRO nulldef = {nullptr, nullptr};
   const D3D_SHADER_MACRO unskinned_def = {"TRANSFORM_UNSKINNED", ""};
   const D3D_SHADER_MACRO skinned_def = {"TRANSFORM_SOFT_SKINNED", ""};
   const D3D_SHADER_MACRO color_def = {"USE_VERTEX_COLOR", ""};

   const D3D_SHADER_MACRO light_dir_def = {"LIGHTING_DIRECTIONAL", ""};
   const D3D_SHADER_MACRO light_point_def = {"LIGHTING_POINT_0", ""};
   const D3D_SHADER_MACRO light_point_1_def = {"LIGHTING_POINT_1", ""};
   const D3D_SHADER_MACRO light_point_23_def = {"LIGHTING_POINT_23", ""};
   const D3D_SHADER_MACRO light_spot_def = {"LIGHTING_SPOT_0", ""};

   // Unskinned
   add_variation(Shader_flags::none, {unskinned_def, nulldef});

   // Unskinned Vertex colored
   if (vertex_color) {
      add_variation(Shader_flags::vertexcolor, {unskinned_def, color_def, nulldef});
   }

   // Skinned Defintions
   if (skinned) {
      // Skinned
      add_variation(Shader_flags::skinned, {skinned_def, nulldef});
   }

   // Skinned Vertex colored
   if (skinned && vertex_color) {
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor,
                    {skinned_def, color_def, nulldef});
   }

   // Unskinned Lighting Definitions
   if (lighting) {
      // Directional Light
      add_variation(Shader_flags::light_dir, {unskinned_def, light_dir_def, nulldef});

      // Directional Lights + Point Light
      add_variation(Shader_flags::light_dir_point,
                    {unskinned_def, light_dir_def, light_point_def, nulldef});

      // Directional Lights + 2 Point Lights
      add_variation(Shader_flags::light_dir_point2,
                    {unskinned_def, light_dir_def, light_point_def,
                     light_point_1_def, nulldef});

      // Directional Lights + 4 Point Lights
      add_variation(Shader_flags::light_dir_point4,
                    {unskinned_def, light_dir_def, light_point_def,
                     light_point_1_def, light_point_23_def, nulldef});

      // Direction Lights + Spot Light
      add_variation(Shader_flags::light_dir_spot,
                    {unskinned_def, light_dir_def, light_spot_def, nulldef});

      // Directional Lights + Point Light + Spot Light
      add_variation(Shader_flags::light_dir_point_spot,
                    {unskinned_def, light_dir_def, light_point_def,
                     light_spot_def, nulldef});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Shader_flags::light_dir_point2_spot,
                    {unskinned_def, light_dir_def, light_point_def,
                     light_spot_def, light_point_1_def, nulldef});
   }

   // Unskinned Lighting Vertexcolor Definitions
   if (lighting && vertex_color) {
      // Directional Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir,
                    {unskinned_def, color_def, light_dir_def, nulldef});

      // Directional Lights + Point Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point,
                    {unskinned_def, color_def, light_dir_def, light_point_def, nulldef});

      // Directional Lights + 2 Point Lights
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point2,
                    {unskinned_def, color_def, light_dir_def, light_point_def,
                     light_point_1_def, nulldef});

      // Directional Lights + 4 Point Lights
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point4,
                    {unskinned_def, color_def, light_dir_def, light_point_def,
                     light_point_1_def, light_point_23_def, nulldef});

      // Direction Lights + Spot Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_spot,
                    {unskinned_def, color_def, light_dir_def, light_spot_def, nulldef});

      // Directional Lights + Point Light + Spot Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point_spot,
                    {unskinned_def, color_def, light_dir_def, light_point_def,
                     light_spot_def, nulldef});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Shader_flags::vertexcolor | Shader_flags::light_dir_point2_spot,
                    {unskinned_def, color_def, light_dir_def, light_point_def,
                     light_spot_def, light_point_1_def, nulldef});
   }

   // Skinned Lighting Definitions
   if (lighting && skinned) {
      // Directional Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir,
                    {skinned_def, light_dir_def, nulldef});

      // Directional Lights + Point Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point,
                    {skinned_def, light_dir_def, light_point_def, nulldef});

      // Directional Lights + 2 Point Lights
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point2,
                    {skinned_def, light_dir_def, light_point_def,
                     light_point_1_def, nulldef});

      // Directional Lights + 4 Point Lights
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point4,
                    {skinned_def, light_dir_def, light_point_def,
                     light_point_1_def, light_point_23_def, nulldef});

      // Direction Lights + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_spot,
                    {skinned_def, light_dir_def, light_spot_def, nulldef});

      // Directional Lights + Point Light + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point_spot,
                    {skinned_def, light_dir_def, light_point_def,
                     light_spot_def, nulldef});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::light_dir_point2_spot,
                    {skinned_def, light_dir_def, light_point_def,
                     light_spot_def, light_point_1_def, nulldef});
   }

   // Skinned Lighting Vertexcolor Definitions
   if (lighting && skinned && vertex_color) {
      // Directional Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir,
                    {skinned_def, color_def, light_dir_def, nulldef});

      // Directional Lights + Point Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point,
                    {skinned_def, color_def, light_dir_def, light_point_def, nulldef});

      // Directional Lights + 2 Point Lights
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point2,
                    {skinned_def, color_def, light_dir_def, light_point_def,
                     light_point_1_def, nulldef});

      // Directional Lights + 4 Point Lights
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point4,
                    {skinned_def, color_def, light_dir_def, light_point_def,
                     light_point_1_def, light_point_23_def, nulldef});

      // Direction Lights + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_spot,
                    {skinned_def, color_def, light_dir_def, light_spot_def, nulldef});

      // Directional Lights + Point Light + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point_spot,
                    {skinned_def, color_def, light_dir_def, light_point_def,
                     light_spot_def, nulldef});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Shader_flags::skinned | Shader_flags::vertexcolor |
                       Shader_flags::light_dir_point2_spot,
                    {skinned_def, color_def, light_dir_def, light_point_def,
                     light_spot_def, light_point_1_def, nulldef});
   }

   return variations;
}
}
