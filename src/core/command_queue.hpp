#pragma once

#include "common.hpp"
#include "constant_buffers.hpp"
#include "game_input_layout.hpp"
#include "handles.hpp"
#include "single_producer_single_consumer_queue.hpp"

#include <cstdint>
#include <new>
#include <optional>
#include <span>

namespace sp::core {

enum class Command : std::uint8_t {
   reset,
   set_text_dpi,
   present,
   destroy_game_rendertarget,
   destroy_game_texture,
   destroy_patch_texture,
   destroy_patch_material,
   load_patch_effects_config,
   destroy_patch_effects_config,
   load_colorgrading_regions,
   destroy_ia_buffer,
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
   rename_ia_buffer_dynamic_cpu,

   null
};

namespace commands {
struct Reset {
   const UINT width;
   const UINT height;
};

struct Set_text_dpi {
   const UINT dpi;
};

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

struct Load_patch_effects_config {
   const std::string_view effects_config;
};

struct Destroy_patch_effects_config {
   const Patch_effects_config_handle effects_config;
};

struct Load_colorgrading_regions {
   const std::span<const std::byte> regions_data;
};

struct Destroy_ia_buffer {
   const Buffer_handle buffer_handle;
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
   const std::optional<RECT> rect = std::nullopt;
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
   const Game_input_layout input_layout;
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
   const std::span<const std::byte> constants;
};

struct Set_constants_draw {
   const UINT offset;
   const std::span<const std::byte> constants;
};

struct Set_constants_fixedfunction {
   const cb::Fixedfunction constants;
};

struct Set_constants_skin {
   const UINT offset;
   const std::span<const std::byte> constants;
};

struct Set_constants_draw_ps {
   const UINT offset;
   const std::span<const std::byte> constants;
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

struct Rename_ia_buffer_dynamic_cpu {
   const Buffer_handle buffer_handle;
   const std::size_t index;
};

struct Null {
};

}

struct alignas(std::hardware_constructive_interference_size) Command_data {
   Command type = Command::null;

   union {
      commands::Reset reset;
      commands::Set_text_dpi set_text_dpi;
      commands::Present present;
      commands::Destroy_game_rendertarget destroy_game_rendertarget;
      commands::Destroy_game_texture destroy_game_texture;
      commands::Destroy_patch_texture destroy_patch_texture;
      commands::Load_patch_effects_config load_patch_effects_config;
      commands::Destroy_patch_material destroy_patch_material;
      commands::Load_colorgrading_regions load_colorgrading_regions;
      commands::Destroy_patch_effects_config destroy_patch_effects_config;
      commands::Destroy_ia_buffer destroy_ia_buffer;
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
      commands::Rename_ia_buffer_dynamic_cpu rename_ia_buffer_dynamic_cpu;
      commands::Null null = {};
   };
};

static_assert(std::is_default_constructible_v<Command_data>);
static_assert(std::is_trivially_destructible_v<Command_data>);

class Command_queue {
public:
   constexpr static std::size_t max_queued_commands = 2048;

   void enqueue(const Command_data& command) noexcept
   {
      _queue.enqueue(command);
   }

   [[nodiscard]] auto dequeue() noexcept -> Command_data
   {
      return _queue.dequeue();
   }

   void wait_for_empty()
   {
      _waits.fetch_add(1, std::memory_order_relaxed);
      _queue.wait_for_empty();
   }

   auto wait_count() noexcept -> int
   {
      return _waits.exchange(0, std::memory_order_relaxed);
   }

private:
   single_producer_single_consumer_queue<Command_data, max_queued_commands> _queue;
   std::atomic_int _waits = 0;
};

}
