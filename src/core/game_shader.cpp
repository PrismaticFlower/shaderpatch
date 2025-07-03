
#include "game_shader.hpp"
#include "../game_support/munged_shader_declarations.hpp"
#include "../logger.hpp"

namespace sp::core {

namespace {

auto make_game_shader(shader::Rendertypes_database& database,
                      const game_support::Shader_metadata& metadata) noexcept -> Game_shader
{
   auto& state =
      database[to_string_view(metadata.rendertype)].state(metadata.shader_name);

   auto [vs, vs_bytecode, vs_inputlayout] = state.vertex(metadata.vertex_shader_flags);
   auto [vs_compressed, vs_bytecode_compressed, vs_inputlayout_compressed] =
      state.vertex(metadata.vertex_shader_flags | shader::Vertex_shader_flags::compressed);

   if (!vs && !vs_compressed) {
      log_and_terminate("Game shader has no vertex shader!"sv);
   }

   Com_ptr<ID3D11PixelShader> ps = state.pixel();
   Com_ptr<ID3D11PixelShader> ps_al = state.pixel_al();
   Com_ptr<ID3D11PixelShader> ps_oit = state.pixel_oit();
   Com_ptr<ID3D11PixelShader> ps_al_oit = state.pixel_al_oit();

   if (!ps_al) ps_al = ps;
   if (!ps_al_oit) ps_al_oit = ps_oit;

   return Game_shader{.vs = std::move(vs),
                      .vs_compressed = std::move(vs_compressed),
                      .ps = std::move(ps),
                      .ps_al = std::move(ps_al),
                      .ps_oit = std::move(ps_oit),
                      .ps_al_oit = std::move(ps_al_oit),
                      .light_active = metadata.light_active,
                      .light_active_point_count = metadata.light_active_point_count,
                      .light_active_spot = metadata.light_active_spot,
                      .rendertype = metadata.rendertype,
                      .srgb_state = metadata.srgb_state,
                      .shader_name = std::string{metadata.shader_name},
                      .vertex_shader_flags = metadata.vertex_shader_flags,
                      .input_layouts = {std::move(vs_inputlayout), std::move(vs_bytecode)},
                      .input_layouts_compressed = {std::move(vs_inputlayout_compressed),
                                                   std::move(vs_bytecode_compressed)}};
}

}

Game_shader_store::Game_shader_store(shader::Rendertypes_database& database) noexcept
{
   const auto& game_shader_pool = game_support::munged_shader_declarations().shader_pool;

   _shaders.reserve(game_shader_pool.size());

   for (const auto& entry : game_shader_pool) {
      _shaders.emplace_back(make_game_shader(database, entry));
   }
}

}
