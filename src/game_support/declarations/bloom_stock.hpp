
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration bloom_stock_declaration{
   .rendertype = Rendertype::hdr,

   .pool = std::array{Shader_entry{.name = "glow threshold"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = true,
                                   .base_input = Base_input::none,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "luminance"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = true,
                                   .base_input = Base_input::none,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "bloomfilter"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = true,
                                   .base_input = Base_input::none,

                                   .srgb_state = {false, false, false, false}},

                      Shader_entry{.name = "screenspace"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = true,
                                   .base_input = Base_input::none,

                                   .srgb_state = {false, false, false, false}}},

   .states = std::tuple{
      Shader_state{.passes = single_pass({.shader_name = "glow threshold"sv})},

      Shader_state{.passes = single_pass({.shader_name = "luminance"sv})},

      Shader_state{.passes = single_pass({.shader_name = "bloomfilter"sv})},

      Shader_state{.passes = single_pass({.shader_name = "screenspace"sv})}}};

}
