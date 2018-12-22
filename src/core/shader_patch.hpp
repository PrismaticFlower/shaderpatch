#pragma once

#include "com_ptr.hpp"
#include "constant_buffers.hpp"
#include "d3d11_helpers.hpp"
#include "depthstencil.hpp"
#include "game_input_layout.hpp"
#include "game_rendertarget.hpp"
#include "game_shader.hpp"
#include "game_texture.hpp"
#include "input_layout_descriptions.hpp"
#include "input_layout_element.hpp"
#include "sampler_states.hpp"
#include "shader_database.hpp"
#include "shader_loader.hpp"
#include "shader_metadata.hpp"

#include <vector>

#include <glm/glm.hpp>
#include <gsl/gsl>

#include <DirectXTex.h>
#include <d3d11_1.h>

#pragma warning(disable : 4324)

namespace sp::core {

enum class Game_rendertarget_id : int {};

enum class Game_depthstencil { nearscene, farscene, reflectionscene, none };

enum class Projtex_mode { clamp, wrap };

enum class Projtex_type { tex2d, texcube };

enum class Query_result { success, notready, error };

class Shader_patch {
public:
   Shader_patch(IDXGIAdapter2& adapter, const HWND window) noexcept;

   Shader_patch(const Shader_patch&) = delete;
   Shader_patch& operator=(const Shader_patch&) = delete;

   Shader_patch(Shader_patch&&) = delete;
   Shader_patch& operator=(Shader_patch&&) = delete;

   void reset() noexcept;

   void present() noexcept;

   auto get_back_buffer() noexcept -> Game_rendertarget_id;

   auto create_rasterizer_state(const D3D11_RASTERIZER_DESC d3d11_rasterizer_desc) noexcept
      -> Com_ptr<ID3D11RasterizerState>;

   auto create_depthstencil_state(const D3D11_DEPTH_STENCIL_DESC depthstencil_desc) noexcept
      -> Com_ptr<ID3D11DepthStencilState>;

   auto create_blend_state(const D3D11_BLEND_DESC blend_state_desc) noexcept
      -> Com_ptr<ID3D11BlendState>;

   auto create_query(const D3D11_QUERY_DESC desc) noexcept -> Com_ptr<ID3D11Query>;

   auto create_game_rendertarget(const UINT width, const UINT height) noexcept
      -> Game_rendertarget_id;

   void destroy_game_rendertarget(const Game_rendertarget_id id) noexcept;

   auto create_game_texture2d(const DirectX::ScratchImage& image) noexcept
      -> Game_texture;

   auto create_game_texture3d(const DirectX::ScratchImage& image) noexcept
      -> Game_texture;

   auto create_game_texture_cube(const DirectX::ScratchImage& image) noexcept
      -> Game_texture;

   auto create_game_input_layout(const gsl::span<const Input_layout_element> layout,
                                 const bool compressed) noexcept -> Game_input_layout;

   auto create_game_shader(const Shader_metadata metadata) noexcept
      -> std::shared_ptr<Game_shader>;

   auto create_ia_buffer(const UINT size, const bool vertex_buffer,
                         const bool index_buffer, const bool dynamic) noexcept
      -> Com_ptr<ID3D11Buffer>;

   void update_ia_buffer(ID3D11Buffer& buffer, const UINT offset,
                         const UINT size, const std::byte* data) noexcept;

   auto map_ia_buffer(ID3D11Buffer& buffer, const D3D11_MAP map_type) noexcept
      -> std::byte*;

   void unmap_ia_buffer(ID3D11Buffer& buffer) noexcept;

   void stretch_rendertarget(const Game_rendertarget_id source,
                             const RECT* source_rect, const Game_rendertarget_id dest,
                             const RECT* dest_rect) noexcept;

   void color_fill_rendertarget(const Game_rendertarget_id rendertarget,
                                const glm::vec4 color,
                                const RECT* rect = nullptr) noexcept;

   void clear_rendertarget(const glm::vec4 color) noexcept;

   void clear_depthstencil(const float z, const UINT8 stencil,
                           const bool clear_depth, const bool clear_stencil) noexcept;

   void reset_depthstencil(const Game_depthstencil depthstencil) noexcept;

   void set_index_buffer(ID3D11Buffer& buffer, const UINT offset) noexcept;

   void set_vertex_buffer(ID3D11Buffer& buffer, const UINT offset,
                          const UINT stride) noexcept;

   void set_input_layout(const Game_input_layout& input_layout) noexcept;

   void set_primitive_topology(const D3D11_PRIMITIVE_TOPOLOGY topology) noexcept;

   void set_game_shader(std::shared_ptr<Game_shader> shader) noexcept;

   void set_rendertarget(const Game_rendertarget_id rendertarget) noexcept;

   void set_depthstencil(const Game_depthstencil depthstencil) noexcept;

   void set_viewport(const float x, const float y, const float width,
                     const float height) noexcept;

   void set_rasterizer_state(ID3D11RasterizerState& rasterizer_state) noexcept;

   void set_depthstencil_state(ID3D11DepthStencilState& depthstencil_state,
                               const UINT8 stencil_ref) noexcept;

   void set_blend_state(ID3D11BlendState& blend_state) noexcept;

   void set_fog_state(const bool enabled, const glm::vec4 color) noexcept;

   void set_texture(const UINT slot, const Game_texture& texture) noexcept;

   void set_texture(const UINT slot, const Game_rendertarget_id rendertarget) noexcept;

   void set_projtex_mode(const Projtex_mode mode) noexcept;

   void set_projtex_type(const Projtex_type type) noexcept;

   void set_projtex_cube(const Game_texture& texture) noexcept;

   void set_constants(const cb::Scene_tag, const UINT offset,
                      const gsl::span<const std::array<float, 4>> constants) noexcept;

   void set_constants(const cb::Draw_tag, const UINT offset,
                      const gsl::span<const std::array<float, 4>> constants) noexcept;

   void set_constants(const cb::Skin_tag, const UINT offset,
                      const gsl::span<const std::array<float, 4>> constants) noexcept;

   void set_constants(const cb::Draw_ps_tag, const UINT offset,
                      const gsl::span<const std::array<float, 4>> constants) noexcept;

   void draw(const UINT vertex_count, const UINT start_vertex) noexcept;

   void draw_indexed(const UINT index_count, const UINT start_index,
                     const UINT start_vertex) noexcept;

   void begin_query(ID3D11Query& query) noexcept;

   void end_query(ID3D11Query& query) noexcept;

   auto get_query_data(ID3D11Query& query, const bool flush,
                       gsl::span<std::byte> data) noexcept -> Query_result;

private:
   auto get_backbuffer_views() noexcept -> Game_rendertarget;

   void bind_static_resources() noexcept;

   void update_dirty_state() noexcept;

   constexpr static auto _game_backbuffer_index = Game_rendertarget_id{0};

   const Com_ptr<ID3D11Device1> _device;
   const Com_ptr<ID3D11DeviceContext1> _device_context = [this] {
      Com_ptr<ID3D11DeviceContext1> dc;

      _device->GetImmediateContext1(dc.clear_and_assign());

      return dc;
   }();
   const Com_ptr<IDXGISwapChain1> _swapchain;

   Input_layout_descriptions _input_layout_descriptions;
   const Shader_database _shader_database =
      load_shader_lvl(L"data/shaderpatch/shaders.lvl", *_device);

   std::vector<Game_rendertarget> _game_rendertargets = {get_backbuffer_views()};
   Game_rendertarget_id _current_game_rendertarget = _game_backbuffer_index;

   Depthstencil _nearscene_depthstencil{*_device, 800, 600};
   Depthstencil _farscene_depthstencil{*_device, 800, 600};
   Depthstencil _reflectionscene_depthstencil{*_device, 512, 256};
   Depthstencil* _current_depthstencil = &_nearscene_depthstencil;

   Game_input_layout _game_input_layout{};
   std::shared_ptr<Game_shader> _game_shader{};

   std::array<Game_texture, 4> _game_textures;

   bool _ia_vs_dirty = true;
   bool _om_targets_dirty = true;
   bool _ps_textures_dirty = true;
   bool _cb_scene_dirty = true;
   bool _cb_draw_dirty = true;
   bool _cb_skin_dirty = true;
   bool _cb_draw_ps_dirty = true;

   cb::Scene _cb_scene{};
   cb::Draw _cb_draw{};
   cb::Skin _cb_skin{};
   cb::Draw_ps _cb_draw_ps{};

   const Com_ptr<ID3D11Buffer> _cb_scene_buffer =
      create_dynamic_constant_buffer(*_device, sizeof(_cb_scene));
   const Com_ptr<ID3D11Buffer> _cb_draw_buffer =
      create_dynamic_constant_buffer(*_device, sizeof(_cb_draw));
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

      const std::array<glm::uvec4, 32> data{};

      const auto desc = CD3D11_BUFFER_DESC{data.size(), D3D11_BIND_VERTEX_BUFFER,
                                           D3D11_USAGE_IMMUTABLE, 0};

      const auto init_data = D3D11_SUBRESOURCE_DATA{data.data(), data.size(), 0};

      _device->CreateBuffer(&desc, &init_data, buffer.clear_and_assign());

      return buffer;
   }();

   const Sampler_states _sampler_states{*_device};
};

}

#pragma warning(default : 4324)
