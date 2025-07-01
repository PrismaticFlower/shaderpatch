#pragma once

#include "../effects/control.hpp"
#include "../effects/profiler.hpp"
#include "../effects/rendertarget_allocator.hpp"
#include "../material/factory.hpp"
#include "../material/material.hpp"
#include "../material/shader_factory.hpp"
#include "../shader/database.hpp"
#include "../user_config.hpp"
#include "backbuffer_cmaa2_views.hpp"
#include "basic_builtin_textures.hpp"
#include "com_ptr.hpp"
#include "constant_buffers.hpp"
#include "d3d11_helpers.hpp"
#include "depth_msaa_resolver.hpp"
#include "depthstencil.hpp"
#include "game_alt_postprocessing.hpp"
#include "game_input_layout.hpp"
#include "game_rendertarget.hpp"
#include "game_shader.hpp"
#include "game_texture.hpp"
#include "image_stretcher.hpp"
#include "input_layout_descriptions.hpp"
#include "input_layout_element.hpp"
#include "normalized_rect.hpp"
#include "oit_provider.hpp"
#include "patch_effects_config_handle.hpp"
#include "postprocessing/backbuffer_resolver.hpp"
#include "sampler_states.hpp"
#include "small_function.hpp"
#include "swapchain.hpp"
#include "text/font_atlas_builder.hpp"
#include "texture_database.hpp"
#include "texture_loader.hpp"
#include "tools/pixel_inspector.hpp"

#include <span>
#include <vector>

#include <glm/glm.hpp>

#include <DirectXTex.h>
#include <d3d11_4.h>
#include <dxgi1_6.h>

namespace sp {
class BF2_log_monitor;

namespace core {

enum class Game_rendertarget_id : int {};

enum class Game_depthstencil { nearscene, farscene, reflectionscene, none };

enum class Projtex_mode { clamp, wrap };

enum class Projtex_type { tex2d, texcube };

enum class Query_result { success, notready, error };

struct Mapped_texture {
   UINT row_pitch;
   UINT depth_pitch;
   std::byte* data;
};

struct Reset_flags {
   bool legacy_fullscreen = false;
   bool aspect_ratio_hack = false;
};

using Material_handle =
   std::unique_ptr<material::Material, Small_function<void(material::Material*) noexcept>>;

using Texture_handle =
   std::unique_ptr<ID3D11ShaderResourceView, Small_function<void(ID3D11ShaderResourceView*) noexcept>>;

class Shadows_provider;

class Shader_patch {
public:
   Shader_patch(IDXGIAdapter4& adapter, const HWND window,
                const UINT render_width, const UINT render_height,
                const UINT window_width, const UINT window_height) noexcept;

   ~Shader_patch();

   Shader_patch(const Shader_patch&) = delete;
   Shader_patch& operator=(const Shader_patch&) = delete;

   Shader_patch(Shader_patch&&) = delete;
   Shader_patch& operator=(Shader_patch&&) = delete;

   void reset(const Reset_flags flags, const UINT render_width, const UINT render_height,
              const UINT window_width, const UINT window_height) noexcept;

   void set_text_dpi(const std::uint32_t dpi) noexcept;

   void set_expected_aspect_ratio(const float expected_aspect_ratio) noexcept;

   void present() noexcept;

   auto get_back_buffer() noexcept -> Game_rendertarget_id;

   auto create_rasterizer_state(const D3D11_RASTERIZER_DESC d3d11_rasterizer_desc) noexcept
      -> Com_ptr<ID3D11RasterizerState>;

   auto create_depthstencil_state(const D3D11_DEPTH_STENCIL_DESC depthstencil_desc) noexcept
      -> Com_ptr<ID3D11DepthStencilState>;

   auto create_blend_state(const D3D11_BLEND_DESC1 blend_state_desc) noexcept
      -> Com_ptr<ID3D11BlendState1>;

   auto create_query(const D3D11_QUERY_DESC desc) noexcept -> Com_ptr<ID3D11Query>;

   auto create_game_rendertarget(const UINT width, const UINT height) noexcept
      -> Game_rendertarget_id;

   void destroy_game_rendertarget(const Game_rendertarget_id id) noexcept;

   auto create_game_texture2d(const UINT width, const UINT height,
                              const UINT mip_levels, const DXGI_FORMAT format,
                              const std::span<const Mapped_texture> data) noexcept
      -> Game_texture;

   auto create_game_dynamic_texture2d(const Game_texture& texture) noexcept
      -> Game_texture;

   void destroying_game_texture2d(const Game_texture& texture) noexcept;

   auto create_game_texture3d(const UINT width, const UINT height, const UINT depth,
                              const UINT mip_levels, const DXGI_FORMAT format,
                              const std::span<const Mapped_texture> data) noexcept
      -> Game_texture;

   auto create_game_texture_cube(const UINT width, const UINT height,
                                 const UINT mip_levels, const DXGI_FORMAT format,
                                 const std::span<const Mapped_texture> data) noexcept
      -> Game_texture;

   auto create_patch_texture(const std::span<const std::byte> texture_data) noexcept
      -> Texture_handle;

   auto create_patch_material(const std::span<const std::byte> material_data) noexcept
      -> Material_handle;

   auto create_patch_effects_config(const std::span<const std::byte> effects_config) noexcept
      -> Patch_effects_config_handle;

   auto create_game_input_layout(const std::span<const Input_layout_element> layout,
                                 const bool compressed, const bool particle_texture_scale,
                                 const bool vertex_weights) noexcept -> Game_input_layout;

   auto create_ia_buffer(const UINT size, const bool vertex_buffer,
                         const bool index_buffer, const bool dynamic) noexcept
      -> Com_ptr<ID3D11Buffer>;

   void load_colorgrading_regions(const std::span<const std::byte> regions_data) noexcept;

   void update_ia_buffer(ID3D11Buffer& buffer, const UINT offset,
                         const UINT size, const std::byte* data) noexcept;

   auto map_ia_buffer(ID3D11Buffer& buffer, const D3D11_MAP map_type) noexcept
      -> std::byte*;

   void unmap_ia_buffer(ID3D11Buffer& buffer) noexcept;

   auto map_dynamic_texture(const Game_texture& texture, const UINT mip_level,
                            const D3D11_MAP map_type) noexcept -> Mapped_texture;

   void unmap_dynamic_texture(const Game_texture& texture, const UINT mip_level) noexcept;

   void stretch_rendertarget(const Game_rendertarget_id source,
                             const Normalized_rect source_rect,
                             const Game_rendertarget_id dest,
                             const Normalized_rect dest_rect) noexcept;

   void color_fill_rendertarget(const Game_rendertarget_id rendertarget,
                                const glm::vec4 color,
                                const Normalized_rect* rect = nullptr) noexcept;

   void clear_rendertarget(const glm::vec4 color) noexcept;

   void clear_depthstencil(const float z, const UINT8 stencil,
                           const bool clear_depth, const bool clear_stencil) noexcept;

   void set_index_buffer(ID3D11Buffer& buffer, const UINT offset) noexcept;

   void set_vertex_buffer(ID3D11Buffer& buffer, const UINT offset,
                          const UINT stride) noexcept;

   void set_input_layout(const Game_input_layout& input_layout) noexcept;

   void set_game_shader(const std::uint32_t game_shader_index) noexcept;

   void set_rendertarget(const Game_rendertarget_id rendertarget) noexcept;

   void set_depthstencil(const Game_depthstencil depthstencil) noexcept;

   void set_rasterizer_state(ID3D11RasterizerState& rasterizer_state) noexcept;

   void set_depthstencil_state(ID3D11DepthStencilState& depthstencil_state,
                               const UINT8 stencil_ref, const bool readonly) noexcept;

   void set_blend_state(ID3D11BlendState1& blend_state,
                        const bool additive_blending) noexcept;

   void set_fog_state(const bool enabled, const glm::vec4 color) noexcept;

   void set_texture(const UINT slot, const Game_texture& texture) noexcept;

   void set_texture(const UINT slot, const Game_rendertarget_id rendertarget) noexcept;

   void set_projtex_mode(const Projtex_mode mode) noexcept;

   void set_projtex_type(const Projtex_type type) noexcept;

   void set_projtex_cube(const Game_texture& texture) noexcept;

   void set_patch_material(material::Material* material) noexcept;

   void set_constants(const cb::Scene_tag, const UINT offset,
                      const std::span<const std::array<float, 4>> constants) noexcept;

   void set_constants(const cb::Draw_tag, const UINT offset,
                      const std::span<const std::array<float, 4>> constants) noexcept;

   void set_constants(const cb::Fixedfunction_tag,
                      const cb::Fixedfunction constants) noexcept;

   void set_constants(const cb::Skin_tag, const UINT offset,
                      const std::span<const std::array<float, 4>> constants) noexcept;

   void set_constants(const cb::Draw_ps_tag, const UINT offset,
                      const std::span<const std::array<float, 4>> constants) noexcept;

   void set_informal_projection_matrix(const glm::mat4 matrix) noexcept;

   void draw(const D3D11_PRIMITIVE_TOPOLOGY topology, const UINT vertex_count,
             const UINT start_vertex) noexcept;

   void draw_indexed(const D3D11_PRIMITIVE_TOPOLOGY topology, const UINT index_count,
                     const UINT start_index, const INT base_vertex) noexcept;

   void begin_query(ID3D11Query& query) noexcept;

   void end_query(ID3D11Query& query) noexcept;

   auto get_query_data(ID3D11Query& query, const bool flush,
                       std::span<std::byte> data) noexcept -> Query_result;

   void force_shader_cache_save_to_disk() noexcept;

private:
   auto current_depthstencil(const bool readonly) const noexcept
      -> ID3D11DepthStencilView*;

   auto current_depthstencil() const noexcept -> ID3D11DepthStencilView*
   {
      return current_depthstencil(_om_depthstencil_readonly |
                                  _om_depthstencil_material_force_readonly |
                                  _om_depthstencil_force_readonly);
   }

   auto current_nearscene_depthstencil_srv() const noexcept
      -> ID3D11ShaderResourceView*;

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

   void update_team_colors() noexcept;

   void update_material_resources() noexcept;

   void recreate_patch_backbuffer() noexcept;

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
   const Com_ptr<ID3D11DeviceContext4> _device_context = [this] {
      Com_ptr<ID3D11DeviceContext3> dc3;

      _device->GetImmediateContext3(dc3.clear_and_assign());

      Com_ptr<ID3D11DeviceContext4> dc;

      dc3->QueryInterface(dc.clear_and_assign());

      return dc;
   }();
   UINT _render_width = 0;
   UINT _render_height = 0;
   UINT _window_width = 0;
   UINT _window_height = 0;
   float _expected_aspect_ratio = 0.75f;
   Swapchain _swapchain;

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

   std::vector<Game_rendertarget> _game_rendertargets = {_swapchain.game_rendertarget()};
   Game_rendertarget_id _current_game_rendertarget = _game_backbuffer_index;
   Game_rendertarget _patch_backbuffer;
   Game_rendertarget _shadow_msaa_rt;
   Game_rendertarget _refraction_rt;
   Game_rendertarget _farscene_refraction_rt;

   Backbuffer_cmaa2_views _backbuffer_cmaa2_views;

   Depthstencil _nearscene_depthstencil;
   Depthstencil _farscene_depthstencil;
   Depthstencil _interface_depthstencil;
   Depthstencil _reflectionscene_depthstencil;
   Game_depthstencil _current_depthstencil_id = Game_depthstencil::nearscene;

   Game_input_layout _game_input_layout{};
   D3D11_PRIMITIVE_TOPOLOGY _primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
   Game_shader* _game_shader = nullptr;
   Rendertype _previous_shader_rendertype = Rendertype::invalid;
   Rendertype _shader_rendertype = Rendertype::invalid;

   std::array<Game_texture, 4> _game_textures;
   std::array<Game_texture, 2> _extra_game_textures;
   material::Material* _patch_material = nullptr;

   Com_ptr<ID3D11Buffer> _game_index_buffer;
   UINT _game_index_buffer_offset = 0;
   Com_ptr<ID3D11Buffer> _game_vertex_buffer;
   UINT _game_vertex_buffer_offset = 0;
   UINT _game_vertex_buffer_stride = 0;
   Com_ptr<ID3D11RasterizerState> _game_rs_state;
   Com_ptr<ID3D11DepthStencilState> _game_depthstencil_state;
   Com_ptr<ID3D11BlendState1> _game_blend_state_override;
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
   bool _om_depthstencil_material_force_readonly = false;
   bool _om_depthstencil_state_dirty = true;
   bool _om_blend_state_dirty = true;
   bool _ps_textures_dirty = true;
   bool _ps_textures_material_wants_depthstencil = false;
   bool _ps_textures_shader_wants_depthstencil = false;
   bool _ps_extra_textures_dirty = true;
   bool _ps_textures_material_wants_refraction = false;
   bool _ps_textures_shader_wants_refraction = false;
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
   bool _stock_bloom_used = false;
   bool _stock_bloom_used_last_frame = false;
   bool _effects_postprocessing_applied = false;
   bool _override_viewport = false;
   bool _set_aspect_ratio_on_present = false;
   bool _frame_had_shadows = false;
   bool _frame_had_skyfog = false;
   bool _frame_swapped_depthstencil = false;

   bool _use_shadow_maps = true;
   bool _preview_shadow_world = false;
   bool _preview_shadow_world_textured = false;
   bool _overlay_shadow_world_aabbs = false;
   bool _overlay_shadow_world_leaf_patches = false;
   bool _aspect_ratio_hack_enabled = false;
   bool _imgui_enabled = false;
   bool _screenshot_requested = false;

   Small_function<void(const D3D11_PRIMITIVE_TOPOLOGY, const UINT, const UINT) noexcept> _on_draw;
   Small_function<void(const D3D11_PRIMITIVE_TOPOLOGY, const UINT, const UINT, const INT) noexcept> _on_draw_indexed;
   Small_function<void(Game_rendertarget&, const Normalized_rect&,
                       Game_rendertarget&, const Normalized_rect&) noexcept>
      _on_stretch_rendertarget;
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
   const Com_ptr<ID3D11Buffer> _cb_team_colors_buffer =
      create_dynamic_constant_buffer(*_device, sizeof(cb::Team_colors));
   const Com_ptr<ID3D11Buffer> _cb_skin_buffer =
      create_dynamic_structured_buffer(*_device, sizeof(_cb_skin),
                                       sizeof(std::array<glm::vec4, 3>));
   const Com_ptr<ID3D11ShaderResourceView> _cb_skin_buffer_srv = [this] {
      Com_ptr<ID3D11ShaderResourceView> srv;

      const D3D11_SHADER_RESOURCE_VIEW_DESC desc =
         {.Format = DXGI_FORMAT_UNKNOWN,
          .ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
          .Buffer = {
             .FirstElement = 0,
             .NumElements = sizeof(_cb_skin) / sizeof(std::array<glm::vec4, 3>),
          }};

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

   const Com_ptr<ID3D11BlendState1> _shadowquad_blend_state = [this] {
      Com_ptr<ID3D11BlendState1> blend_state;

      const D3D11_BLEND_DESC1 desc{
         .AlphaToCoverageEnable = false,
         .IndependentBlendEnable = false,
         .RenderTarget =
            D3D11_RENDER_TARGET_BLEND_DESC1{
               .BlendEnable = false,
               .LogicOpEnable = false,
               .SrcBlend = D3D11_BLEND_ONE,
               .DestBlend = D3D11_BLEND_ZERO,
               .BlendOp = D3D11_BLEND_OP_ADD,
               .SrcBlendAlpha = D3D11_BLEND_ONE,
               .DestBlendAlpha = D3D11_BLEND_ZERO,
               .BlendOpAlpha = D3D11_BLEND_OP_ADD,
               .LogicOp = D3D11_LOGIC_OP_CLEAR,
               .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED,
            },
      };

      _device->CreateBlendState1(&desc, blend_state.clear_and_assign());

      return blend_state;
   }();

   const Com_ptr<ID3D11BlendState1> _shadow_particle_blend_state = [this] {
      Com_ptr<ID3D11BlendState1> blend_state;

      const D3D11_BLEND_DESC1 desc{
         .AlphaToCoverageEnable = false,
         .IndependentBlendEnable = false,
         .RenderTarget =
            D3D11_RENDER_TARGET_BLEND_DESC1{
               .BlendEnable = true,
               .LogicOpEnable = false,
               .SrcBlend = D3D11_BLEND_ZERO,
               .DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
               .BlendOp = D3D11_BLEND_OP_ADD,
               .SrcBlendAlpha = D3D11_BLEND_ZERO,
               .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
               .BlendOpAlpha = D3D11_BLEND_OP_ADD,
               .LogicOp = D3D11_LOGIC_OP_CLEAR,
               .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED,
            },
      };

      _device->CreateBlendState1(&desc, blend_state.clear_and_assign());

      return blend_state;
   }();
   const Com_ptr<ID3D11BlendState1> _ambient_occlusion_output_blend_state = [this] {
      Com_ptr<ID3D11BlendState1> blend_state;

      const D3D11_BLEND_DESC1 desc{
         .AlphaToCoverageEnable = false,
         .IndependentBlendEnable = false,
         .RenderTarget =
            D3D11_RENDER_TARGET_BLEND_DESC1{
               .BlendEnable = false,
               .LogicOpEnable = false,
               .SrcBlend = D3D11_BLEND_ONE,
               .DestBlend = D3D11_BLEND_ZERO,
               .BlendOp = D3D11_BLEND_OP_ADD,
               .SrcBlendAlpha = D3D11_BLEND_ONE,
               .DestBlendAlpha = D3D11_BLEND_ZERO,
               .BlendOpAlpha = D3D11_BLEND_OP_ADD,
               .LogicOp = D3D11_LOGIC_OP_CLEAR,
               .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_GREEN,
            },
      };

      _device->CreateBlendState1(&desc, blend_state.clear_and_assign());

      return blend_state;
   }();

   OIT_provider _oit_provider{_device, _shader_database};

   const Image_stretcher _image_stretcher{*_device, _shader_database};
   const Depth_msaa_resolver _depth_msaa_resolver{*_device, _shader_database};
   Sampler_states _sampler_states{*_device};
   Shader_resource_database _shader_resource_database{
      load_texture_lvl(L"data/shaderpatch/textures.lvl", *_device)};
   Game_alt_postprocessing _game_postprocessing{*_device, _shader_database};
   postprocessing::Backbuffer_resolver _backbuffer_resolver{_device, _shader_database};

   effects::Control _effects{_device, _shader_database};
   effects::Rendertarget_allocator _rendertarget_allocator{_device};

   material::Factory _material_factory{_device, _shader_rendertypes_database,
                                       _shader_resource_database};
   std::vector<std::unique_ptr<material::Material>> _materials;

   std::unique_ptr<Shadows_provider> _shadows;

   glm::mat4 _informal_projection_matrix;
   glm::mat4 _postprocess_projection_matrix;

   const HWND _window;

   bool _effects_active = false;
   bool _effects_request_soft_skinning = false;
   DXGI_FORMAT _current_rt_format = Swapchain::format;
   int _current_effects_id = 0;

   UINT _rt_sample_count = 1;
   Antialiasing_method _aa_method = Antialiasing_method::none;
   Refraction_quality _refraction_quality = Refraction_quality::medium;

   D3D11_VIEWPORT _viewport_override = {};

   Basic_builtin_textures _basic_builtin_textures{*_device};

   std::unique_ptr<text::Font_atlas_builder> _font_atlas_builder =
      text::Font_atlas_builder::use_scalable_fonts()
         ? std::make_unique<text::Font_atlas_builder>(_device)
         : nullptr;

   std::unique_ptr<BF2_log_monitor> _bf2_log_monitor;

   tools::Pixel_inspector _pixel_inspector{_device, _shader_database};
};
}
}
