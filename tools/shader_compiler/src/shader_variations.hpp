#pragma once

#include "compiler_helpers.hpp"
#include "preprocessor_defines.hpp"
#include "shader_entrypoint.hpp"
#include "shader_flags.hpp"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace sp::shader {

struct Vertex_variation {
   Vertex_shader_flags flags = Vertex_shader_flags::none;
   std::uint32_t static_flags = 0;
   Preprocessor_defines defines;
};

struct Pixel_variation {
   Pixel_shader_flags flags = Pixel_shader_flags::none;
   std::uint32_t static_flags = 0;
   Preprocessor_defines defines;
};

inline auto get_vertex_base_variation(const Vertex_state& state) noexcept -> Vertex_variation
{
   using namespace std::literals;

   Vertex_variation variation;

   if (state.generic_input.position) {
      variation.flags |= Vertex_shader_flags::position;
      variation.defines.add_define("__VERTEX_INPUT_POSITION__"s, "1"s);
   }

   if (state.generic_input.normal) {
      variation.flags |= Vertex_shader_flags::normal;
      variation.defines.add_define("__VERTEX_INPUT_NORMAL__"s, "1"s);
   }

   if (state.generic_input.tangents) {
      variation.flags |= Vertex_shader_flags::tangents;
      variation.defines.add_define("__VERTEX_INPUT_TANGENTS__"s, "1"s);
   }

   if (state.generic_input.texture_coords) {
      variation.flags |= Vertex_shader_flags::texcoords;
      variation.defines.add_define("__VERTEX_INPUT_TEXCOORDS__"s, "1"s);
   }

   return variation;
}

inline auto get_vertex_shader_variations(const Vertex_state& state) noexcept
   -> std::vector<Vertex_variation>
{
   using namespace std::literals;

   std::vector<Vertex_variation> variations;
   variations.reserve(16);

   const auto base_variation = get_vertex_base_variation(state);

   const auto add_variation = [&](const Vertex_shader_flags flags,
                                  std::initializer_list<Shader_define> defines) {
      auto& vari = variations.emplace_back(base_variation);
      vari.flags |= flags;
      vari.defines.add_defines(defines);
   };

   const Shader_define compressed_def = {"__VERTEX_INPUT_IS_COMPRESSED__"s, "1"s};

   const Shader_define unskinned_def = {"__VERTEX_TRANSFORM_UNSKINNED__", "1"};
   const Shader_define skinned_def = {"__VERTEX_TRANSFORM_HARD_SKINNED__", "1"};

   const Shader_define blend_indices_def = {"__VERTEX_INPUT_BLEND_INDICES__"s, "1"s};
   const Shader_define color_def = {"__VERTEX_INPUT_COLOR__", "1"};

   add_variation(Vertex_shader_flags::none, {unskinned_def});

   if (state.generic_input.skinned) {
      add_variation(Vertex_shader_flags::hard_skinned,
                    {skinned_def, blend_indices_def});
   }

   if (state.generic_input.color) {
      add_variation(Vertex_shader_flags::color, {unskinned_def, color_def});
   }

   if (state.generic_input.skinned && state.generic_input.color) {
      add_variation(Vertex_shader_flags::hard_skinned | Vertex_shader_flags::color,
                    {skinned_def, blend_indices_def, color_def});
   }

   if (state.generic_input.dynamic_compression) {
      add_variation(Vertex_shader_flags::compressed, {unskinned_def, compressed_def});

      if (state.generic_input.skinned) {
         add_variation(Vertex_shader_flags::hard_skinned | Vertex_shader_flags::compressed,
                       {skinned_def, blend_indices_def, compressed_def});
      }

      if (state.generic_input.color) {
         add_variation(Vertex_shader_flags::color | Vertex_shader_flags::compressed,
                       {unskinned_def, color_def, compressed_def});
      }

      if (state.generic_input.skinned && state.generic_input.color) {
         add_variation(Vertex_shader_flags::hard_skinned | Vertex_shader_flags::color |
                          Vertex_shader_flags::compressed,
                       {skinned_def, blend_indices_def, color_def, compressed_def});
      }
   }

   return variations;
}

inline auto get_pixel_shader_variations(const Pixel_state& state) noexcept
   -> std::vector<Pixel_variation>
{
   using namespace std::literals;

   std::vector<Pixel_variation> variations;
   variations.reserve(8);

   const auto add_variation = [&](const Pixel_shader_flags flags,
                                  std::initializer_list<Shader_define> defines) {
      auto& vari = variations.emplace_back();
      vari.flags = flags;
      vari.defines.add_defines(defines);
   };

   const Shader_define light_dir_def = {"__PS_LIGHT_ACTIVE_DIRECTIONAL__", "1"};
   const Shader_define light_point_def = {"__PS_LIGHT_ACTIVE_POINT_0__", "1"};
   const Shader_define light_point_1_def = {"__PS_LIGHT_ACTIVE_POINT_1__", "1"};
   const Shader_define light_point_23_def = {"__PS_LIGHT_ACTIVE_POINT_23__", "1"};
   const Shader_define light_spot_def = {"__PS_LIGHT_ACTIVE_SPOT__", "1"};

   add_variation(Pixel_shader_flags::none, {});

   if (state.lighting) {
      // Directional Light
      add_variation(Pixel_shader_flags::light_directional, {light_dir_def});

      // Directional Lights + Point Light
      add_variation(Pixel_shader_flags::light_directional | Pixel_shader_flags::light_point_1,
                    {light_dir_def, light_point_def});

      // Directional Lights + 2 Point Lights
      add_variation(Pixel_shader_flags::light_directional | Pixel_shader_flags::light_point_2,
                    {light_dir_def, light_point_def, light_point_1_def});

      // Directional Lights + 4 Point Lights
      add_variation(Pixel_shader_flags::light_directional | Pixel_shader_flags::light_point_4,
                    {light_dir_def, light_point_def, light_point_1_def,
                     light_point_23_def});

      // Direction Lights + Spot Light
      add_variation(Pixel_shader_flags::light_directional | Pixel_shader_flags::light_spot,
                    {light_dir_def, light_spot_def});

      // Directional Lights + Point Light + Spot Light
      add_variation(Pixel_shader_flags::light_directional |
                       Pixel_shader_flags::light_point_1 | Pixel_shader_flags::light_spot,
                    {light_dir_def, light_point_def, light_spot_def});

      // Directional Lights + 2 Point Lights + Spot Light
      add_variation(Pixel_shader_flags::light_directional |
                       Pixel_shader_flags::light_point_2 | Pixel_shader_flags::light_spot,
                    {light_dir_def, light_point_def, light_spot_def, light_point_1_def});
   }

   return variations;
}

template<typename Left_variations, typename Right_variations>
inline auto combine_variations(const Left_variations& left,
                               const Right_variations& right) noexcept
{
   using Variation = typename Left_variations::value_type;

   std::vector<Variation> combined;
   combined.reserve(left.size() * right.size());

   for (const auto& base : left) {
      for (const auto& with : right) {
         Variation variation;

         variation.defines = combine_defines(base.defines, with.defines);
         variation.flags = base.flags | with.flags;
         variation.static_flags = base.static_flags | with.static_flags;

         combined.emplace_back(variation);
      }
   }

   return combined;
}

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
   -> std::vector<std::tuple<Shader_flags, Vertex_shader_flags, Pixel_shader_flags>>
{
   using namespace std::literals;

   std::vector<std::tuple<Shader_flags, Vertex_shader_flags, Pixel_shader_flags>> variations;
   variations.reserve(36);

   const auto base_vertex_flags = get_base_declaration_vertex_shader_flags(flags);
   const bool vertex_color = pass_flag_set(flags, Pass_flags::vertex_color);
   const bool lighting = pass_flag_set(flags, Pass_flags::lighting);

   // Unskinned
   variations.emplace_back(Shader_flags::none, base_vertex_flags,
                           Pixel_shader_flags{});

   // Unskinned Vertex colored
   if (vertex_color) {
      variations.emplace_back(Shader_flags::vertexcolor,
                              Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags{});
   }

   // Skinned Defintions
   if (skinned) {
      // Skinned
      variations.emplace_back(Shader_flags::skinned,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags,
                              Pixel_shader_flags{});
   }

   // Skinned Vertex colored
   if (skinned && vertex_color) {
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags{});
   }

   // Unskinned Lighting Definitions
   if (lighting) {
      // Directional Light
      variations.emplace_back(Shader_flags::light_dir, base_vertex_flags,
                              Pixel_shader_flags::light_directional);

      // Directional Lights + Point Light
      variations.emplace_back(Shader_flags::light_dir_point, base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_1);

      // Directional Lights + 2 Point Lights
      variations.emplace_back(Shader_flags::light_dir_point2, base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_2);

      // Directional Lights + 4 Point Lights
      variations.emplace_back(Shader_flags::light_dir_point4, base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_4);

      // Direction Lights + Spot Light
      variations.emplace_back(Shader_flags::light_dir_spot, base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_spot);

      // Directional Lights + Point Light + Spot Light
      variations.emplace_back(Shader_flags::light_dir_point_spot, base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_1 |
                                 Pixel_shader_flags::light_spot);

      // Directional Lights + 2 Point Lights + Spot Light
      variations.emplace_back(Shader_flags::light_dir_point2_spot, base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_2 |
                                 Pixel_shader_flags::light_spot);
   }

   // Unskinned Lighting Vertexcolor Definitions
   if (lighting && vertex_color) {
      // Directional Light
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir,
                              Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional);

      // Directional Lights + Point Light
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir_point,
                              Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_1);

      // Directional Lights + 2 Point Lights
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir_point2,
                              Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_2);

      // Directional Lights + 4 Point Lights
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir_point4,
                              Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_4);

      // Direction Lights + Spot Light
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir_spot,
                              Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_spot);

      // Directional Lights + Point Light + Spot Light
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir_point_spot,
                              Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_1 |
                                 Pixel_shader_flags::light_spot);

      // Directional Lights + 2 Point Lights + Spot Light
      variations.emplace_back(Shader_flags::vertexcolor | Shader_flags::light_dir_point2_spot,
                              Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_2 |
                                 Pixel_shader_flags::light_spot);
   }

   // Skinned Lighting Definitions
   if (lighting && skinned) {
      // Directional Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags,
                              Pixel_shader_flags::light_directional);

      // Directional Lights + Point Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir_point,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_1);

      // Directional Lights + 2 Point Lights
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir_point2,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_2);

      // Directional Lights + 4 Point Lights
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir_point4,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_4);

      // Direction Lights + Spot Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir_spot,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_spot);

      // Directional Lights + Point Light + Spot Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir_point_spot,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_1 |
                                 Pixel_shader_flags::light_spot);

      // Directional Lights + 2 Point Lights + Spot Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::light_dir_point2_spot,
                              Vertex_shader_flags::hard_skinned | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_2 |
                                 Pixel_shader_flags::light_spot);
   }

   // Skinned Lighting Vertexcolor Definitions
   if (lighting && skinned && vertex_color) {
      // Directional Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional);

      // Directional Lights + Point Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir_point,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_1);

      // Directional Lights + 2 Point Lights
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir_point2,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_2);

      // Directional Lights + 4 Point Lights
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir_point4,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_4);

      // Direction Lights + Spot Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir_spot,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_spot);

      // Directional Lights + Point Light + Spot Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir_point_spot,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_1 |
                                 Pixel_shader_flags::light_spot);

      // Directional Lights + 2 Point Lights + Spot Light
      variations.emplace_back(Shader_flags::skinned | Shader_flags::vertexcolor |
                                 Shader_flags::light_dir_point2_spot,
                              Vertex_shader_flags::hard_skinned |
                                 Vertex_shader_flags::color | base_vertex_flags,
                              Pixel_shader_flags::light_directional |
                                 Pixel_shader_flags::light_point_2 |
                                 Pixel_shader_flags::light_spot);
   }

   return variations;
}

}
