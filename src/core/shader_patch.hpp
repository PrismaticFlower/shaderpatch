#pragma once

#include "../effects/control.hpp"
#include "../effects/profiler.hpp"
#include "../effects/rendertarget_allocator.hpp"
#include "../material/factory.hpp"
#include "../material/material.hpp"
#include "../material/shader_factory.hpp"
#include "../shader/database.hpp"
#include "backbuffer_cmaa2_views.hpp"
#include "buffer.hpp"
#include "com_ptr.hpp"
#include "command_queue.hpp"
#include "common.hpp"
#include "constant_buffers.hpp"
#include "d3d11_helpers.hpp"
#include "depth_msaa_resolver.hpp"
#include "depthstencil.hpp"
#include "game_alt_postprocessing.hpp"
#include "game_input_layout.hpp"
#include "game_rendertarget.hpp"
#include "game_shader.hpp"
#include "game_texture.hpp"
#include "handles.hpp"
#include "image_stretcher.hpp"
#include "input_layout_descriptions.hpp"
#include "input_layout_element.hpp"
#include "late_backbuffer_resolver.hpp"
#include "linear_allocator.hpp"
#include "oit_provider.hpp"
#include "sampler_states.hpp"
#include "small_function.hpp"
#include "swapchain.hpp"
#include "text/font_atlas_builder.hpp"
#include "texture_database.hpp"
#include "texture_loader.hpp"

#include <bit>
#include <mutex>
#include <span>
#include <thread>
#include <vector>

#include <glm/glm.hpp>

#include <DirectXTex.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>

namespace sp {
class BF2_log_monitor;

namespace core {

class Shader_patch {
public:
   Shader_patch(IDXGIAdapter4& adapter, const HWND window, const UINT width,
                const UINT height) noexcept;

   ~Shader_patch();

   Shader_patch(const Shader_patch&) = delete;
   Shader_patch& operator=(const Shader_patch&) = delete;

   Shader_patch(Shader_patch&&) = delete;
   Shader_patch& operator=(Shader_patch&&) = delete;

   void reset_async(const UINT width, const UINT height) noexcept;

   void set_text_dpi_async(const std::uint32_t dpi) noexcept;

   void present_async() noexcept;

   static auto get_back_buffer() noexcept -> Game_rendertarget_id;

   auto create_rasterizer_state(const D3D11_RASTERIZER_DESC d3d11_rasterizer_desc) noexcept
      -> Com_ptr<ID3D11RasterizerState>;

   auto create_depthstencil_state(const D3D11_DEPTH_STENCIL_DESC depthstencil_desc) noexcept
      -> Com_ptr<ID3D11DepthStencilState>;

   auto create_blend_state(const D3D11_BLEND_DESC1 blend_state_desc) noexcept
      -> Com_ptr<ID3D11BlendState1>;

   auto create_game_rendertarget(const UINT width, const UINT height) noexcept
      -> Game_rendertarget_id;

   void destroy_game_rendertarget_async(const Game_rendertarget_id id) noexcept;

   auto create_game_texture2d(const UINT width, const UINT height,
                              const UINT mip_levels, const DXGI_FORMAT format,
                              const std::span<const Mapped_texture> data) noexcept
      -> Game_texture_handle;

   auto create_game_texture3d(const UINT width, const UINT height, const UINT depth,
                              const UINT mip_levels, const DXGI_FORMAT format,
                              const std::span<const Mapped_texture> data) noexcept
      -> Game_texture_handle;

   auto create_game_texture_cube(const UINT width, const UINT height,
                                 const UINT mip_levels, const DXGI_FORMAT format,
                                 const std::span<const Mapped_texture> data) noexcept
      -> Game_texture_handle;

   void destroy_game_texture_async(const Game_texture_handle game_texture_handle) noexcept;

   auto create_patch_texture(const std::span<const std::byte> texture_data) noexcept
      -> Patch_texture_handle;

   void destroy_patch_texture_async(const Patch_texture_handle texture_handle) noexcept;

   auto create_patch_material(const std::span<const std::byte> material_data) noexcept
      -> Material_handle;

   void destroy_patch_material_async(const Material_handle material_handle) noexcept;

   auto create_patch_effects_config(const std::span<const std::byte> effects_config) noexcept
      -> Patch_effects_config_handle;

   void destroy_patch_effects_config_async(const Patch_effects_config_handle effects_config) noexcept;

   auto create_game_input_layout(const std::span<const Input_layout_element> layout,
                                 const bool compressed,
                                 const bool particle_texture_scale) noexcept
      -> Game_input_layout;

   auto create_ia_buffer(const UINT size, const bool vertex_buffer,
                         const bool index_buffer, const bool dynamic) noexcept
      -> Buffer_handle;

   void destroy_ia_buffer_async(const Buffer_handle buffer_handle) noexcept;

   void load_colorgrading_regions(const std::span<const std::byte> regions_data) noexcept;

   void update_ia_buffer(const Buffer_handle buffer_handle, const UINT offset,
                         const UINT size, const std::byte* data) noexcept;

   void update_ia_buffer_dynamic(const Buffer_handle buffer_handle,
                                 const UINT offset, const UINT size,
                                 const std::byte* data, const Map map_type) noexcept;

   void update_texture(const Game_texture_handle game_texture_handle,
                       const std::span<const Mapped_texture> data) noexcept;

   void stretch_rendertarget_async(const Game_rendertarget_id source,
                                   const RECT source_rect,
                                   const Game_rendertarget_id dest,
                                   const RECT dest_rect) noexcept;

   void color_fill_rendertarget_async(const Game_rendertarget_id rendertarget,
                                      const Clear_color color,
                                      const std::optional<RECT> rect = std::nullopt) noexcept;

   void clear_rendertarget_async(const Clear_color color) noexcept;

   void clear_depthstencil_async(const float z, const UINT8 stencil,
                                 const bool clear_depth,
                                 const bool clear_stencil) noexcept;

   void set_index_buffer_async(const Buffer_handle buffer_handle,
                               const UINT offset) noexcept;

   void set_vertex_buffer_async(const Buffer_handle buffer_handle,
                                const UINT offset, const UINT stride) noexcept;

   void set_input_layout_async(const Game_input_layout input_layout) noexcept;

   void set_game_shader_async(const std::uint32_t game_shader_index) noexcept;

   void set_rendertarget_async(const Game_rendertarget_id rendertarget) noexcept;

   void set_depthstencil_async(const Game_depthstencil depthstencil) noexcept;

   void set_rasterizer_state_async(ID3D11RasterizerState& rasterizer_state) noexcept;

   void set_depthstencil_state_async(ID3D11DepthStencilState& depthstencil_state,
                                     const UINT8 stencil_ref,
                                     const bool readonly) noexcept;

   void set_blend_state_async(ID3D11BlendState1& blend_state,
                              const bool additive_blending) noexcept;

   void set_fog_state_async(const bool enabled, const glm::vec4 color) noexcept;

   void set_texture_async(const UINT slot,
                          const Game_texture_handle game_texture_handle) noexcept;

   void set_texture_async(const UINT slot,
                          const Game_rendertarget_id rendertarget) noexcept;

   void set_projtex_mode_async(const Projtex_mode mode) noexcept;

   void set_projtex_type_async(const Projtex_type type) noexcept;

   void set_projtex_cube_async(const Game_texture_handle game_texture_handle) noexcept;

   void set_patch_material_async(const Material_handle material_handle) noexcept;

   /// @brief Allocate memory that is suitable to be passed to calls such as set_constants_async.
   /// The memory's lifetime will be managed by Shader_patch and is guaranteed to have it's lifetime be sufficent for this use.
   /// @param size Size of the memory to allocate.
   /// @return A span to the allocated memory.
   auto allocate_memory_for_async_data(const std::size_t size) -> std::span<std::byte>
   {
      return std::span{_constants_storage_allocator.allocate(size), size};
   }

   void set_constants_async(const cb::Scene_tag, const UINT offset,
                            const std::span<const std::byte> constants) noexcept;

   void set_constants_async(const cb::Draw_tag, const UINT offset,
                            const std::span<const std::byte> constants) noexcept;

   void set_constants_async(const cb::Fixedfunction_tag,
                            const cb::Fixedfunction constants) noexcept;

   void set_constants_async(const cb::Skin_tag, const UINT offset,
                            const std::span<const std::byte> constants) noexcept;

   void set_constants_async(const cb::Draw_ps_tag, const UINT offset,
                            const std::span<const std::byte> constants) noexcept;

   void set_informal_projection_matrix_async(const glm::mat4& matrix) noexcept;

   void draw_async(const D3D11_PRIMITIVE_TOPOLOGY topology,
                   const UINT vertex_count, const UINT start_vertex) noexcept;

   void draw_indexed_async(const D3D11_PRIMITIVE_TOPOLOGY topology,
                           const UINT index_count, const UINT start_index,
                           const UINT start_vertex) noexcept;

   void force_shader_cache_save_to_disk() noexcept;

private:
   void async_command_processor(std::stop_token stop_token) noexcept;

   void process_command(const Command_data& command) noexcept;

   auto discard_ia_buffer_dynamic_cpu(Buffer& buffer) noexcept -> std::size_t;

   void rename_ia_buffer_dynamic_cpu_async(const Buffer_handle buffer_handle,
                                           const std::size_t index) noexcept;

   void reset(const UINT width, const UINT height) noexcept;

   void set_text_dpi(const std::uint32_t dpi) noexcept;

   void present() noexcept;

   void destroy_game_rendertarget(const Game_rendertarget_id id) noexcept;

   void destroy_game_texture(const Game_texture_handle game_texture_handle) noexcept;

   void destroy_patch_texture(const Patch_texture_handle texture_handle) noexcept;

   void destroy_patch_material(const Material_handle material_handle) noexcept;

   void destroy_patch_effects_config(const Patch_effects_config_handle effects_config) noexcept;

   void destroy_ia_buffer(const Buffer_handle buffer_handle) noexcept;

   void stretch_rendertarget(const Game_rendertarget_id source,
                             const RECT source_rect, const Game_rendertarget_id dest,
                             const RECT dest_rect) noexcept;

   void color_fill_rendertarget(const Game_rendertarget_id rendertarget,
                                const Clear_color color,
                                const std::optional<RECT> rect = std::nullopt) noexcept;

   void clear_rendertarget(const Clear_color color) noexcept;

   void clear_depthstencil(const float z, const UINT8 stencil,
                           const bool clear_depth, const bool clear_stencil) noexcept;

   void set_index_buffer(const Buffer_handle buffer_handle, const UINT offset) noexcept;

   void set_vertex_buffer(const Buffer_handle buffer_handle, const UINT offset,
                          const UINT stride) noexcept;

   void set_input_layout(const Game_input_layout input_layout) noexcept;

   void set_game_shader(const std::uint32_t game_shader_index) noexcept;

   void set_rendertarget(const Game_rendertarget_id rendertarget) noexcept;

   void set_depthstencil(const Game_depthstencil depthstencil) noexcept;

   void set_rasterizer_state(ID3D11RasterizerState& rasterizer_state) noexcept;

   void set_depthstencil_state(ID3D11DepthStencilState& depthstencil_state,
                               const UINT8 stencil_ref, const bool readonly) noexcept;

   void set_blend_state(ID3D11BlendState1& blend_state,
                        const bool additive_blending) noexcept;

   void set_fog_state(const bool enabled, const glm::vec4 color) noexcept;

   void set_texture(const UINT slot,
                    const Game_texture_handle game_texture_handle) noexcept;

   void set_texture(const UINT slot, const Game_rendertarget_id rendertarget) noexcept;

   void set_projtex_mode(const Projtex_mode mode) noexcept;

   void set_projtex_type(const Projtex_type type) noexcept;

   void set_projtex_cube(const Game_texture_handle game_texture_handle) noexcept;

   void set_patch_material(const Material_handle material_handle) noexcept;

   void set_constants(const cb::Scene_tag, const UINT offset,
                      const std::span<const std::byte> constants) noexcept;

   void set_constants(const cb::Draw_tag, const UINT offset,
                      const std::span<const std::byte> constants) noexcept;

   void set_constants(const cb::Fixedfunction_tag,
                      const cb::Fixedfunction constants) noexcept;

   void set_constants(const cb::Skin_tag, const UINT offset,
                      const std::span<const std::byte> constants) noexcept;

   void set_constants(const cb::Draw_ps_tag, const UINT offset,
                      const std::span<const std::byte> constants) noexcept;

   void set_informal_projection_matrix(const glm::mat4& matrix) noexcept;

   void draw(const D3D11_PRIMITIVE_TOPOLOGY topology, const UINT vertex_count,
             const UINT start_vertex) noexcept;

   void draw_indexed(const D3D11_PRIMITIVE_TOPOLOGY topology, const UINT index_count,
                     const UINT start_index, const UINT start_vertex) noexcept;

   void rename_ia_buffer_dynamic_cpu(const Buffer_handle buffer_handle,
                                     const std::size_t index) noexcept;

   auto create_game_texture(Com_ptr<ID3D11Resource> texture,
                            const D3D11_SRV_DIMENSION srv_dimension,
                            const DXGI_FORMAT format) noexcept -> Game_texture_handle;

   auto current_depthstencil(const bool readonly) const noexcept
      -> ID3D11DepthStencilView*;

   auto current_depthstencil() const noexcept -> ID3D11DepthStencilView*
   {
      return current_depthstencil(_om_depthstencil_readonly |
                                  _om_depthstencil_force_readonly);
   }

   void bind_static_resources() noexcept;

   void restore_all_game_state() noexcept;

   void game_rendertype_changed() noexcept;

   void update_dirty_state(const D3D11_PRIMITIVE_TOPOLOGY draw_primitive_topology) noexcept;

   void update_shader() noexcept;

   void update_frame_state() noexcept;

   void update_imgui() noexcept;

   void update_effects() noexcept;

   void update_rendertargets() noexcept;

   void update_refraction_target() noexcept;

   void update_samplers() noexcept;

   void update_material_resources() noexcept;

   void set_linear_rendering(bool linear_rendering) noexcept;

   void resolve_refraction_texture() noexcept;

   template<bool restore_state>
   void resolve_msaa_depthstencil() noexcept
   {
      if ((!std::exchange(_msaa_depthstencil_resolve, true)) & (_rt_sample_count != 1)) {
         _depth_msaa_resolver.resolve(*_device_context, _nearscene_depthstencil,
                                      _farscene_depthstencil);

         if constexpr (restore_state) restore_all_game_state();
      }
   }

   void resolve_oit() noexcept;

   void patch_backbuffer_resolve() noexcept;

   constexpr static auto _game_backbuffer_index = Game_rendertarget_id{0};

   const Com_ptr<ID3D11Device5> _device;
   mutable std::mutex _device_context_mutex;
   const Com_ptr<ID3D11DeviceContext4> _device_context = [this] {
      Com_ptr<ID3D11DeviceContext3> dc3;

      _device->GetImmediateContext3(dc3.clear_and_assign());

      Com_ptr<ID3D11DeviceContext4> dc;

      dc3->QueryInterface(dc.clear_and_assign());

      return dc;
   }();
   Swapchain _swapchain;
   UINT64 _current_frame = 1; // Start at 1 so resources can use 0 as their last use.

   Input_layout_descriptions _input_layout_descriptions;
   shader::Database _shader_database{_device,
                                     {.shader_cache = user_config.developer.shader_cache_path,
                                      .shader_definitions =
                                         user_config.developer.shader_definitions_path,
                                      .shader_source_files =
                                         user_config.developer.shader_source_path}};
   shader::Rendertypes_database _shader_rendertypes_database{_shader_database,
                                                             OIT_provider::usable(
                                                                *_device)};

   Game_shader_store _game_shaders{_shader_rendertypes_database};

   std::mutex _game_rendertargets_mutex;
   std::vector<Game_rendertarget> _game_rendertargets = {_swapchain.game_rendertarget()};
   Game_rendertarget_id _current_game_rendertarget = _game_backbuffer_index;
   Game_rendertarget _patch_backbuffer;
   Game_rendertarget _shadow_msaa_rt;
   Game_rendertarget _refraction_rt;
   Game_rendertarget _farscene_refraction_rt;

   Backbuffer_cmaa2_views _backbuffer_cmaa2_views;

   Depthstencil _nearscene_depthstencil;
   Depthstencil _farscene_depthstencil;
   Depthstencil _reflectionscene_depthstencil;
   Game_depthstencil _current_depthstencil_id = Game_depthstencil::nearscene;

   Game_input_layout _game_input_layout{};
   D3D11_PRIMITIVE_TOPOLOGY _primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
   Game_shader* _game_shader = nullptr;
   Rendertype _previous_shader_rendertype = Rendertype::invalid;
   Rendertype _shader_rendertype = Rendertype::invalid;

   std::array<Game_texture, 7> _game_textures;
   material::Material* _patch_material = nullptr;

   Com_ptr<ID3D11Buffer> _game_index_buffer;
   UINT _game_index_buffer_offset = 0;
   Com_ptr<ID3D11Buffer> _game_vertex_buffer;
   UINT _game_vertex_buffer_offset = 0;
   UINT _game_vertex_buffer_stride = 0;
   Com_ptr<ID3D11RasterizerState> _game_rs_state;
   Com_ptr<ID3D11DepthStencilState> _game_depthstencil_state;
   Com_ptr<ID3D11BlendState1> _game_blend_state;

   bool _discard_draw_calls = false;
   bool _shader_rendertype_changed = false;
   bool _shader_dirty = true;
   bool _ia_index_buffer_dirty = true;
   bool _ia_vertex_buffer_dirty = true;
   bool _rs_state_dirty = true;
   bool _om_targets_dirty = true;
   UINT8 _game_stencil_ref = 0xff;
   bool _om_depthstencil_readonly = true;
   bool _om_depthstencil_force_readonly = false;
   bool _om_depthstencil_state_dirty = true;
   bool _om_blend_state_dirty = true;
   bool _ps_textures_dirty = true;
   bool _cb_scene_dirty = true;
   bool _cb_draw_dirty = true;
   bool _cb_skin_dirty = true;
   bool _cb_draw_ps_dirty = true;

   // Frame State
   bool _use_interface_depthstencil = false;
   bool _lock_projtex_cube_slot = false;
   bool _refraction_farscene_texture_resolve = false;
   bool _refraction_nearscene_texture_resolve = false;
   bool _msaa_depthstencil_resolve = false;
   bool _linear_rendering = false;
   bool _oit_active = false;

   bool _imgui_enabled = false;
   bool _screenshot_requested = false;

   Small_function<void(Game_rendertarget&, const RECT, Game_rendertarget&, const RECT) noexcept> _on_stretch_rendertarget;
   Small_function<void() noexcept> _on_rendertype_changed;

   cb::Scene _cb_scene{};
   cb::Draw _cb_draw{};
   cb::Skin _cb_skin{};
   cb::Draw_ps _cb_draw_ps{};

   const Com_ptr<ID3D11Buffer> _cb_scene_buffer =
      create_dynamic_constant_buffer(*_device, sizeof(_cb_scene));
   const Com_ptr<ID3D11Buffer> _cb_draw_buffer =
      create_dynamic_constant_buffer(*_device, sizeof(_cb_draw));
   const Com_ptr<ID3D11Buffer> _cb_fixedfunction_buffer =
      create_dynamic_constant_buffer(*_device, sizeof(cb::Fixedfunction));
   const Com_ptr<ID3D11Buffer> _cb_draw_ps_buffer =
      create_dynamic_constant_buffer(*_device, sizeof(_cb_draw_ps));
   const Com_ptr<ID3D11Buffer> _cb_skin_buffer =
      create_dynamic_texture_buffer(*_device, sizeof(_cb_skin));
   const Com_ptr<ID3D11ShaderResourceView> _cb_skin_buffer_srv = [this] {
      Com_ptr<ID3D11ShaderResourceView> srv;

      const auto desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_BUFFER,
                                          DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                                          sizeof(_cb_skin) / sizeof(glm::vec4)};

      _device->CreateShaderResourceView(_cb_skin_buffer.get(), &desc,
                                        srv.clear_and_assign());

      return srv;
   }();
   const Com_ptr<ID3D11Buffer> _empty_vertex_buffer = [this] {
      Com_ptr<ID3D11Buffer> buffer;

      const std::array<std::byte, 32> data{};

      const auto desc = CD3D11_BUFFER_DESC{data.size(), D3D11_BIND_VERTEX_BUFFER,
                                           D3D11_USAGE_IMMUTABLE, 0};

      const auto init_data = D3D11_SUBRESOURCE_DATA{data.data(), data.size(), 0};

      _device->CreateBuffer(&desc, &init_data, buffer.clear_and_assign());

      return buffer;
   }();

   OIT_provider _oit_provider{_device, _shader_database};

   const Image_stretcher _image_stretcher{*_device, _shader_database};
   Late_backbuffer_resolver _late_backbuffer_resolver{*_device, _shader_database};
   const Depth_msaa_resolver _depth_msaa_resolver{*_device, _shader_database};
   Sampler_states _sampler_states{*_device};
   Shader_resource_database _shader_resource_database{
      load_texture_lvl(L"data/shaderpatch/textures.lvl", *_device)};
   Game_alt_postprocessing _game_postprocessing{*_device, _shader_database};

   std::mutex _game_texture_pool_mutex;
   std::vector<std::unique_ptr<Game_texture>> _game_texture_pool;
   std::mutex _buffer_pool_mutex;
   std::vector<std::unique_ptr<Buffer>> _buffer_pool;

   effects::Control _effects{_device, _shader_database};
   effects::Rendertarget_allocator _rendertarget_allocator{_device};

   material::Factory _material_factory{_device, _shader_rendertypes_database,
                                       _shader_resource_database};
   std::mutex _materials_pool_mutex;
   std::vector<std::unique_ptr<material::Material>> _materials_pool;

   glm::mat4 _informal_projection_matrix;

   const HWND _window;

   bool _effects_active = false;
   DXGI_FORMAT _current_rt_format = Swapchain::format;
   std::uintptr_t _current_effects_id = 0;

   UINT _rt_sample_count = 1;
   Antialiasing_method _aa_method = Antialiasing_method::none;
   Refraction_quality _refraction_quality = Refraction_quality::medium;

   text::Font_atlas_builder _font_atlas_builder{_device};

   std::unique_ptr<BF2_log_monitor> _bf2_log_monitor;
   Linear_allocator<16> _constants_storage_allocator{4'194'304};

   Command_queue _command_queue;
   std::jthread _command_processor_thread{[this](std::stop_token stop_token) {
      async_command_processor(stop_token);
   }};
};

}
}
