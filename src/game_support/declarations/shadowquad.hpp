
#include "../shader_declaration.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_declaration shadowquad_declaration{
   .rendertype = Rendertype::shadowquad,

   .pool = std::array{Shader_entry{.name = "shadowquad"sv,

                                   .skinned = false,
                                   .lighting = false,
                                   .vertex_color = false,
                                   .texture_coords = false,
                                   .base_input = Base_input::none,

                                   .srgb_state = {false, false, false, false}}},

   .states = std::tuple{
      Shader_state{.passes = single_pass({.shader_name = "shadowquad"sv})}}};

}
