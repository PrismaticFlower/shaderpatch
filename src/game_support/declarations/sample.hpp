
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration sample_declaration{
   .rendertype = Rendertype::sample,

   .pool = std::array{Shader_entry{.name = "sample"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {false, false, false, false}}},

   .states =
      std::tuple{Shader_state{.passes = single_pass({.shader_name = "sample"sv})}}};

}
