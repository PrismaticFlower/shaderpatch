#pragma once

#include "shader_metadata.hpp"

using namespace std::literals;

namespace sp::game_support {

inline constexpr Shader_metadata
   fixedfunc_color_fill_declaration{.rendertype = Rendertype::fixedfunc_color_fill,
                                    .shader_name = "color fill"sv,
                                    .srgb_state = {false, false, false, false}};

inline constexpr Shader_metadata fixedfunc_damage_overlay_declaration{
   .rendertype = Rendertype::fixedfunc_damage_overlay,
   .shader_name = "damage overlay"sv,
   .srgb_state = {false, false, false, false}};

inline constexpr Shader_metadata fixedfunc_plain_texture_declaration{
   .rendertype = Rendertype::fixedfunc_plain_texture,
   .shader_name = "plain texture"sv,

   .srgb_state = {false, false, false, false}};

inline constexpr Shader_metadata
   fixedfunc_scene_blur_declaration{.rendertype = Rendertype::fixedfunc_scene_blur,
                                    .shader_name = "scene blur"sv,
                                    .srgb_state = {false, false, false, false}};

inline constexpr Shader_metadata
   fixedfunc_zoom_blur_declaration{.rendertype = Rendertype::fixedfunc_zoom_blur,
                                   .shader_name = "zoom blur"sv,
                                   .srgb_state = {false, false, false, false}};

inline constexpr std::array fixedfunc_shader_declarations{
   fixedfunc_color_fill_declaration, fixedfunc_damage_overlay_declaration,
   fixedfunc_plain_texture_declaration, fixedfunc_scene_blur_declaration,
   fixedfunc_zoom_blur_declaration};

}
