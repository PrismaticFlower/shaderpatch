#pragma once

#include "shader_patch.hpp"

#include <cstdint>
#include <new>

namespace sp::core {

enum class Command_type : std::uint8_t {
   present,
   destroy_game_rendertarget,
   destroy_game_texture,
   destroy_patch_texture,
   destroy_patch_material,
   destroy_patch_effects_config,
   destroy_ia_buffer,
   rename_ia_buffer_cpu,
   stretch_rendertarget,
   color_fill_rendertarget,
   clear_rendertarget,
   clear_depthstencil,
   set_index_buffer,
   set_vertex_buffer,
   set_input_layout,
   set_game_shader,
   set_rendertarget,
   set_depthstencil,
   set_rasterizer_state,
   set_depthstencil_state,
   set_blend_state,
   set_fog_state,
   set_texture,
   set_texture_rendertarget,
   set_projtex_mode,
   set_projtex_type,
   set_projtex_cube,
   set_patch_material,
   set_constants_scene,
   set_constants_draw,
   set_constants_fixedfunction,
   set_constants_skin,
   set_constants_draw_ps,
   set_informal_projection_matrix,
   draw,
   draw_indexed,

   destruction
};

namespace sync_commands {

void update_ia_buffer(const Buffer_handle buffer_handle, const UINT offset,
                      const UINT size, const std::byte* data) noexcept;

auto map_dynamic_texture(const Game_texture_handle game_texture_handle,
                         const UINT mip_level, const D3D11_MAP map_type) noexcept
   -> Mapped_texture;

void unmap_dynamic_texture(const Game_texture_handle game_texture_handle,
                           const UINT mip_level) noexcept;
}

namespace immediate_commands {

auto discard_ia_buffer_cpu(const Buffer_handle buffer_handle) noexcept -> std::size_t;

auto map_ia_buffer(const Buffer_handle Buffer, const D3D11_MAP map_type) noexcept
   -> std::byte*;

void unmap_ia_buffer(const Buffer_handle Buffer) noexcept;

}

namespace commands {
struct Present {
};

struct Destroy_game_rendertarget {
   const Game_rendertarget_id id;
};

struct Destroy_game_texture {
   const Game_texture_handle game_texture_handle;
};

struct Destroy_patch_texture {
   const Patch_texture_handle texture_handle;
};

struct Destroy_patch_material {
   const Material_handle material_handle;
};

struct Destroy_patch_effects_config {
   const Patch_effects_config_handle effects_config;
};

struct Destroy_ia_buffer {
   const Buffer_handle buffer_handle;
};

struct Rename_ia_buffer_cpu {
   const Buffer_handle buffer_handle;
   const std::size_t index
};

struct Stretch_rendertarget {
   const Game_rendertarget_id source;
   const RECT source_rect;
   const Game_rendertarget_id dest;
   const RECT dest_rect;
};

struct Color_fill_rendertarget {
   const Game_rendertarget_id rendertarget;
   const Clear_color color;
   const RECT* rect = nullptr;
};

struct Clear_rendertarget {
   const Clear_color color;
};

struct Clear_depthstencil {
   const float z;
   const UINT8 stencil;
   const bool clear_depth;
   const bool clear_stencil;
};

struct Set_index_buffer {
   const Buffer_handle buffer_handle;
   const UINT offset;
};

struct Set_vertex_buffer {
   const Buffer_handle buffer_handle;
   const UINT offset;
   const UINT stride;
};

struct Set_input_layout {
   const Game_input_layout& input_layout;
};

struct Set_game_shader {
   const std::uint32_t game_shader_index;
};

struct Set_rendertarget {
   const Game_rendertarget_id rendertarget;
};

struct Set_depthstencil {
   const Game_depthstencil depthstencil;
};

struct Set_rasterizer_state {
   ID3D11RasterizerState& rasterizer_state;
};

struct Set_depthstencil_state {
   ID3D11DepthStencilState& depthstencil_state;
   const UINT8 stencil_ref;
   const bool readonly;
};

struct Set_blend_state {
   ID3D11BlendState1& blend_state;
   const bool additive_blending;
};

struct Set_fog_state {
   const bool enabled;
   const glm::vec4 color;
};

struct Set_texture {
   const UINT slot;
   const Game_texture_handle game_texture_handle;
};

struct Set_texture_rendertarget {
   const UINT slot;
   const Game_rendertarget_id rendertarget;
};

struct Set_projtex_mode {
   const Projtex_mode mode;
};

struct Set_projtex_type {
   const Projtex_type type;
};

struct Set_projtex_cube {
   const Game_texture_handle game_texture_handle;
};

struct Set_patch_material {
   const Material_handle material_handle;
};

struct Set_constants_scene {
   const UINT offset;
   const std::span<const std::array<float, 4>> constants;
};

struct Set_constants_draw {
   const UINT offset;
   const std::span<const std::array<float, 4>> constants;
};

struct Set_constants_fixedfunction {
   const std::span<float, 8> constants;
};

struct Set_constants_skin {
   const UINT offset;
   const std::span<const std::array<float, 4>> constants;
};

struct Set_constants_draw_ps {
   const UINT offset;
   const std::span<const std::array<float, 4>> constants;
};

struct Set_informal_projection_matrix {
   const glm::mat4& matrix;
};

struct Draw {
   const D3D11_PRIMITIVE_TOPOLOGY topology;
   const UINT vertex_count;
   const UINT start_vertex;
};

struct Draw_indexed {
   const D3D11_PRIMITIVE_TOPOLOGY topology;
   const UINT index_count;
   const UINT start_index;
   const UINT start_vertex;
};

struct Destruction {
};

}

struct alignas(std::hardware_constructive_interference_size) Command {
   Command_type type;

   union {
      commands::Present present;
      commands::Destroy_game_rendertarget destroy_game_rendertarget;
      commands::Destroy_game_texture destroy_game_texture;
      commands::Destroy_patch_texture destroy_patch_texture;
      commands::Destroy_patch_material destroy_patch_material;
      commands::Destroy_patch_effects_config destroy_patch_effects_config;
      commands::Destroy_ia_buffer destroy_ia_buffer;
      commands::Rename_ia_buffer_cpu rename_ia_buffer_cpu;
      commands::Stretch_rendertarget stretch_rendertarget;
      commands::Color_fill_rendertarget color_fill_rendertarget;
      commands::Clear_rendertarget clear_rendertarget;
      commands::Clear_depthstencil clear_depthstencil;
      commands::Set_index_buffer set_index_buffer;
      commands::Set_vertex_buffer set_vertex_buffer;
      commands::Set_input_layout set_input_layout;
      commands::Set_game_shader set_game_shader;
      commands::Set_rendertarget set_rendertarget;
      commands::Set_depthstencil set_depthstencil;
      commands::Set_rasterizer_state set_rasterizer_state;
      commands::Set_depthstencil_state set_depthstencil_state;
      commands::Set_blend_state set_blend_state;
      commands::Set_fog_state set_fog_state;
      commands::Set_texture set_texture;
      commands::Set_texture_rendertarget set_texture_rendertarget;
      commands::Set_projtex_mode set_projtex_mode;
      commands::Set_projtex_type set_projtex_type;
      commands::Set_projtex_cube set_projtex_cube;
      commands::Set_patch_material set_patch_material;
      commands::Set_constants_scene set_constants_scene;
      commands::Set_constants_draw set_constants_draw;
      commands::Set_constants_fixedfunction set_constants_fixedfunction;
      commands::Set_constants_skin set_constants_skin;
      commands::Set_constants_draw_ps set_constants_draw_ps;
      commands::Set_informal_projection_matrix set_informal_projection_matrix;
      commands::Draw draw;
      commands::Draw_indexed draw_indexed;
      commands::Destruction destruction;
   };
};

static_assert(std::is_trivially_destructible_v<Command>);

class Command_queue {
public:
   void push(const Command& command) noexcept;

   auto pop() noexcept -> Command;

private:
};

}
