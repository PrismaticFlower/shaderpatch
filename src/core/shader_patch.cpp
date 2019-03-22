
#include "shader_patch.hpp"
#include "../input_hooker.hpp"
#include "../logger.hpp"
#include "../user_config.hpp"
#include "context_state_guard.hpp"
#include "patch_material_io.hpp"
#include "patch_texture_io.hpp"
#include "utility.hpp"

#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"

#include <chrono>

#include <comdef.h>

using namespace std::literals;

namespace sp::core {

constexpr auto custom_tex_begin = 4;
constexpr auto projtex_cube_slot = 127;
constexpr auto shadow_texture_format = DXGI_FORMAT_A8_UNORM;

namespace {

constexpr std::array supported_feature_levels{D3D_FEATURE_LEVEL_11_0};

auto create_device(IDXGIAdapter2& adapater) noexcept -> Com_ptr<ID3D11Device1>
{
   Com_ptr<ID3D11Device> device;

   UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
   const auto d3d_debug =
      user_config.developer.use_d3d11_debug_layer && IsDebuggerPresent();

   if (d3d_debug) create_flags |= D3D11_CREATE_DEVICE_DEBUG;

   if (const auto result =
          D3D11CreateDevice(&adapater, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                            create_flags, supported_feature_levels.data(),
                            supported_feature_levels.size(), D3D11_SDK_VERSION,
                            device.clear_and_assign(), nullptr, nullptr);
       FAILED(result)) {

      log_and_terminate("Failed to create Direct3D 11 device! Reason: ",
                        _com_error{result}.ErrorMessage());
   }

   Com_ptr<ID3D11Device1> device1;

   if (const auto result = device->QueryInterface(device1.clear_and_assign());
       FAILED(result)) {
      log_and_terminate("Failed to create Direct3D 11.1 device!");
   }

   if (d3d_debug) {
      Com_ptr<ID3D11Debug> debug;
      if (auto result = device->QueryInterface(debug.clear_and_assign());
          SUCCEEDED(result)) {
         Com_ptr<ID3D11InfoQueue> infoqueue;
         if (result = debug->QueryInterface(infoqueue.clear_and_assign());
             SUCCEEDED(result)) {
            infoqueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            infoqueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);

            std::array hide{D3D11_MESSAGE_ID_DEVICE_DRAW_VIEW_DIMENSION_MISMATCH};

            D3D11_INFO_QUEUE_FILTER filter{};
            filter.DenyList.NumIDs = hide.size();
            filter.DenyList.pIDList = hide.data();
            infoqueue->AddStorageFilterEntries(&filter);
         }
      }
   }

   return device1;
}

}

Shader_patch::Shader_patch(IDXGIAdapter2& adapter, const HWND window,
                           const UINT width, const UINT height) noexcept
   : _device{create_device(adapter)},
     _swapchain{_device, adapter, window, width, height},
     _window{window},
     _nearscene_depthstencil{*_device, width, height,
                             to_sample_count(user_config.graphics.antialiasing_method)},
     _farscene_depthstencil{*_device, width, height, 1},
     _reflectionscene_depthstencil{*_device, 512, 256, 1},
     _refraction_rt{*_device, Swapchain::format, width / 2, height / 2}
{
   bind_static_resources();
   set_viewport(0, 0, static_cast<float>(width), static_cast<float>(height));

   if (_rt_sample_count > 1) {
      _patch_backbuffer = {*_device, Swapchain::format, _swapchain.width(),
                           _swapchain.height(), _rt_sample_count};
      _shadow_msaa_rt = {*_device,           shadow_texture_format,
                         _swapchain.width(), _swapchain.height(),
                         _rt_sample_count,   Game_rt_type::shadow};
      _game_rendertargets[0] = _patch_backbuffer.game_rendertarget();
   }

   _cb_scene.vertex_color_srgb = true;
   _cb_draw_ps.rt_multiply_blending_state = 0.0f;
   _cb_draw_ps.stock_tonemap_state = 0.0f;
   _cb_draw_ps.cube_projtex = false;

   initialize_input_hooks(GetCurrentThreadId(), window);
   set_input_hotkey(user_config.developer.toggle_key);
   set_input_hotkey_func([this]() noexcept {
      if (!std::exchange(_imgui_enabled, !_imgui_enabled))
         set_input_mode(Input_mode::imgui);
      else
         set_input_mode(Input_mode::normal);
   });

   ImGui::CreateContext();
   ImGui_ImplWin32_Init(window);
   ImGui_ImplDX11_Init(_device.get(), _device_context.get());
   ImGui_ImplDX11_NewFrame();
   ImGui_ImplWin32_NewFrame();
   ImGui::NewFrame();
}

void Shader_patch::reset(const UINT width, const UINT height) noexcept
{
   _device_context->ClearState();
   _game_rendertargets.clear();

   bind_static_resources();
   set_viewport(0, 0, static_cast<float>(width), static_cast<float>(height));

   _swapchain.resize(width, height);
   _game_rendertargets.emplace_back() = _swapchain.game_rendertarget();
   _nearscene_depthstencil = {*_device, width, height, _rt_sample_count};
   _farscene_depthstencil = {*_device, width, height, 1};
   _refraction_rt = {*_device, Swapchain::format, width / 2, height / 2};
   _current_game_rendertarget = _game_backbuffer_index;
   _primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
   _patch_material_topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
   _game_input_layout = {};
   _game_shader = {};
   _game_textures = {};

   if (_rt_sample_count > 1) {
      _patch_backbuffer = {*_device, Swapchain::format, _swapchain.width(),
                           _swapchain.height(), _rt_sample_count};
      _shadow_msaa_rt = {*_device,           shadow_texture_format,
                         _swapchain.width(), _swapchain.height(),
                         _rt_sample_count,   Game_rt_type::shadow};
      _game_rendertargets[0] = _patch_backbuffer.game_rendertarget();
   }

   _shader_dirty = true;
   _om_targets_dirty = true;
   _ps_textures_dirty = true;
   _cb_scene_dirty = true;
   _cb_draw_dirty = true;
   _cb_skin_dirty = true;
   _cb_draw_ps_dirty = true;
}

void Shader_patch::present() noexcept
{
   _effects.profiler.end_frame(*_device_context);

   update_imgui();

   if (_game_rendertargets[0].type != Game_rt_type::presentation)
      patch_backbuffer_resolve();

   _swapchain.present();
   _om_targets_dirty = true;

   update_frame_state();
   update_effects();
   update_aa_rendertargets();
   update_samplers();

   if (_patch_backbuffer)
      _game_rendertargets[0] = _patch_backbuffer.game_rendertarget();

   if (_interface_depthstencil)
      _device_context->ClearDepthStencilView(_interface_depthstencil.dsv.get(),
                                             D3D11_CLEAR_DEPTH, 1.0f, 0x0);
}

auto Shader_patch::get_back_buffer() noexcept -> Game_rendertarget_id
{
   return _game_backbuffer_index;
}

auto Shader_patch::create_rasterizer_state(const D3D11_RASTERIZER_DESC d3d11_rasterizer_desc) noexcept
   -> Com_ptr<ID3D11RasterizerState>
{
   Com_ptr<ID3D11RasterizerState> state;

   _device->CreateRasterizerState(&d3d11_rasterizer_desc, state.clear_and_assign());

   return state;
}

auto Shader_patch::create_depthstencil_state(const D3D11_DEPTH_STENCIL_DESC depthstencil_desc) noexcept
   -> Com_ptr<ID3D11DepthStencilState>
{
   Com_ptr<ID3D11DepthStencilState> state;

   _device->CreateDepthStencilState(&depthstencil_desc, state.clear_and_assign());

   return state;
}

auto Shader_patch::create_blend_state(const D3D11_BLEND_DESC blend_state_desc) noexcept
   -> Com_ptr<ID3D11BlendState>
{
   Com_ptr<ID3D11BlendState> state;

   _device->CreateBlendState(&blend_state_desc, state.clear_and_assign());

   return state;
}

auto Shader_patch::create_query(const D3D11_QUERY_DESC desc) noexcept
   -> Com_ptr<ID3D11Query>
{
   Com_ptr<ID3D11Query> query;

   _device->CreateQuery(&desc, query.clear_and_assign());

   return query;
}

auto Shader_patch::create_game_rendertarget(const UINT width, const UINT height) noexcept
   -> Game_rendertarget_id
{
   const int index = _game_rendertargets.size();
   _game_rendertargets.emplace_back(*_device, current_rt_format(), width, height);

   return Game_rendertarget_id{index};
}

void Shader_patch::destroy_game_rendertarget(const Game_rendertarget_id id) noexcept
{
   _game_rendertargets[static_cast<int>(id)] = {};

   if (id == _current_game_rendertarget) {
      set_rendertarget(_game_backbuffer_index);
   }
}

auto Shader_patch::create_game_texture2d(const UINT width, const UINT height,
                                         const UINT mip_levels, const DXGI_FORMAT format,
                                         const gsl::span<const Mapped_texture> data) noexcept
   -> Game_texture
{
   Expects(width != 0 && height != 0 && mip_levels != 0);

   const auto typeless_format = DirectX::MakeTypeless(format);

   const auto desc = CD3D11_TEXTURE2D_DESC{typeless_format,
                                           width,
                                           height,
                                           1,
                                           mip_levels,
                                           D3D11_BIND_SHADER_RESOURCE,
                                           D3D11_USAGE_IMMUTABLE};

   const auto initial_data = [&] {
      std::vector<D3D11_SUBRESOURCE_DATA> initial_data;
      initial_data.reserve(data.size());

      for (const auto& subres : data) {
         auto& init = initial_data.emplace_back();

         init.pSysMem = subres.data;
         init.SysMemPitch = subres.row_pitch;
         init.SysMemSlicePitch = subres.depth_pitch;
      }

      return initial_data;
   }();

   Com_ptr<ID3D11Texture2D> texture;

   if (const auto result = _device->CreateTexture2D(&desc, initial_data.data(),
                                                    texture.clear_and_assign());
       FAILED(result)) {
      log(Log_level::error, "Failed to create game texture! reason: ",
          _com_error{result}.ErrorMessage());

      return {};
   }

   Com_ptr<ID3D11ShaderResourceView> srv;
   {
      const auto srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE2D, format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srv_desc,
                                               srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   const auto srgb_format = DirectX::MakeSRGB(format);

   if (format == srgb_format) return Game_texture{srv, srv};

   Com_ptr<ID3D11ShaderResourceView> srgb_srv;
   {
      const auto srgb_srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE2D, srgb_format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srgb_srv_desc,
                                               srgb_srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRGB SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   return Game_texture{std::move(srv), std::move(srgb_srv)};
}

auto Shader_patch::create_game_dynamic_texture2d(const Game_texture& texture) noexcept
   -> Game_texture
{
   Expects(texture.srv && texture.srgb_srv);

   Com_ptr<ID3D11Resource> source_resource;
   texture.srv->GetResource(source_resource.clear_and_assign());

   Com_ptr<ID3D11Texture2D> source_texture;
   if (const auto result =
          source_resource->QueryInterface(source_texture.clear_and_assign());
       FAILED(result)) {
      log_and_terminate(
         "Attempt to create dynamic 2D texture using non-2D texture.");
   }

   D3D11_TEXTURE2D_DESC desc;
   source_texture->GetDesc(&desc);

   desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
   desc.Usage = D3D11_USAGE_DYNAMIC;

   Com_ptr<ID3D11Texture2D> dest_texture;
   if (const auto result =
          _device->CreateTexture2D(&desc, nullptr, dest_texture.clear_and_assign());
       FAILED(result)) {
      log(Log_level::error, "Failed to create game texture! reason: ",
          _com_error{result}.ErrorMessage());

      return {};
   }

   _device_context->CopyResource(dest_texture.get(), source_texture.get());

   const auto view_format = DirectX::MakeTypelessUNORM(desc.Format);

   Com_ptr<ID3D11ShaderResourceView> srv;
   {
      const auto srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE2D, view_format};

      if (const auto result =
             _device->CreateShaderResourceView(dest_texture.get(), &srv_desc,
                                               srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   const auto srgb_view_format = DirectX::MakeSRGB(view_format);

   if (view_format == srgb_view_format) {
      return Game_texture{srv, srv};
   }

   Com_ptr<ID3D11ShaderResourceView> srgb_srv;
   {
      const auto srgb_srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE2D,
                                          srgb_view_format};

      if (const auto result =
             _device->CreateShaderResourceView(dest_texture.get(), &srgb_srv_desc,
                                               srgb_srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRGB SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   return Game_texture{std::move(srv), std::move(srgb_srv)};
}

auto Shader_patch::create_game_texture3d(const UINT width, const UINT height,
                                         const UINT depth, const UINT mip_levels,
                                         const DXGI_FORMAT format,
                                         const gsl::span<const Mapped_texture> data) noexcept
   -> Game_texture
{
   Expects(width != 0 && height != 0 && depth != 0 && mip_levels != 0);

   const auto typeless_format = DirectX::MakeTypeless(format);

   const auto desc = CD3D11_TEXTURE3D_DESC{typeless_format,
                                           width,
                                           height,
                                           depth,
                                           mip_levels,
                                           D3D11_BIND_SHADER_RESOURCE,
                                           D3D11_USAGE_IMMUTABLE};

   const auto initial_data = [&] {
      std::vector<D3D11_SUBRESOURCE_DATA> initial_data;
      initial_data.reserve(data.size());

      for (const auto& subres : data) {
         auto& init = initial_data.emplace_back();

         init.pSysMem = subres.data;
         init.SysMemPitch = subres.row_pitch;
         init.SysMemSlicePitch = subres.depth_pitch;
      }

      return initial_data;
   }();

   Com_ptr<ID3D11Texture3D> texture;

   if (const auto result = _device->CreateTexture3D(&desc, initial_data.data(),
                                                    texture.clear_and_assign());
       FAILED(result)) {
      log(Log_level::error, "Failed to create game texture! reason: ",
          _com_error{result}.ErrorMessage());

      return {};
   }

   Com_ptr<ID3D11ShaderResourceView> srv;
   {
      const auto srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE3D, format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srv_desc,
                                               srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   const auto srgb_format = DirectX::MakeSRGB(format);

   if (format == srgb_format) return Game_texture{srv, srv};

   Com_ptr<ID3D11ShaderResourceView> srgb_srv;
   {
      const auto srgb_srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURE3D, srgb_format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srgb_srv_desc,
                                               srgb_srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRGB SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   return Game_texture{std::move(srv), std::move(srgb_srv)};
}

auto Shader_patch::create_game_texture_cube(const UINT width, const UINT height,
                                            const UINT mip_levels,
                                            const DXGI_FORMAT format,
                                            const gsl::span<const Mapped_texture> data) noexcept
   -> Game_texture
{
   Expects(width != 0 && height != 0 && mip_levels != 0);

   const auto typeless_format = DirectX::MakeTypeless(format);

   const auto desc = CD3D11_TEXTURE2D_DESC{typeless_format,
                                           width,
                                           height,
                                           6,
                                           mip_levels,
                                           D3D11_BIND_SHADER_RESOURCE,
                                           D3D11_USAGE_IMMUTABLE,
                                           0,
                                           1,
                                           0,
                                           D3D11_RESOURCE_MISC_TEXTURECUBE};

   const auto initial_data = [&] {
      std::vector<D3D11_SUBRESOURCE_DATA> initial_data;
      initial_data.reserve(data.size());

      for (const auto& subres : data) {
         auto& init = initial_data.emplace_back();

         init.pSysMem = subres.data;
         init.SysMemPitch = subres.row_pitch;
         init.SysMemSlicePitch = subres.depth_pitch;
      }

      return initial_data;
   }();

   Com_ptr<ID3D11Texture2D> texture;

   if (const auto result = _device->CreateTexture2D(&desc, initial_data.data(),
                                                    texture.clear_and_assign());
       FAILED(result)) {
      log(Log_level::error, "Failed to create game texture! reason: ",
          _com_error{result}.ErrorMessage());

      return {};
   }

   Com_ptr<ID3D11ShaderResourceView> srv;
   {
      const auto srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURECUBE, format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srv_desc,
                                               srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   const auto srgb_format = DirectX::MakeSRGB(format);

   if (format == srgb_format) return Game_texture{srv, srv};

   Com_ptr<ID3D11ShaderResourceView> srgb_srv;
   {
      const auto srgb_srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{D3D11_SRV_DIMENSION_TEXTURECUBE, srgb_format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srgb_srv_desc,
                                               srgb_srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRGB SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   return Game_texture{std::move(srv), std::move(srgb_srv)};
}

auto Shader_patch::create_patch_texture(const gsl::span<const std::byte> texture_data) noexcept
   -> Patch_texture
{
   auto [srv, name] =
      load_patch_texture(ucfb::Reader_strict<"sptx"_mn>{texture_data}, *_device);

   auto shared_srv = make_shared_com_ptr(srv);

   _texture_database.add(name, shared_srv);

   return {std::move(shared_srv)};
}

auto Shader_patch::create_patch_material(const gsl::span<const std::byte> material_data) noexcept
   -> std::shared_ptr<Patch_material>
{
   return std::make_shared<Patch_material>(read_patch_material(
                                              ucfb::Reader_strict<"matl"_mn>{material_data}),
                                           _material_shader_factory,
                                           _texture_database, *_device);
}

auto Shader_patch::create_patch_effects_config(const gsl::span<const std::byte> effects_config) noexcept
   -> Patch_effects_config_handle
{
   try {
      std::string config_str{reinterpret_cast<const char*>(effects_config.data()),
                             static_cast<std::size_t>(effects_config.size())};

      while (config_str.back() == '\0') config_str.pop_back();

      _effects.read_config(YAML::Load(config_str));
      _effects.enabled(true);

      const auto fx_id = _current_effects_id += 1;

      const auto on_destruction = [fx_id, this]() noexcept {
         if (fx_id != _current_effects_id) return;

         _effects.enabled(false);
      };

      return {on_destruction};
   }
   catch (std::exception& e) {
      log_and_terminate("Exception occured while loading FX Config: "sv, e.what());
   }
}

auto Shader_patch::create_game_input_layout(const gsl::span<const Input_layout_element> layout,
                                            const bool compressed) noexcept -> Game_input_layout
{
   return {_input_layout_descriptions.try_add(layout), compressed};
}

auto Shader_patch::create_game_shader(const Shader_metadata metadata) noexcept
   -> std::shared_ptr<Game_shader>
{
   auto& state =
      _shader_database->rendertypes.at(metadata.rendertype_name).at(metadata.shader_name);

   auto vertex_shader = state.vertex.at_if(metadata.vertex_shader_flags);

   auto [vs, vs_bytecode, vs_inputlayout] =
      vertex_shader.value_or(decltype(vertex_shader)::value_type{});

   auto vertex_shader_compressed = state.vertex.at_if(
      metadata.vertex_shader_flags | Vertex_shader_flags::compressed);

   auto [vs_compressed, vs_bytecode_compressed, vs_inputlayout_compressed] =
      vertex_shader_compressed.value_or(decltype(vertex_shader)::value_type{});

   auto game_shader = std::make_shared<Game_shader>(
      Game_shader{std::move(vs),
                  std::move(vs_compressed),
                  state.pixel.at(metadata.pixel_shader_flags),
                  metadata.rendertype,
                  metadata.srgb_state,
                  metadata.shader_name,
                  metadata.vertex_shader_flags,
                  metadata.pixel_shader_flags,
                  {std::move(vs_inputlayout), std::move(vs_bytecode)},
                  {std::move(vs_inputlayout_compressed),
                   std::move(vs_bytecode_compressed)}});

   if (!game_shader->vs && !game_shader->vs_compressed)
      log_and_terminate("Game_shader has no vertex shader!");
   if (!game_shader->ps) log_and_terminate("Game_shader has no pixel shader!");

   return game_shader;
}

auto Shader_patch::create_ia_buffer(const UINT size, const bool vertex_buffer,
                                    const bool index_buffer, const bool dynamic) noexcept
   -> Com_ptr<ID3D11Buffer>
{
   Com_ptr<ID3D11Buffer> buffer;

   const UINT bind_flags = (vertex_buffer ? D3D11_BIND_VERTEX_BUFFER : 0) |
                           (index_buffer ? D3D11_BIND_INDEX_BUFFER : 0);
   const auto usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
   const UINT cpu_access = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

   const auto desc = CD3D11_BUFFER_DESC{size, bind_flags, usage, cpu_access};

   if (const auto result =
          _device->CreateBuffer(&desc, nullptr, buffer.clear_and_assign());
       FAILED(result)) {
      log_and_terminate("Failed to create game IA buffer! reason: ",
                        _com_error{result}.ErrorMessage());
   }

   return buffer;
}

void Shader_patch::update_ia_buffer(ID3D11Buffer& buffer, const UINT offset,
                                    const UINT size, const std::byte* data) noexcept
{
   const D3D11_BOX box{offset, 0, 0, offset + size, 1, 1};

   _device_context->UpdateSubresource(&buffer, 0, &box, data, 0, 0);
}

auto Shader_patch::map_ia_buffer(ID3D11Buffer& buffer, const D3D11_MAP map_type) noexcept
   -> std::byte*
{
   D3D11_MAPPED_SUBRESOURCE mapped;

   _device_context->Map(&buffer, 0, map_type, 0, &mapped);

   return static_cast<std::byte*>(mapped.pData);
}

void Shader_patch::unmap_ia_buffer(ID3D11Buffer& buffer) noexcept
{
   _device_context->Unmap(&buffer, 0);
}

auto Shader_patch::map_dynamic_texture(const Game_texture& texture, const UINT mip_level,
                                       const D3D11_MAP map_type) noexcept -> Mapped_texture
{
   Com_ptr<ID3D11Resource> resource;
   texture.srv->GetResource(resource.clear_and_assign());

   D3D11_MAPPED_SUBRESOURCE mapped;
   _device_context->Map(resource.get(), mip_level, map_type, 0, &mapped);

   return {mapped.RowPitch, mapped.DepthPitch, static_cast<std::byte*>(mapped.pData)};
}

void Shader_patch::unmap_dynamic_texture(const Game_texture& texture,
                                         const UINT mip_level) noexcept
{
   Com_ptr<ID3D11Resource> resource;
   texture.srv->GetResource(resource.clear_and_assign());

   _device_context->Unmap(resource.get(), mip_level);
}

void Shader_patch::stretch_rendertarget(const Game_rendertarget_id source,
                                        const RECT* source_rect,
                                        const Game_rendertarget_id dest,
                                        const RECT* dest_rect) noexcept
{
   auto& src_rt = _game_rendertargets[static_cast<int>(source)];
   auto& dest_rt = _game_rendertargets[static_cast<int>(dest)];

   if (_on_stretch_rendertarget)
      _on_stretch_rendertarget(src_rt, source_rect, dest_rt, dest_rect);

   const auto src_box = rect_to_box(
      source_rect ? *source_rect : RECT{0, 0, src_rt.width, src_rt.height});
   const auto dest_box =
      rect_to_box(dest_rect ? *dest_rect : RECT{0, 0, src_rt.width, src_rt.height});

   // Skip any fullscreen resolve or copy operation, as these will be handled as special cases by the shaders that use them.
   if (glm::uvec2{src_rt.width, src_rt.height} ==
          glm::uvec2{dest_rt.width, dest_rt.height} &&
       glm::uvec2{src_rt.width, src_rt.height} ==
          glm::uvec2{src_box.right - src_box.left, src_box.bottom - src_box.top} &&
       glm::uvec2{dest_rt.width, dest_rt.height} ==
          glm::uvec2{dest_box.right - dest_box.left, dest_box.bottom - dest_box.top}) {
      return;
   }

   Context_state_guard<state_flags::ia_all | state_flags::vs_all | state_flags::hs_shader |
                       state_flags::ds_shader | state_flags::gs_shader |
                       state_flags::rs_all | state_flags::ps_all | state_flags::om_all>
      state_guard{*_device_context};

   _image_stretcher.stretch(*_device_context, src_box, src_rt, dest_box, dest_rt);
}

void Shader_patch::color_fill_rendertarget(const Game_rendertarget_id rendertarget,
                                           const glm::vec4 color, const RECT* rect) noexcept
{
   _device_context
      ->ClearView(_game_rendertargets[static_cast<int>(rendertarget)].rtv.get(),
                  &color.x, rect, rect ? 1 : 0);
}

void Shader_patch::clear_rendertarget(const glm::vec4 color) noexcept
{
   _device_context->ClearRenderTargetView(
      _game_rendertargets[static_cast<int>(_current_game_rendertarget)].rtv.get(),
      &color.x);
}

void Shader_patch::clear_depthstencil(const float depth, const UINT8 stencil,
                                      const bool clear_depth,
                                      const bool clear_stencil) noexcept
{
   auto* const dsv = current_depthstencil();

   if (!dsv) return;

   const UINT clear_flags = (clear_depth ? D3D11_CLEAR_DEPTH : 0) |
                            (clear_stencil ? D3D11_CLEAR_STENCIL : 0);

   _device_context->ClearDepthStencilView(dsv, clear_flags, depth, stencil);
}

void Shader_patch::reset_depthstencil(const Game_depthstencil depthstencil) noexcept
{
   constexpr auto flags = D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;

   if (depthstencil == Game_depthstencil::nearscene)
      _device_context->ClearDepthStencilView(_nearscene_depthstencil.dsv.get(),
                                             flags, 0.0f, 0x00);
   else if (depthstencil == Game_depthstencil::farscene)
      _device_context->ClearDepthStencilView(_farscene_depthstencil.dsv.get(),
                                             flags, 0.0f, 0x00);
   else if (depthstencil == Game_depthstencil::reflectionscene)
      _device_context->ClearDepthStencilView(_reflectionscene_depthstencil.dsv.get(),
                                             flags, 0.0f, 0x00);
}

void Shader_patch::set_index_buffer(ID3D11Buffer& buffer, const UINT offset) noexcept
{
   _device_context->IASetIndexBuffer(&buffer, DXGI_FORMAT_R16_UINT, offset);
}

void Shader_patch::set_vertex_buffer(ID3D11Buffer& buffer, const UINT offset,
                                     const UINT stride) noexcept
{
   auto* ptr_buffer = &buffer;

   _device_context->IASetVertexBuffers(0, 1, &ptr_buffer, &stride, &offset);
}

void Shader_patch::set_input_layout(const Game_input_layout& input_layout) noexcept
{
   _game_input_layout = input_layout;
   _shader_dirty = true;
}

void Shader_patch::set_game_shader(std::shared_ptr<Game_shader> shader) noexcept
{
   _game_shader = shader;
   _shader_dirty = true;

   const auto rendertype = _game_shader->rendertype;

   _shader_rendertype_changed =
      std::exchange(_shader_rendertype, rendertype) != rendertype;
}

void Shader_patch::set_rendertarget(const Game_rendertarget_id rendertarget) noexcept
{
   _om_targets_dirty = true;
   _current_game_rendertarget = rendertarget;
}

void Shader_patch::set_depthstencil(const Game_depthstencil depthstencil) noexcept
{
   _om_targets_dirty = true;
   _current_depthstencil_id = depthstencil;
}

void Shader_patch::set_viewport(float x, float y, float width, float height) noexcept
{
   const auto viewport = CD3D11_VIEWPORT{x, y, width, height};

   _device_context->RSSetViewports(1, &viewport);

   _cb_scene_dirty = true;
   _cb_scene.tessellation_resolution_factor =
      std::sqrt((width * height) / (1920.0f * 1080.0f));
}

void Shader_patch::set_rasterizer_state(ID3D11RasterizerState& rasterizer_state) noexcept
{
   _device_context->RSSetState(&rasterizer_state);
}

void Shader_patch::set_depthstencil_state(ID3D11DepthStencilState& depthstencil_state,
                                          const UINT8 stencil_ref) noexcept
{
   _device_context->OMSetDepthStencilState(&depthstencil_state, stencil_ref);
}

void Shader_patch::set_blend_state(ID3D11BlendState& blend_state) noexcept
{
   constexpr std::array blend_factor{1.f, 1.f, 1.f, 1.f};

   _device_context->OMSetBlendState(&blend_state, blend_factor.data(), 0xffffffff);
}

void Shader_patch::set_fog_state(const bool enabled, const glm::vec4 color) noexcept
{
   _cb_draw_ps_dirty = true;
   _cb_draw_ps.fog_enabled = enabled;
   _cb_draw_ps.fog_color = color;
}

void Shader_patch::set_texture(const UINT slot, const Game_texture& texture) noexcept
{
   Expects(slot < 4);

   _game_textures[slot] = texture;
   _ps_textures_dirty = true;
}

void Shader_patch::set_texture(const UINT slot,
                               const Game_rendertarget_id rendertarget) noexcept
{
   Expects(slot < 4);

   const auto& srv = _game_rendertargets[static_cast<int>(rendertarget)].srv;

   _game_textures[slot] = {srv, srv};
   _ps_textures_dirty = true;
   _om_targets_dirty = true;

   _device_context->OMSetRenderTargets(0, nullptr, nullptr);
}

void Shader_patch::set_projtex_mode(const Projtex_mode mode) noexcept
{
   if (mode == Projtex_mode::clamp) {
      auto* const sampler = _sampler_states.linear_clamp_sampler.get();
      _device_context->PSSetSamplers(3, 1, &sampler);
   }
   else if (mode == Projtex_mode::wrap) {
      auto* const sampler = _sampler_states.linear_wrap_sampler.get();
      _device_context->PSSetSamplers(3, 1, &sampler);
   }
}

void Shader_patch::set_projtex_type(const Projtex_type type) noexcept
{
   if (type == Projtex_type::tex2d) {
      _cb_draw_ps.cube_projtex = false;
   }
   else if (type == Projtex_type::texcube) {
      _cb_draw_ps.cube_projtex = true;
   }

   _cb_draw_ps_dirty = true;
}

void Shader_patch::set_projtex_cube(const Game_texture& texture) noexcept
{
   auto* const srv = texture.srv.get();

   _device_context->PSSetShaderResources(projtex_cube_slot, 1, &srv);
}

void Shader_patch::set_patch_material(std::shared_ptr<Patch_material> material) noexcept
{
   _patch_material = std::move(material);

   _shader_dirty = true;

   if (_patch_material) {
      auto* const cb = _patch_material->constant_buffer.get();
      _device_context->PSSetConstantBuffers(2, 1, &cb);

      _game_textures[0] = _patch_material->fail_safe_game_texture;

      _patch_material->bind_constant_buffers(*_device_context);
      _patch_material->bind_shader_resources(*_device_context);
   }
}

void Shader_patch::set_constants(const cb::Scene_tag, const UINT offset,
                                 const gsl::span<const std::array<float, 4>> constants) noexcept
{
   _cb_scene_dirty = true;

   std::memcpy(bit_cast<std::byte*>(&_cb_scene) +
                  (offset * sizeof(std::array<float, 4>)),
               constants.data(), constants.size_bytes());

   if (offset < (offsetof(cb::Scene, near_scene_fade) / sizeof(glm::vec4))) {
      _cb_draw_ps_dirty = true;
      _cb_draw_ps.ps_lighting_scale =
         _linear_rendering ? 1.0f : _cb_scene.near_scene_fade.z;
   }

   if (offset < (offsetof(cb::Scene, vs_view_positionWS) / sizeof(glm::vec4))) {
      _cb_draw_ps_dirty = true;
      _cb_draw_ps.ps_view_positionWS = _cb_scene.vs_view_positionWS;
   }
}

void Shader_patch::set_constants(const cb::Draw_tag, const UINT offset,
                                 const gsl::span<const std::array<float, 4>> constants) noexcept
{
   _cb_draw_dirty = true;

   std::memcpy(bit_cast<std::byte*>(&_cb_draw) +
                  (offset * sizeof(std::array<float, 4>)),
               constants.data(), constants.size_bytes());
}

void Shader_patch::set_constants(const cb::Fixedfunction_tag,
                                 const glm::vec4 texture_factor) noexcept
{
   const auto& rt = _game_rendertargets[static_cast<int>(_current_game_rendertarget)];

   cb::Fixedfunction constants;

   constants.texture_factor = texture_factor;
   constants.inv_resolution = {1.0f / rt.width, 1.0f / rt.height};

   update_dynamic_buffer(*_device_context, *_cb_fixedfunction_buffer, constants);
}

void Shader_patch::set_constants(const cb::Skin_tag, const UINT offset,
                                 const gsl::span<const std::array<float, 4>> constants) noexcept
{
   _cb_skin_dirty = true;

   std::memcpy(bit_cast<std::byte*>(&_cb_skin) +
                  (offset * sizeof(std::array<float, 4>)),
               constants.data(), constants.size_bytes());
}

void Shader_patch::set_constants(const cb::Draw_ps_tag, const UINT offset,
                                 const gsl::span<const std::array<float, 4>> constants) noexcept
{
   _cb_draw_ps_dirty = true;

   std::memcpy(bit_cast<std::byte*>(&_cb_draw_ps) +
                  (offset * sizeof(std::array<float, 4>)),
               constants.data(), constants.size_bytes());
}

void Shader_patch::draw(const D3D11_PRIMITIVE_TOPOLOGY topology,
                        const UINT vertex_count, const UINT start_vertex) noexcept
{
   update_dirty_state(topology);

   if (_discard_draw_calls) return;

   _device_context->Draw(vertex_count, start_vertex);
}

void Shader_patch::draw_indexed(const D3D11_PRIMITIVE_TOPOLOGY topology,
                                const UINT index_count, const UINT start_index,
                                const UINT start_vertex) noexcept
{
   update_dirty_state(topology);

   if (_discard_draw_calls) return;

   _device_context->DrawIndexed(index_count, start_index, start_vertex);
}

void Shader_patch::begin_query(ID3D11Query& query) noexcept
{
   _device_context->Begin(&query);
}

void Shader_patch::end_query(ID3D11Query& query) noexcept
{
   _device_context->End(&query);
}

auto Shader_patch::get_query_data(ID3D11Query& query, const bool flush,
                                  gsl::span<std::byte> data) noexcept -> Query_result
{
   const auto result =
      _device_context->GetData(&query, data.data(), data.size(),
                               flush ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH);

   if (result == S_OK) return Query_result::success;
   if (result == S_FALSE) return Query_result::notready;

   return Query_result::error;
}

auto Shader_patch::current_rt_format() const noexcept -> DXGI_FORMAT
{
   if (_effects.enabled()) return _effects.rt_format();

   return Swapchain::format;
}

auto Shader_patch::current_depthstencil() const noexcept -> ID3D11DepthStencilView*
{
   if (_current_depthstencil_id == Game_depthstencil::nearscene)
      return _use_interface_depthstencil ? _interface_depthstencil.dsv.get()
                                         : _nearscene_depthstencil.dsv.get();
   else if (_current_depthstencil_id == Game_depthstencil::farscene)
      return _farscene_depthstencil.dsv.get();
   else if (_current_depthstencil_id == Game_depthstencil::reflectionscene)
      return _reflectionscene_depthstencil.dsv.get();

   return nullptr;
}

void Shader_patch::bind_static_resources() noexcept
{
   auto* const empty_vertex_buffer = _empty_vertex_buffer.get();
   const UINT empty_vertex_stride = 64;
   const UINT empty_vertex_offset = 0;
   _device_context->IASetVertexBuffers(1, Shader_input_layouts::throwaway_input_slot,
                                       &empty_vertex_buffer, &empty_vertex_stride,
                                       &empty_vertex_offset);

   auto* cb_scene = _cb_scene_buffer.get();
   const auto vs_constant_buffers =
      std::array{cb_scene, _cb_draw_buffer.get(), _cb_fixedfunction_buffer.get()};

   _device_context->VSSetConstantBuffers(0, vs_constant_buffers.size(),
                                         vs_constant_buffers.data());

   auto* const cb_skin_buffer = _cb_skin_buffer_srv.get();
   _device_context->VSSetShaderResources(0, 1, &cb_skin_buffer);

   _device_context->HSSetConstantBuffers(0, 1, &cb_scene);
   _device_context->DSSetConstantBuffers(0, 1, &cb_scene);
   _device_context->GSSetConstantBuffers(0, 1, &cb_scene);

   const auto ps_constant_buffers =
      std::array{_cb_draw_ps_buffer.get(), _cb_draw_buffer.get()};

   _device_context->PSSetConstantBuffers(0, ps_constant_buffers.size(),
                                         ps_constant_buffers.data());

   const auto ps_samplers = std::array{_sampler_states.aniso_wrap_sampler.get(),
                                       _sampler_states.linear_clamp_sampler.get(),
                                       _sampler_states.linear_wrap_sampler.get(),
                                       _sampler_states.linear_clamp_sampler.get()};

   _device_context->PSSetSamplers(0, ps_samplers.size(), ps_samplers.data());
}

void Shader_patch::game_rendertype_changed() noexcept
{
   if (_on_rendertype_changed) _on_rendertype_changed();

   if (_shader_rendertype == Rendertype::zprepass) {
      _om_targets_dirty = true;
   }
   else if (_shader_rendertype == Rendertype::shadowquad) {
      _om_targets_dirty = true;
      _discard_draw_calls = true;

      _on_stretch_rendertarget = [this](Game_rendertarget&, const RECT*,
                                        Game_rendertarget& dest, const RECT*) noexcept {
         _on_stretch_rendertarget = nullptr;

         if (dest.type != Game_rt_type::shadow) {
            dest = Game_rendertarget{*_device,
                                     shadow_texture_format,
                                     _swapchain.width(),
                                     _swapchain.height(),
                                     1,
                                     Game_rt_type::shadow};
         }

         const bool multisampled = _rt_sample_count > 1;
         auto* const rtv = multisampled ? _shadow_msaa_rt.rtv.get() : dest.rtv.get();
         auto* const dsv = current_depthstencil();

         _device_context
            ->ClearRenderTargetView(rtv, std::array{1.0f, 1.0f, 1.0f, 1.0f}.data());
         _device_context->OMSetRenderTargets(1, &rtv, dsv);
         _device_context->Draw(3, 0);

         if (multisampled)
            _device_context->ResolveSubresource(dest.texture.get(), 0,
                                                _shadow_msaa_rt.texture.get(),
                                                0, shadow_texture_format);

         // Skip the next draw call.
         _on_rendertype_changed = [this]() noexcept {
            _on_rendertype_changed = [this]() noexcept {
               _discard_draw_calls = false;
               _on_rendertype_changed = nullptr;
            };
         };
      };
   }
   else if (_shader_rendertype == Rendertype::shield) {
      resolve_refraction_texture();

      auto normalmap_srv = _texture_database.get("$water");

      auto* const srv = normalmap_srv.get();

      _device_context->PSSetShaderResources(custom_tex_begin, 1, &srv);
   }
   else if (_shader_rendertype == Rendertype::refraction) {
      resolve_refraction_texture();

      auto* const srv = _refraction_rt.srv.get();

      _device_context->PSSetShaderResources(custom_tex_begin, 1, &srv);
   }
   else if (_shader_rendertype == Rendertype::water) {
      resolve_refraction_texture();

      auto water_srv = _texture_database.get("$water");

      const std::array srvs{water_srv.get(), _refraction_rt.srv.get()};

      _device_context->PSSetShaderResources(custom_tex_begin, srvs.size(),
                                            srvs.data());
   }
   else if (_shader_rendertype == Rendertype::hdr && !_effects_active &&
            _game_shader->shader_name == "glow threshold"sv) {
      if (_game_rendertargets[0].sample_count > 1) {
         Com_ptr<ID3D11Resource> dest;
         _game_textures[0].srv->GetResource(dest.clear_and_assign());

         _device_context->ResolveSubresource(dest.get(), 0,
                                             _game_rendertargets[0].texture.get(),
                                             0, _game_rendertargets[0].format);
      }
      else {
         _ps_textures_dirty = true;
         _game_textures[0] = {_game_rendertargets[0].srv, _game_rendertargets[0].srv};
      }
   }

   if (_effects_active) {
      if (_shader_rendertype == Rendertype::hdr) {
         _om_targets_dirty = true;
         _use_interface_depthstencil = true;
         _game_rendertargets[0] = _swapchain.game_rendertarget();

         {
            Context_state_guard<state_flags::all> state_guard{*_device_context};

            _effects.postprocess.apply(*_device_context, _rendertarget_allocator,
                                       _effects.profiler, _texture_database,
                                       _patch_backbuffer.postprocess_input(),
                                       _swapchain.postprocess_output());
         }

         set_linear_rendering(false);
         _discard_draw_calls = true;

         _on_rendertype_changed = [this]() noexcept {
            _discard_draw_calls = false;
            _on_rendertype_changed = nullptr;
         };
      }
   }
}

void Shader_patch::update_dirty_state(const D3D11_PRIMITIVE_TOPOLOGY draw_primitive_topology) noexcept
{
   if (std::exchange(_shader_rendertype_changed, false))
      game_rendertype_changed();

   if (std::exchange(_shader_dirty, false)) {
      update_shader();
   }

   const auto primitive_topology = _use_patch_material_topology
                                      ? _patch_material_topology
                                      : draw_primitive_topology;

   if (std::exchange(_primitive_topology, primitive_topology) != primitive_topology)
      _device_context->IASetPrimitiveTopology(_primitive_topology);

   if (std::exchange(_ps_textures_dirty, false)) {
      if (_linear_rendering) {
         const auto& srgb = _game_shader->srgb_state;
         _device_context->PSSetShaderResources(
            0, 4,
            std::array{srgb[0] ? _game_textures[0].srgb_srv.get()
                               : _game_textures[0].srv.get(),
                       srgb[1] ? _game_textures[1].srgb_srv.get()
                               : _game_textures[1].srv.get(),
                       srgb[2] ? _game_textures[2].srgb_srv.get()
                               : _game_textures[2].srv.get(),
                       srgb[3] ? _game_textures[3].srgb_srv.get()
                               : _game_textures[3].srv.get()}
               .data());
      }
      else {
         _device_context->PSSetShaderResources(
            0, 4,
            std::array{_game_textures[0].srv.get(), _game_textures[1].srv.get(),
                       _game_textures[2].srv.get(), _game_textures[3].srv.get()}
               .data());
      }
   }

   if (std::exchange(_om_targets_dirty, false)) {
      const auto& rt =
         _game_rendertargets[static_cast<int>(_current_game_rendertarget)];
      auto* const rtv = rt.rtv.get();

      _device_context->OMSetRenderTargets(1, &rtv, current_depthstencil());

      _cb_draw_ps_dirty = true;
      _cb_draw_ps.rt_resolution = {rt.width, rt.height, 1.0f / rt.width,
                                   1.0f / rt.height};

      set_viewport(0, 0, rt.width, rt.height);
   }

   if (std::exchange(_cb_scene_dirty, false)) {
      update_dynamic_buffer(*_device_context, *_cb_scene_buffer, _cb_scene);
   }

   if (std::exchange(_cb_draw_dirty, false)) {
      update_dynamic_buffer(*_device_context, *_cb_draw_buffer, _cb_draw);
   }

   if (std::exchange(_cb_skin_dirty, false)) {
      update_dynamic_buffer(*_device_context, *_cb_skin_buffer, _cb_skin);
   }

   if (std::exchange(_cb_draw_ps_dirty, false)) {
      update_dynamic_buffer(*_device_context, *_cb_draw_ps_buffer, _cb_draw_ps);
   }
}

void Shader_patch::update_shader() noexcept
{
   if (_patch_material) {
      auto vs_flags = _game_shader->vertex_shader_flags;

      if (_game_input_layout.compressed)
         vs_flags |= Vertex_shader_flags::compressed;

      _use_patch_material_topology = (_patch_material->tessellation &&
                                      user_config.graphics.enable_tessellation);
      if (_use_patch_material_topology)
         _patch_material_topology = _patch_material->tessellation_primitive_topology;

      if (_shader_rendertype == _patch_material->overridden_rendertype) {
         _patch_material->shader->update(*_device_context, _input_layout_descriptions,
                                         _game_input_layout.layout_index,
                                         _game_shader->shader_name, vs_flags,
                                         _game_shader->pixel_shader_flags);
         return;
      }
      else if (_shader_rendertype == Rendertype::zprepass &&
               _patch_material->tessellation &&
               user_config.graphics.enable_tessellation) {
         vs_flags |= Vertex_shader_flags::normal;

         _patch_material->shader->update_for_zprepass(*_device_context,
                                                      _input_layout_descriptions,
                                                      _game_input_layout.layout_index,
                                                      _game_shader->shader_name,
                                                      vs_flags);
         return;
      }
   }

   _use_patch_material_topology = false;

   if (_game_input_layout.compressed) {
      auto& input_layout =
         _game_shader->input_layouts_compressed.get(*_device, _input_layout_descriptions,
                                                    _game_input_layout.layout_index);

      _device_context->IASetInputLayout(&input_layout);
      _device_context->VSSetShader(_game_shader->vs_compressed.get(), nullptr, 0);
   }
   else {
      auto& input_layout =
         _game_shader->input_layouts.get(*_device, _input_layout_descriptions,
                                         _game_input_layout.layout_index);

      _device_context->IASetInputLayout(&input_layout);
      _device_context->VSSetShader(_game_shader->vs.get(), nullptr, 0);
   }

   _device_context->PSSetShader(_game_shader->ps.get(), nullptr, 0);
   _device_context->HSSetShader(nullptr, nullptr, 0);
   _device_context->DSSetShader(nullptr, nullptr, 0);
   _device_context->GSSetShader(nullptr, nullptr, 0);
}

void Shader_patch::update_frame_state() noexcept
{
   const auto time = std::chrono::steady_clock{}.now().time_since_epoch();

   _cb_scene_dirty = true;
   _cb_scene.time = std::chrono::duration<float>{(time % 30s)}.count();

   _refraction_farscene_texture_resolve = false;
   _refraction_nearscene_texture_resolve = false;
   _use_interface_depthstencil = false;
}

void Shader_patch::update_imgui() noexcept
{
   if (_imgui_enabled) {
      user_config.show_imgui();
      _effects.show_imgui(_window);
   }

   if (_imgui_enabled || _effects.profiler.enabled) {
      ImGui::Render();
      ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
   }
   else {
      ImGui::EndFrame();
   }

   ImGui_ImplDX11_NewFrame();
   ImGui_ImplWin32_NewFrame();
   ImGui::NewFrame();
}

void Shader_patch::update_effects() noexcept
{
   if (std::exchange(_effects_active, _effects.enabled()) != _effects.enabled()) {
      _om_targets_dirty = true;
      _use_interface_depthstencil = false;

      if (_effects.enabled()) {
         _patch_backbuffer = {*_device, _effects.rt_format(), _swapchain.width(),
                              _swapchain.height(), _rt_sample_count};
         _interface_depthstencil = {*_device, _swapchain.width(),
                                    _swapchain.height(), 1};
      }
      else {
         _rendertarget_allocator.reset();
         _patch_backbuffer =
            (_rt_sample_count > 1)
               ? Effects_backbuffer{*_device, Swapchain::format, _swapchain.width(),
                                    _swapchain.height(), _rt_sample_count}
               : Effects_backbuffer{};
         _interface_depthstencil = {};
         _current_effects_rt_format = DXGI_FORMAT_UNKNOWN;

         for (auto i = 1; i < _game_rendertargets.size(); ++i) {
            _game_rendertargets[i] =
               Game_rendertarget{*_device, Swapchain::format,
                                 _game_rendertargets[i].width,
                                 _game_rendertargets[i].height};
         }
      }
   }

   update_rendertarget_formats();
   set_linear_rendering(_effects.enabled() && _effects.config().hdr_rendering);
}

void Shader_patch::update_rendertarget_formats() noexcept
{
   if (!_effects.enabled()) return;
   if (std::exchange(_current_effects_rt_format, _effects.rt_format()) ==
       _effects.rt_format())
      return;

   _om_targets_dirty = true;
   _rendertarget_allocator.reset();

   if (_patch_backbuffer.format != _effects.rt_format()) {
      _patch_backbuffer =
         Effects_backbuffer{*_device, _effects.rt_format(), _swapchain.width(),
                            _swapchain.height(), _rt_sample_count};
   }

   for (auto i = 1; i < _game_rendertargets.size(); ++i) {
      if (!_game_rendertargets[i] || _game_rendertargets[i].type != Game_rt_type::untyped)
         continue;

      _game_rendertargets[i] = Game_rendertarget{*_device, _effects.rt_format(),
                                                 _game_rendertargets[i].width,
                                                 _game_rendertargets[i].height};
   }
}

void Shader_patch::update_aa_rendertargets() noexcept
{
   const auto new_sample_count =
      to_sample_count(user_config.graphics.antialiasing_method);

   if (std::exchange(_rt_sample_count, new_sample_count) == new_sample_count)
      return;

   _om_targets_dirty = true;

   _nearscene_depthstencil = {*_device, _swapchain.width(), _swapchain.height(),
                              _rt_sample_count};

   if (_rt_sample_count > 1) {
      _patch_backbuffer =
         Effects_backbuffer{*_device,
                            _effects_active ? _effects.rt_format() : Swapchain::format,
                            _swapchain.width(), _swapchain.height(), _rt_sample_count};
      _shadow_msaa_rt = {*_device,           shadow_texture_format,
                         _swapchain.width(), _swapchain.height(),
                         _rt_sample_count,   Game_rt_type::shadow};
      _game_rendertargets[0] = _patch_backbuffer.game_rendertarget();
   }
   else {
      _patch_backbuffer =
         _effects_active
            ? Effects_backbuffer{*_device, _effects.rt_format(),
                                 _swapchain.width(), _swapchain.height(), 1}
            : Effects_backbuffer{};
      _shadow_msaa_rt = {};
      _game_rendertargets[0] = _swapchain.game_rendertarget();
   }
}

void Shader_patch::update_samplers() noexcept
{
   _sampler_states.update_ansio_samplers(*_device);

   auto* const sampler = _sampler_states.aniso_wrap_sampler.get();

   _device_context->PSSetSamplers(0, 1, &sampler);
}

void Shader_patch::set_linear_rendering(bool linear_rendering) noexcept
{
   if (linear_rendering) {
      _cb_scene.vertex_color_srgb = true;
      _cb_draw_ps.stock_tonemap_state = 1.0f;
      _cb_draw_ps.ps_lighting_scale = 1.0f;
   }
   else {
      _cb_scene.vertex_color_srgb = false;
      _cb_draw_ps.stock_tonemap_state = 0.0f;
      _cb_draw_ps.ps_lighting_scale = 0.5f;
   }

   _cb_scene_dirty = true;
   _cb_draw_ps_dirty = true;
   _ps_textures_dirty = true;
   _linear_rendering = linear_rendering;
}

void Shader_patch::resolve_refraction_texture() noexcept
{
   if (_current_depthstencil_id == Game_depthstencil::farscene) {
      if (std::exchange(_refraction_farscene_texture_resolve, true)) return;
   }
   else {
      if (std::exchange(_refraction_nearscene_texture_resolve, true)) return;
   }

   _om_targets_dirty = true;

   std::array<ID3D11ShaderResourceView*, 2> null_srvs{};
   _device_context->PSSetShaderResources(custom_tex_begin, null_srvs.size(),
                                         null_srvs.data());

   ID3D11RenderTargetView* null_rtv = nullptr;
   _device_context->OMSetRenderTargets(0, &null_rtv, nullptr);

   const auto& src_rt =
      _game_rendertargets[static_cast<int>(_current_game_rendertarget)];

   const D3D11_BOX src_box{0, 0, 0, src_rt.width, src_rt.height, 1};
   const D3D11_BOX dest_box{0,
                            0,
                            0,
                            _refraction_rt.width,
                            _refraction_rt.height,
                            1};

   Context_state_guard<state_flags::ia_all | state_flags::vs_all | state_flags::hs_shader |
                       state_flags::ds_shader | state_flags::gs_shader |
                       state_flags::rs_all | state_flags::ps_all | state_flags::om_all>
      state_guard{*_device_context};

   _image_stretcher.stretch(*_device_context, src_box, src_rt, dest_box, _refraction_rt);
}

void Shader_patch::patch_backbuffer_resolve() noexcept
{
   if (_game_rendertargets[0].format == Swapchain::format) {
      if (_rt_sample_count > 1) {
         _device_context->ResolveSubresource(_swapchain.texture(), 0,
                                             _game_rendertargets[0].texture.get(),
                                             0, _game_rendertargets[0].format);
      }
      else {
         _device_context->CopyResource(_swapchain.texture(),
                                       _game_rendertargets[0].texture.get());
      }
   }
   else {
      Context_state_guard<state_flags::ia_all | state_flags::vs_all | state_flags::hs_shader |
                          state_flags::ds_shader | state_flags::gs_shader |
                          state_flags::rs_all | state_flags::ps_all | state_flags::om_all>
         state_guard{*_device_context};

      _late_backbuffer_resolver.resolve(*_device_context,
                                        _game_rendertargets[0], _swapchain);
   }
}
}
