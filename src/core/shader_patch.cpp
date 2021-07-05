
#include "shader_patch.hpp"
#include "../bf2_log_monitor.hpp"
#include "../input_hooker.hpp"
#include "../logger.hpp"
#include "../material/editor.hpp"
#include "../user_config.hpp"
#include "basic_builtin_textures.hpp"
#include "patch_material_io.hpp"
#include "patch_texture_io.hpp"
#include "screenshot.hpp"
#include "utility.hpp"

#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"

#include <chrono>
#include <cmath>

#include <comdef.h>

using namespace std::literals;

namespace sp::core {

constexpr auto projtex_cube_slot = 4;
constexpr auto shadow_texture_format = DXGI_FORMAT_A8_UNORM;
constexpr auto flares_texture_format = DXGI_FORMAT_A8_UNORM;
constexpr auto screenshots_folder = L"ScreenShots/";
constexpr auto refraction_texture_name = "_SP_BUILTIN_refraction"sv;
constexpr auto depth_texture_name = "_SP_BUILTIN_depth"sv;

namespace {

constexpr std::array supported_feature_levels{D3D_FEATURE_LEVEL_12_1,
                                              D3D_FEATURE_LEVEL_12_0,
                                              D3D_FEATURE_LEVEL_11_1,
                                              D3D_FEATURE_LEVEL_11_0};

auto create_device(IDXGIAdapter4& adapater) noexcept -> Com_ptr<ID3D11Device5>
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

   Com_ptr<ID3D11Device5> device5;

   if (const auto result = device->QueryInterface(device5.clear_and_assign());
       FAILED(result)) {
      log_and_terminate(
         "Failed to create Direct3D 11.4 device! This likely "
         "means you're not running Windows 10 or it is not up to date.");
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
            infoqueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);

            std::array hide{D3D11_MESSAGE_ID_DEVICE_DRAW_VIEW_DIMENSION_MISMATCH};

            D3D11_INFO_QUEUE_FILTER filter{};
            filter.DenyList.NumIDs = hide.size();
            filter.DenyList.pIDList = hide.data();
            infoqueue->AddStorageFilterEntries(&filter);
         }
      }
   }

   return device5;
}
}

Shader_patch::Shader_patch(IDXGIAdapter4& adapter, const HWND window,
                           const UINT width, const UINT height) noexcept
   : _device{create_device(adapter)},
     _swapchain{_device, adapter, window, width, height},
     _window{window},
     _nearscene_depthstencil{*_device, width, height,
                             to_sample_count(user_config.graphics.antialiasing_method)},
     _farscene_depthstencil{*_device, width, height, 1},
     _reflectionscene_depthstencil{*_device, 512, 256, 1},
     _bf2_log_monitor{user_config.developer.monitor_bfront2_log
                         ? std::make_unique<BF2_log_monitor>()
                         : nullptr}
{
   _game_texture_pool.reserve(1024);
   _materials_pool.reserve(1024);

   add_builtin_textures(*_device, _shader_resource_database);
   bind_static_resources();
   update_rendertargets();
   update_refraction_target();

   _cb_scene.input_color_srgb = false;
   _cb_draw_ps.additive_blending = false;
   _cb_draw_ps.limit_normal_shader_bright_lights = true;
   _cb_draw_ps.cube_projtex = false;
   _cb_draw_ps.input_color_srgb = false;

   install_window_hooks(window);
   install_dinput_hooks();
   set_input_hotkey(user_config.developer.toggle_key);
   set_input_hotkey_func([this]() noexcept {
      if (!std::exchange(_imgui_enabled, !_imgui_enabled))
         set_input_mode(Input_mode::imgui);
      else
         set_input_mode(Input_mode::normal);
   });
   set_input_screenshot_func([this]() noexcept { _screenshot_requested = true; });

   ImGui::CreateContext();
   ImGui::GetIO().MouseDrawCursor = true;
   ImGui::GetIO().ConfigFlags |=
      (ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad |
       ImGuiConfigFlags_NoMouseCursorChange);
   ImGui_ImplWin32_Init(window);
   ImGui_ImplDX11_Init(_device.get(), _device_context.get());
   ImGui_ImplDX11_NewFrame();
   ImGui_ImplWin32_NewFrame();
   ImGui::NewFrame();
}

Shader_patch::~Shader_patch() = default;

void Shader_patch::reset(const UINT width, const UINT height) noexcept
{
   _device_context->ClearState();
   _game_rendertargets.clear();
   _effects.cmaa2.clear_resources();
   _effects.postprocess.color_grading_regions({});
   _oit_provider.clear_resources();

   _swapchain.resize(width, height);
   _game_rendertargets.emplace_back() = _swapchain.game_rendertarget();
   _nearscene_depthstencil = {*_device, width, height, _rt_sample_count};
   _farscene_depthstencil = {*_device, width, height, 1};
   _refraction_rt = {};
   _farscene_refraction_rt = {};
   _current_game_rendertarget = _game_backbuffer_index;
   _primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
   _game_input_layout = {};
   _game_shader = nullptr;
   _game_textures = {};
   _game_stencil_ref = 0xff;
   _game_index_buffer_offset = 0;
   _game_vertex_buffer_offset = 0;
   _game_vertex_buffer_stride = 0;
   _game_index_buffer = nullptr;
   _game_vertex_buffer = nullptr;
   _game_blend_state = nullptr;
   _game_rs_state = nullptr;
   _game_depthstencil_state = nullptr;
   _previous_shader_rendertype = Rendertype::invalid;
   _shader_rendertype = Rendertype::invalid;
   _aa_method = Antialiasing_method::none;
   _current_rt_format = Swapchain::format;

   update_rendertargets();
   update_refraction_target();
   update_material_resources();
   restore_all_game_state();
}

void Shader_patch::set_text_dpi(const std::uint32_t dpi) noexcept
{
   _font_atlas_builder.set_dpi(dpi);
}

void Shader_patch::present() noexcept
{
   _effects.profiler.end_frame(*_device_context);
   _game_postprocessing.end_frame();

   if (_game_rendertargets[0].type != Game_rt_type::presentation)
      patch_backbuffer_resolve();

   update_imgui();

   if (std::exchange(_screenshot_requested, false))
      screenshot(*_device, *_device_context, _swapchain, screenshots_folder);

   _swapchain.present();
   _om_targets_dirty = true;

   _shader_database.cache_update();

   if (_font_atlas_builder.update_srv_database(_shader_resource_database)) {
      update_material_resources();
   }

   update_frame_state();
   update_effects();
   update_rendertargets();
   update_refraction_target();
   update_samplers();

   if (_patch_backbuffer) _game_rendertargets[0] = _patch_backbuffer;
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

auto Shader_patch::create_blend_state(const D3D11_BLEND_DESC1 blend_state_desc) noexcept
   -> Com_ptr<ID3D11BlendState1>
{
   Com_ptr<ID3D11BlendState1> state;

   _device->CreateBlendState1(&blend_state_desc, state.clear_and_assign());

   return state;
}

auto Shader_patch::create_game_rendertarget(const UINT width, const UINT height) noexcept
   -> Game_rendertarget_id
{
   const int index = _game_rendertargets.size();
   _game_rendertargets.emplace_back(*_device, _current_rt_format, width, height);

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
                                         const std::span<const Mapped_texture> data) noexcept
   -> Game_texture_handle
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

   return create_game_texture(texture, D3D11_SRV_DIMENSION_TEXTURE2D, format);
}

auto Shader_patch::create_game_texture3d(const UINT width, const UINT height,
                                         const UINT depth, const UINT mip_levels,
                                         const DXGI_FORMAT format,
                                         const std::span<const Mapped_texture> data) noexcept
   -> Game_texture_handle
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

   return create_game_texture(texture, D3D11_SRV_DIMENSION_TEXTURE3D, format);
}

auto Shader_patch::create_game_texture_cube(const UINT width, const UINT height,
                                            const UINT mip_levels,
                                            const DXGI_FORMAT format,
                                            const std::span<const Mapped_texture> data) noexcept
   -> Game_texture_handle
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

   return create_game_texture(texture, D3D11_SRV_DIMENSION_TEXTURECUBE, format);
}

void Shader_patch::convert_game_texture2d_to_dynamic(const Game_texture_handle game_texture_handle) noexcept
{
   auto* const game_texture = handle_to_ptr<Game_texture>(game_texture_handle);

   assert(game_texture->srv && game_texture->srgb_srv);

   Com_ptr<ID3D11Resource> source_resource;
   game_texture->srv->GetResource(source_resource.clear_and_assign());

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

      return;
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

         return;
      }
   }

   const auto srgb_view_format = DirectX::MakeSRGB(view_format);

   if (view_format == srgb_view_format) {
      *game_texture = Game_texture{srv, srv};
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

         return;
      }
   }

   *game_texture = Game_texture{srv, srgb_srv};
}

void Shader_patch::destroy_game_texture(const Game_texture_handle game_texture_handle) noexcept
{
   auto* const game_texture = handle_to_ptr<Game_texture>(game_texture_handle);

   std::erase_if(_game_texture_pool,
                 [=](auto& other) { return other.get() == game_texture; });
}

auto Shader_patch::create_patch_texture(const std::span<const std::byte> texture_data) noexcept
   -> Patch_texture_handle
{
   try {
      auto [srv, name] =
         load_patch_texture(ucfb::Reader_strict<"sptx"_mn>{texture_data}, *_device);

      auto* const raw_srv = srv.get();

      log(Log_level::info, "Loaded texture "sv, std::quoted(name));

      _shader_resource_database.insert(std::move(srv), name);

      return ptr_to_handle(raw_srv);
   }
   catch (std::exception& e) {
      log(Log_level::error, "Failed to create unknown texture! reason: "sv, e.what());

      return null_handle;
   }
}

void Shader_patch::destroy_patch_texture(const Patch_texture_handle texture_handle) noexcept
{
   auto* const texture = handle_to_ptr<ID3D11ShaderResourceView>(texture_handle);

   const auto [exists, name] = _shader_resource_database.reverse_lookup(texture);

   if (!exists) return; // Texture has already been replaced.

   log(Log_level::info, "Destroying texture "sv, std::quoted(name));

   _shader_resource_database.erase(texture);
}

auto Shader_patch::create_patch_material(const std::span<const std::byte> material_data) noexcept
   -> Material_handle
{
   try {
      const auto config =
         read_patch_material(ucfb::Reader_strict<"matl"_mn>{material_data});

      auto material = _materials_pool
                         .emplace_back(std::make_unique<material::Material>(
                            _material_factory.create_material(config)))
                         .get();

      log(Log_level::info, "Loaded material "sv, std::quoted(material->name));

      return ptr_to_handle(material);
   }
   catch (std::exception& e) {
      log(Log_level::error, "Failed to create unknown material! reason: "sv, e.what());

      return null_handle;
   }
}

void Shader_patch::destroy_patch_material(const Material_handle material_handle) noexcept
{
   auto* const material = handle_to_ptr<material::Material>(material_handle);

   if (_patch_material == material) set_patch_material(null_handle);

   for (auto it = _materials_pool.begin(); it != _materials_pool.end(); ++it) {
      if (it->get() != material) continue;

      log(Log_level::info, "Destroying material "sv, std::quoted(material->name));

      _materials_pool.erase(it);

      return;
   }

   log_and_terminate("Attempt to destroy nonexistant material!");
}

auto Shader_patch::create_patch_effects_config(const std::span<const std::byte> effects_config) noexcept
   -> Patch_effects_config_handle
{
   try {
      std::string config_str{reinterpret_cast<const char*>(effects_config.data()),
                             static_cast<std::size_t>(effects_config.size())};

      while (config_str.back() == '\0') config_str.pop_back();

      _effects.read_config(YAML::Load(config_str));
      _effects.enabled(true);

      return _current_effects_id += 1;
   }
   catch (std::exception& e) {
      log_and_terminate("Failed to load effects config! reason: "sv, e.what());
   }
}

void Shader_patch::destroy_patch_effects_config(const Patch_effects_config_handle effects_config) noexcept
{
   if (effects_config != _current_effects_id) return;

   _effects.enabled(false);
}

auto Shader_patch::create_game_input_layout(
   const std::span<const Input_layout_element> layout, const bool compressed,
   const bool particle_texture_scale) noexcept -> Game_input_layout
{
   return {_input_layout_descriptions.try_add(layout), compressed,
           particle_texture_scale};
}

auto Shader_patch::create_ia_buffer(const UINT size, const bool vertex_buffer,
                                    const bool index_buffer,
                                    const bool dynamic) noexcept -> Buffer_handle
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

   return ptr_to_handle(buffer.release());
}

void __declspec(noinline) Shader_patch::destroy_ia_buffer(const Buffer_handle buffer_handle) noexcept
{
   auto* const buffer = handle_to_ptr<ID3D11Buffer>(buffer_handle);

   buffer->Release();
}

void Shader_patch::load_colorgrading_regions(const std::span<const std::byte> regions_data) noexcept
{
   try {
      _effects.postprocess.color_grading_regions(sp::load_colorgrading_regions(
         ucfb::Reader_strict<"clrg"_mn>{regions_data}));
   }
   catch (std::exception& e) {
      log(Log_level::error, "Failed to load colorgrading regions! reason: "sv,
          e.what());
   }
}

void Shader_patch::update_ia_buffer(const Buffer_handle buffer_handle,
                                    const UINT offset, const UINT size,
                                    const std::byte* data) noexcept
{
   auto* buffer = handle_to_ptr<ID3D11Buffer>(buffer_handle);

   const D3D11_BOX box{offset, 0, 0, offset + size, 1, 1};

   _device_context->UpdateSubresource(buffer, 0, &box, data, 0, 0);
}

auto Shader_patch::map_ia_buffer(const Buffer_handle buffer_handle,
                                 const D3D11_MAP map_type) noexcept -> std::byte*
{
   auto* buffer = handle_to_ptr<ID3D11Buffer>(buffer_handle);

   D3D11_MAPPED_SUBRESOURCE mapped;

   _device_context->Map(buffer, 0, map_type, 0, &mapped);

   return static_cast<std::byte*>(mapped.pData);
}

void Shader_patch::unmap_ia_buffer(const Buffer_handle buffer_handle) noexcept
{
   auto* buffer = handle_to_ptr<ID3D11Buffer>(buffer_handle);

   _device_context->Unmap(buffer, 0);
}

auto Shader_patch::map_dynamic_texture(const Game_texture_handle game_texture_handle,
                                       const UINT mip_level,
                                       const D3D11_MAP map_type) noexcept -> Mapped_texture
{
   auto* const game_texture = handle_to_ptr<Game_texture>(game_texture_handle);

   Com_ptr<ID3D11Resource> resource;
   game_texture->srv->GetResource(resource.clear_and_assign());

   D3D11_MAPPED_SUBRESOURCE mapped;
   _device_context->Map(resource.get(), mip_level, map_type, 0, &mapped);

   return {mapped.RowPitch, mapped.DepthPitch, static_cast<std::byte*>(mapped.pData)};
}

void Shader_patch::unmap_dynamic_texture(const Game_texture_handle game_texture_handle,
                                         const UINT mip_level) noexcept
{
   auto* const game_texture = handle_to_ptr<Game_texture>(game_texture_handle);

   Com_ptr<ID3D11Resource> resource;
   game_texture->srv->GetResource(resource.clear_and_assign());

   _device_context->Unmap(resource.get(), mip_level);
}

void Shader_patch::stretch_rendertarget(const Game_rendertarget_id source,
                                        const RECT source_rect,
                                        const Game_rendertarget_id dest,
                                        const RECT dest_rect) noexcept
{
   auto& src_rt = _game_rendertargets[static_cast<int>(source)];
   auto& dest_rt = _game_rendertargets[static_cast<int>(dest)];

   if (_on_stretch_rendertarget)
      _on_stretch_rendertarget(src_rt, source_rect, dest_rt, dest_rect);

   const auto src_box = rect_to_box(source_rect);
   const auto dest_box = rect_to_box(dest_rect);

   // Skip any fullscreen resolve or copy operation, as these will be handled as special cases by the shaders that use them.
   if (glm::uvec2{src_rt.width, src_rt.height} ==
          glm::uvec2{dest_rt.width, dest_rt.height} &&
       glm::uvec2{src_rt.width, src_rt.height} ==
          glm::uvec2{src_box.right - src_box.left, src_box.bottom - src_box.top} &&
       glm::uvec2{dest_rt.width, dest_rt.height} ==
          glm::uvec2{dest_box.right - dest_box.left, dest_box.bottom - dest_box.top}) {
      return;
   }

   _image_stretcher.stretch(*_device_context, src_box, src_rt, dest_box, dest_rt);
   restore_all_game_state();
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
   auto* const dsv = current_depthstencil(false);

   if (!dsv) return;

   const UINT clear_flags = (clear_depth ? D3D11_CLEAR_DEPTH : 0) |
                            (clear_stencil ? D3D11_CLEAR_STENCIL : 0);

   _device_context->ClearDepthStencilView(dsv, clear_flags, depth, stencil);
}

void Shader_patch::set_index_buffer(const Buffer_handle buffer_handle,
                                    const UINT offset) noexcept
{
   auto* buffer = handle_to_ptr<ID3D11Buffer>(buffer_handle);

   _game_index_buffer = copy_raw_com_ptr(buffer);
   _game_index_buffer_offset = offset;
   _ia_index_buffer_dirty = true;
}

void Shader_patch::set_vertex_buffer(const Buffer_handle buffer_handle,
                                     const UINT offset, const UINT stride) noexcept
{
   auto* buffer = handle_to_ptr<ID3D11Buffer>(buffer_handle);

   _game_vertex_buffer = copy_raw_com_ptr(buffer);
   _game_vertex_buffer_offset = offset;
   _game_vertex_buffer_stride = stride;
   _ia_vertex_buffer_dirty = true;
}

void Shader_patch::set_input_layout(const Game_input_layout& input_layout) noexcept
{
   _game_input_layout = input_layout;
   _shader_dirty = true;

   if (std::uint32_t{input_layout.particle_texture_scale} !=
       _cb_scene.particle_texture_scale) {
      _cb_scene.particle_texture_scale = input_layout.particle_texture_scale;
      _cb_scene_dirty = true;
   }
}

void Shader_patch::set_game_shader(const std::uint32_t shader_index) noexcept
{
   _game_shader = &_game_shaders[shader_index];
   _shader_dirty = true;

   const auto rendertype = _game_shader->rendertype;

   _previous_shader_rendertype = _shader_rendertype;
   _shader_rendertype_changed =
      std::exchange(_shader_rendertype, rendertype) != rendertype;

   const std::uint32_t light_active = _game_shader->light_active;
   const std::uint32_t light_active_point_count = _game_shader->light_active_point_count;
   const std::uint32_t light_active_spot = _game_shader->light_active_spot;

   _cb_draw_ps_dirty |=
      ((std::exchange(_cb_draw_ps.light_active, light_active) != light_active) |
       (std::exchange(_cb_draw_ps.light_active_point_count,
                      light_active_point_count) != light_active_point_count) |
       (std::exchange(_cb_draw_ps.light_active_spot, light_active_spot) !=
        light_active_spot));
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

void Shader_patch::set_rasterizer_state(ID3D11RasterizerState& rasterizer_state) noexcept
{
   _game_rs_state = copy_raw_com_ptr(rasterizer_state);
   _rs_state_dirty = true;
}

void Shader_patch::set_depthstencil_state(ID3D11DepthStencilState& depthstencil_state,
                                          const UINT8 stencil_ref,
                                          const bool readonly) noexcept
{
   _game_depthstencil_state = copy_raw_com_ptr(depthstencil_state);
   _game_stencil_ref = stencil_ref;
   _om_depthstencil_state_dirty = true;
   _om_targets_dirty =
      _om_targets_dirty |
      (std::exchange(_om_depthstencil_readonly, readonly) != readonly);
}

void Shader_patch::set_blend_state(ID3D11BlendState1& blend_state,
                                   const bool additive_blending) noexcept
{
   _game_blend_state = copy_raw_com_ptr(blend_state);
   _om_blend_state_dirty = true;

   _cb_draw_ps.additive_blending = additive_blending;
   _cb_draw_ps_dirty = true;
}

void Shader_patch::set_fog_state(const bool enabled, const glm::vec4 color) noexcept
{
   _cb_draw_ps_dirty = true;
   _cb_draw_ps.fog_enabled = enabled;
   _cb_draw_ps.fog_color = color;
}

void Shader_patch::set_texture(const UINT slot,
                               const Game_texture_handle game_texture_handle) noexcept
{
   Expects(slot < 4);

   auto* const game_texture = handle_to_ptr<Game_texture>(game_texture_handle);

   _game_textures[slot] = game_texture ? *game_texture : Game_texture{};
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
      _device_context->PSSetSamplers(5, 1, &sampler);
   }
   else if (mode == Projtex_mode::wrap) {
      auto* const sampler = _sampler_states.linear_wrap_sampler.get();
      _device_context->PSSetSamplers(5, 1, &sampler);
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

void Shader_patch::set_projtex_cube(const Game_texture_handle game_texture_handle) noexcept
{
   auto* const game_texture = handle_to_ptr<Game_texture>(game_texture_handle);

   if (_lock_projtex_cube_slot) return;

   _game_textures[projtex_cube_slot] = game_texture ? *game_texture : Game_texture{};
   _ps_textures_dirty = true;
}

void Shader_patch::set_patch_material(const Material_handle material_handle) noexcept
{
   auto* const material = handle_to_ptr<material::Material>(material_handle);

   _patch_material = material;
   _shader_dirty = true;

   if (_patch_material) {
      _game_textures[0] = {_patch_material->fail_safe_game_texture,
                           _patch_material->fail_safe_game_texture};
      _ps_textures_dirty = true;

      _patch_material->bind_constant_buffers(*_device_context);
      _patch_material->bind_shader_resources(*_device_context);
   }
}

void Shader_patch::set_constants(const cb::Scene_tag, const UINT offset,
                                 const std::span<const std::array<float, 4>> constants) noexcept
{
   _cb_scene_dirty = true;

   std::memcpy(bit_cast<std::byte*>(&_cb_scene) +
                  (offset * sizeof(std::array<float, 4>)),
               constants.data(), constants.size_bytes());

   if (offset < (offsetof(cb::Scene, near_scene_fade_scale) / sizeof(glm::vec4))) {
      const float scale = _linear_rendering ? 1.0f : _cb_scene.vs_lighting_scale;

      _cb_draw_ps_dirty = true;
      _cb_scene.vs_lighting_scale = scale;
      _cb_draw_ps.ps_lighting_scale = scale;
   }

   if (offset < (offsetof(cb::Scene, vs_view_positionWS) / sizeof(glm::vec4))) {
      _cb_draw_ps_dirty = true;
      _cb_draw_ps.ps_view_positionWS = _cb_scene.vs_view_positionWS;
   }
}

void Shader_patch::set_constants(const cb::Draw_tag, const UINT offset,
                                 const std::span<const std::array<float, 4>> constants) noexcept
{
   _cb_draw_dirty = true;

   std::memcpy(bit_cast<std::byte*>(&_cb_draw) +
                  (offset * sizeof(std::array<float, 4>)),
               constants.data(), constants.size_bytes());
}

void Shader_patch::set_constants(const cb::Fixedfunction_tag,
                                 cb::Fixedfunction constants) noexcept
{
   update_dynamic_buffer(*_device_context, *_cb_fixedfunction_buffer, constants);

   _game_postprocessing.blur_factor(constants.texture_factor.a);
}

void Shader_patch::set_constants(const cb::Skin_tag, const UINT offset,
                                 const std::span<const std::array<float, 4>> constants) noexcept
{
   _cb_skin_dirty = true;

   std::memcpy(bit_cast<std::byte*>(&_cb_skin) +
                  (offset * sizeof(std::array<float, 4>)),
               constants.data(), constants.size_bytes());
}

void Shader_patch::set_constants(const cb::Draw_ps_tag, const UINT offset,
                                 const std::span<const std::array<float, 4>> constants) noexcept
{
   _cb_draw_ps_dirty = true;

   std::memcpy(bit_cast<std::byte*>(&_cb_draw_ps) +
                  (offset * sizeof(std::array<float, 4>)),
               constants.data(), constants.size_bytes());
}

void Shader_patch::set_informal_projection_matrix(const glm::mat4 matrix) noexcept
{
   _informal_projection_matrix = matrix;
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

void Shader_patch::force_shader_cache_save_to_disk() noexcept
{
   _shader_database.force_cache_save_to_disk();
}

auto Shader_patch::create_game_texture(Com_ptr<ID3D11Resource> texture,
                                       const D3D11_SRV_DIMENSION srv_dimension,
                                       const DXGI_FORMAT format) noexcept -> Game_texture_handle
{
   Com_ptr<ID3D11ShaderResourceView> srv;
   {
      const auto srv_desc = CD3D11_SHADER_RESOURCE_VIEW_DESC{srv_dimension, format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srv_desc,
                                               srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   Com_ptr<ID3D11ShaderResourceView> srgb_srv;

   const auto srgb_format = DirectX::MakeSRGB(format);

   if (format != srgb_format) {
      const auto srgb_srv_desc =
         CD3D11_SHADER_RESOURCE_VIEW_DESC{srv_dimension, srgb_format};

      if (const auto result =
             _device->CreateShaderResourceView(texture.get(), &srgb_srv_desc,
                                               srgb_srv.clear_and_assign());
          FAILED(result)) {
         log(Log_level::error, "Failed to create game texture SRGB SRV! reason: ",
             _com_error{result}.ErrorMessage());

         return {};
      }
   }

   auto& game_texture = _game_texture_pool.emplace_back(
      std::make_unique<Game_texture>(srv, srgb_srv ? srgb_srv : srv));

   return ptr_to_handle(game_texture.get());
}

auto Shader_patch::current_depthstencil(const bool readonly) const noexcept
   -> ID3D11DepthStencilView*
{
   const auto depthstencil = [this]() -> const Depthstencil* {
      if (_current_depthstencil_id == Game_depthstencil::nearscene)
         return &(_use_interface_depthstencil ? _farscene_depthstencil
                                              : _nearscene_depthstencil);
      else if (_current_depthstencil_id == Game_depthstencil::farscene)
         return &_farscene_depthstencil;
      else if (_current_depthstencil_id == Game_depthstencil::reflectionscene)
         return &_reflectionscene_depthstencil;
      else
         return nullptr;
   }();

   if (!depthstencil) return nullptr;

   return readonly ? depthstencil->dsv_readonly.get() : depthstencil->dsv.get();
}

void Shader_patch::bind_static_resources() noexcept
{
   auto* const empty_vertex_buffer = _empty_vertex_buffer.get();
   const UINT empty_vertex_stride = 0;
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

   const auto ps_constant_buffers =
      std::array{_cb_draw_ps_buffer.get(), _cb_draw_buffer.get()};

   _device_context->PSSetConstantBuffers(0, ps_constant_buffers.size(),
                                         ps_constant_buffers.data());

   const auto ps_samplers = std::array{_sampler_states.aniso_wrap_sampler.get(),
                                       _sampler_states.linear_clamp_sampler.get(),
                                       _sampler_states.linear_wrap_sampler.get(),
                                       _sampler_states.linear_mirror_sampler.get(),
                                       _sampler_states.text_sampler.get(),
                                       _sampler_states.linear_clamp_sampler.get()};

   _device_context->PSSetSamplers(0, ps_samplers.size(), ps_samplers.data());
}

void Shader_patch::game_rendertype_changed() noexcept
{
   if (_on_rendertype_changed) _on_rendertype_changed();

   if (_shader_rendertype == Rendertype::stencilshadow) {
      _on_rendertype_changed = [this]() noexcept {
         _discard_draw_calls = true;
         _on_rendertype_changed = nullptr;
      };
   }
   else if (_shader_rendertype == Rendertype::shadowquad) {
      _discard_draw_calls = false;

      const bool multisampled = _rt_sample_count > 1;
      auto* const shadow_rt = [&]() -> Game_rendertarget* {
         auto it =
            std::find_if(_game_rendertargets.begin(), _game_rendertargets.end(),
                         [&](const Game_rendertarget& rt) {
                            return (rt.type == Game_rt_type::shadow);
                         });

         return it != _game_rendertargets.end() ? &(*it) : nullptr;
      }();
      auto* const rt = multisampled ? &_shadow_msaa_rt : shadow_rt;

      auto backup_rt = _game_rendertargets[0];

      if (rt) {
         _device_context
            ->ClearRenderTargetView(rt->rtv.get(),
                                    std::array{1.0f, 1.0f, 1.0f, 1.0f}.data());

         _game_rendertargets[0] = *rt;
      }

      _on_stretch_rendertarget =
         [this, backup_rt = std::move(backup_rt)](Game_rendertarget&, const RECT,
                                                  Game_rendertarget& dest,
                                                  const RECT) noexcept {
            _game_rendertargets[0] = std::move(backup_rt);
            _on_stretch_rendertarget = nullptr;
            _om_targets_dirty = true;

            if (dest.type != Game_rt_type::shadow) {
               dest = Game_rendertarget{*_device,
                                        shadow_texture_format,
                                        _swapchain.width(),
                                        _swapchain.height(),
                                        1,
                                        Game_rt_type::shadow};
            }

            if (_rt_sample_count > 1)
               _device_context->ResolveSubresource(dest.texture.get(), 0,
                                                   _shadow_msaa_rt.texture.get(),
                                                   0, shadow_texture_format);
         };
   }
   else if (_shader_rendertype == Rendertype::refraction) {
      resolve_refraction_texture();

      const bool far_scene = (_current_depthstencil_id == Game_depthstencil::farscene);

      auto depth_srv = _shader_resource_database.at_if(
         (_current_depthstencil_id == Game_depthstencil::farscene) ? "$null_depth"sv
                                                                   : "$depth"sv);
      const auto& refraction_src = far_scene ? _farscene_refraction_rt : _refraction_rt;

      _game_textures[4] = {refraction_src.srv, refraction_src.srv};
      _game_textures[5] = {depth_srv, depth_srv};

      _om_targets_dirty = true;
      _om_depthstencil_force_readonly = true;
      _ps_textures_dirty = true;

      _on_rendertype_changed = [this]() noexcept {
         _game_textures[5] = {};

         _om_targets_dirty = true;
         _om_depthstencil_force_readonly = false;
         _ps_textures_dirty = true;
         _on_rendertype_changed = nullptr;
      };
   }
   else if (_shader_rendertype == Rendertype::water) {
      resolve_refraction_texture();

      const bool far_scene = (_current_depthstencil_id == Game_depthstencil::farscene);

      auto water_srv = _shader_resource_database.at_if("$water"sv);
      auto depth_srv =
         _shader_resource_database.at_if(far_scene ? "$null_depth"sv : "$depth"sv);

      const auto& refraction_src = far_scene ? _farscene_refraction_rt : _refraction_rt;

      _game_textures[4] = {water_srv, water_srv};
      _game_textures[5] = {refraction_src.srv, refraction_src.srv};
      _game_textures[6] = {depth_srv, depth_srv};

      _om_targets_dirty = true;
      _om_depthstencil_force_readonly = true;
      _ps_textures_dirty = true;
      _lock_projtex_cube_slot = true;

      _on_rendertype_changed = [this]() noexcept {
         _game_textures[6] = {};

         _om_targets_dirty = true;
         _om_depthstencil_force_readonly = false;
         _ps_textures_dirty = true;
         _lock_projtex_cube_slot = false;
         _on_rendertype_changed = nullptr;
      };
   }
   else if (_shader_rendertype == Rendertype::flare ||
            _shader_rendertype == Rendertype::sample) {
      auto* const srv = _game_textures[0].srv.get();
      for (auto& rt : _game_rendertargets) {
         if (rt.srv != srv) continue;

         if (rt.type != Game_rt_type::flares || rt.format != flares_texture_format) {
            rt = Game_rendertarget{*_device, flares_texture_format,
                                   rt.width, rt.height,
                                   1,        Game_rt_type::flares};

            _game_textures[0] = {rt.srv, rt.srv};
            _om_targets_dirty = true;
         }

         break;
      }
   }
   else if (_shader_rendertype == Rendertype::hdr && !_effects_active &&
            !user_config.graphics.enable_alternative_postprocessing) {
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
   else if (_shader_rendertype == Rendertype::skyfog) {
      _cb_scene.prev_near_scene_fade_scale = _cb_scene.near_scene_fade_scale;
      _cb_scene.prev_near_scene_fade_offset = _cb_scene.near_scene_fade_offset;

      if (use_depth_refraction_mask(_refraction_quality)) {
         resolve_msaa_depthstencil<true>();
      }

      _on_rendertype_changed = [&]() noexcept {
         resolve_refraction_texture();

         if (_oit_provider.enabled() ||
             (_effects_active && _effects.config().oit_requested)) {
            _oit_provider.prepare_resources(*_device_context,
                                            *_game_rendertargets[0].texture,
                                            *_game_rendertargets[0].rtv);
            _om_targets_dirty = true;
            _oit_active = true;

            _on_stretch_rendertarget = [&](Game_rendertarget&, const RECT,
                                           Game_rendertarget&,
                                           const RECT dest_rect) noexcept {
               if (dest_rect.left == 0 && dest_rect.top == 0 &&
                   dest_rect.bottom == 256 && dest_rect.right == 256)
                  return;

               resolve_oit();
            };
         }

         _on_rendertype_changed = nullptr;
      };
   }

   if (_oit_active) {
      switch (_shader_rendertype) {
      case Rendertype::flare:
      case Rendertype::hdr:
      case Rendertype::_interface:
      case Rendertype::prereflection:
      case Rendertype::sample:
      case Rendertype::skyfog:
      case Rendertype::fixedfunc_color_fill:
      case Rendertype::fixedfunc_damage_overlay:
      case Rendertype::fixedfunc_plain_texture:
      case Rendertype::fixedfunc_scene_blur:
      case Rendertype::fixedfunc_zoom_blur:
         resolve_oit();
      }
   }

   if (user_config.graphics.enable_alternative_postprocessing) {
      if (_shader_rendertype == Rendertype::filtercopy) {
         _discard_draw_calls = true;
         _on_rendertype_changed = [&]() noexcept {
            _discard_draw_calls = false;
            _on_rendertype_changed = nullptr;
         };
      }
      else if (_shader_rendertype == Rendertype::fixedfunc_scene_blur) {
         _discard_draw_calls = true;

         if (user_config.graphics.enable_scene_blur) {
            _game_postprocessing.apply_scene_blur(*_device_context,
                                                  _game_rendertargets[0],
                                                  _rendertarget_allocator);
            restore_all_game_state();
         }

         _on_rendertype_changed = [&]() noexcept {
            _discard_draw_calls = false;
            _on_rendertype_changed = nullptr;
         };
      }
      else if (_shader_rendertype == Rendertype::fixedfunc_zoom_blur &&
               _previous_shader_rendertype == Rendertype::filtercopy) {
         _discard_draw_calls = true;

         _game_postprocessing.apply_scope_blur(*_device_context,
                                               _game_rendertargets[0],
                                               _rendertarget_allocator);
         restore_all_game_state();

         _on_rendertype_changed = [&]() noexcept {
            _discard_draw_calls = false;
            _on_rendertype_changed = nullptr;
         };
      }
      else if (_shader_rendertype == Rendertype::hdr && !_effects_active) {
         _discard_draw_calls = true;

         const float weight_pow = 2.8f;
         const float intensity =
            std::pow(_cb_draw_ps.ps_custom_constants[1].w * 4.0f, 2.8f) *
            _cb_draw_ps.ps_custom_constants[1].x;

         _game_postprocessing.bloom_threshold(_cb_draw_ps.ps_custom_constants[0].w);
         _game_postprocessing.bloom_intensity(intensity);
         _game_postprocessing.apply_bloom(*_device_context, _game_rendertargets[0],
                                          _rendertarget_allocator);

         restore_all_game_state();

         _on_rendertype_changed = [&]() noexcept {
            _discard_draw_calls = false;
            _on_rendertype_changed = nullptr;
         };
      }
   }

   if (_effects_active) {
      if (_shader_rendertype == Rendertype::skyfog && _effects.ssao.enabled()) {
         resolve_msaa_depthstencil<false>();

         auto depth_srv = (_rt_sample_count != 1)
                             ? _farscene_depthstencil.srv.get()
                             : _nearscene_depthstencil.srv.get();

         _effects.ssao.apply(_effects.profiler, *_device_context, *depth_srv,
                             *_game_rendertargets[0].rtv, _informal_projection_matrix);

         restore_all_game_state();
      }
      else if (_shader_rendertype == Rendertype::hdr) {
         _use_interface_depthstencil = true;
         _game_rendertargets[0] = _swapchain.game_rendertarget();

         _device_context->ClearDepthStencilView(_farscene_depthstencil.dsv.get(),
                                                D3D11_CLEAR_DEPTH, 1.0f, 0x0);

         const effects::Postprocess_input postprocess_input{*_patch_backbuffer.srv,
                                                            _patch_backbuffer.format,
                                                            _patch_backbuffer.width,
                                                            _patch_backbuffer.height,
                                                            _patch_backbuffer.sample_count};

         if (_aa_method == Antialiasing_method::cmaa2) {
            _effects.postprocess.apply(*_device_context, _rendertarget_allocator,
                                       _effects.profiler, _shader_resource_database,
                                       _cb_scene.vs_view_positionWS, _effects.ffx_cas,
                                       _effects.cmaa2, postprocess_input,
                                       _swapchain.postprocess_output());
         }
         else {
            _effects.postprocess.apply(*_device_context, _rendertarget_allocator,
                                       _effects.profiler, _shader_resource_database,
                                       _cb_scene.vs_view_positionWS,
                                       _effects.ffx_cas, postprocess_input,
                                       _swapchain.postprocess_output());
         }

         set_linear_rendering(false);
         _discard_draw_calls = true;

         restore_all_game_state();

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

   if (std::exchange(_primitive_topology, draw_primitive_topology) != draw_primitive_topology)
      _device_context->IASetPrimitiveTopology(_primitive_topology);

   if (std::exchange(_ia_index_buffer_dirty, false)) {
      _device_context->IASetIndexBuffer(_game_index_buffer.get(), DXGI_FORMAT_R16_UINT,
                                        _game_index_buffer_offset);
   }

   if (std::exchange(_ia_vertex_buffer_dirty, false)) {
      auto* const buffer = _game_vertex_buffer.get();

      _device_context->IASetVertexBuffers(0, 1, &buffer, &_game_vertex_buffer_stride,
                                          &_game_vertex_buffer_offset);
   }

   if (std::exchange(_rs_state_dirty, false)) {
      _device_context->RSSetState(_game_rs_state.get());
   }

   if (std::exchange(_ps_textures_dirty, false)) {
      if (_linear_rendering) {
         const auto& srgb = _game_shader->srgb_state;
         const std::array srvs{srgb[0] ? _game_textures[0].srgb_srv.get()
                                       : _game_textures[0].srv.get(),
                               srgb[1] ? _game_textures[1].srgb_srv.get()
                                       : _game_textures[1].srv.get(),
                               srgb[2] ? _game_textures[2].srgb_srv.get()
                                       : _game_textures[2].srv.get(),
                               srgb[3] ? _game_textures[3].srgb_srv.get()
                                       : _game_textures[3].srv.get(),
                               _game_textures[4].srv.get(),
                               _game_textures[5].srv.get(),
                               _game_textures[6].srv.get()};

         _device_context->PSSetShaderResources(0, srvs.size(), srvs.data());
      }
      else {
         const std::array srvs{_game_textures[0].srv.get(),
                               _game_textures[1].srv.get(),
                               _game_textures[2].srv.get(),
                               _game_textures[3].srv.get(),
                               _game_textures[4].srv.get(),
                               _game_textures[5].srv.get(),
                               _game_textures[6].srv.get()};

         _device_context->PSSetShaderResources(0, srvs.size(), srvs.data());
      }
   }

   if (std::exchange(_om_targets_dirty, false)) {
      const auto& rt =
         _game_rendertargets[static_cast<int>(_current_game_rendertarget)];

      if (auto* const rtv = rt.rtv.get(); _oit_active) {
         const auto uavs = _oit_provider.uavs();

         _device_context->OMSetRenderTargetsAndUnorderedAccessViews(
            1, &rtv, current_depthstencil(), 1, uavs.size(), uavs.data(), nullptr);
      }
      else {
         _device_context->OMSetRenderTargets(1, &rtv, current_depthstencil());
      }

      _cb_draw_ps_dirty = true;
      _cb_draw_ps.rt_resolution = {rt.width, rt.height, 1.0f / rt.width,
                                   1.0f / rt.height};

      // Update viewport.
      {
         const auto viewport =
            CD3D11_VIEWPORT{0.0f, 0.0f, static_cast<float>(rt.width),
                            static_cast<float>(rt.height)};

         _device_context->RSSetViewports(1, &viewport);

         _cb_scene_dirty = true;
         _cb_scene.pixel_offset =
            glm::vec2{1.f, -1.f} / glm::vec2{viewport.Width, viewport.Height};
      }
   }

   if (std::exchange(_om_depthstencil_state_dirty, false)) {
      _device_context->OMSetDepthStencilState(_game_depthstencil_state.get(),
                                              _game_stencil_ref);
   }

   if (std::exchange(_om_blend_state_dirty, false)) {
      _device_context->OMSetBlendState(_game_blend_state.get(), nullptr, 0xffffffff);
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
         vs_flags |= shader::Vertex_shader_flags::compressed;

      if (_shader_rendertype == _patch_material->overridden_rendertype) {
         _patch_material->shader->update(*_device_context, _input_layout_descriptions,
                                         _game_input_layout.layout_index,
                                         _game_shader->shader_name, vs_flags,
                                         _oit_active);
         return;
      }
   }

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

   _device_context->PSSetShader(_oit_active ? _game_shader->ps_oit.get()
                                            : _game_shader->ps.get(),
                                nullptr, 0);
}

void Shader_patch::update_frame_state() noexcept
{
   using namespace std::chrono;
   const auto time = steady_clock{}.now().time_since_epoch();

   _cb_scene_dirty = true;
   _cb_scene.time = duration<float>{(time % 3600s)}.count();
   _cb_draw_ps.time_seconds = duration_cast<duration<float>>((time % 3600s)).count();

   _refraction_farscene_texture_resolve = false;
   _refraction_nearscene_texture_resolve = false;
   _msaa_depthstencil_resolve = false;
   _use_interface_depthstencil = false;
   _lock_projtex_cube_slot = false;
   _oit_active = false;
}

void Shader_patch::update_imgui() noexcept
{
   auto* const rtv = _swapchain.rtv();

   _device_context->OMSetRenderTargets(1, &rtv, nullptr);
   _om_targets_dirty = true;

   if (_imgui_enabled) {
      ImGui::ShowDemoWindow();
      user_config.show_imgui();
      _effects.show_imgui(_window);

      material::show_editor(_material_factory, _materials_pool);

      if (_bf2_log_monitor) _bf2_log_monitor->show_imgui(true);
   }

   if (_imgui_enabled) {
      ImGui::Render();
      ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
   }
   else if (_effects.profiler.enabled ||
            (_bf2_log_monitor && _bf2_log_monitor->overlay())) {
      if (_bf2_log_monitor) _bf2_log_monitor->show_imgui(false);

      auto& io = ImGui::GetIO();
      io.MouseDrawCursor = !io.MouseDrawCursor;

      ImGui::Render();
      ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

      io.MouseDrawCursor = !io.MouseDrawCursor;
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
      _use_interface_depthstencil = false;
   }

   update_rendertargets();
   set_linear_rendering(_effects.enabled() && _effects.config().hdr_rendering);
}

void Shader_patch::update_rendertargets() noexcept
{
   const auto new_format = [&] {
      if (_effects_active &&
          (_effects.config().hdr_rendering || _effects.config().fp_rendertargets))
         return DXGI_FORMAT_R16G16B16A16_FLOAT;
      else if (user_config.graphics.enable_16bit_color_rendering)
         return DXGI_FORMAT_R16G16B16A16_UNORM;
      else
         return Swapchain::format;
   }();

   const auto new_aa_method = [&] {
      if (user_config.graphics.antialiasing_method != Antialiasing_method::none &&
          (_oit_provider.enabled() ||
           (_effects.enabled() && _effects.config().oit_requested))) {
         return Antialiasing_method::cmaa2;
      }

      return user_config.graphics.antialiasing_method;
   }();

   if (const auto [old_format, old_aa_method] =
          std::pair{std::exchange(_current_rt_format, new_format),
                    std::exchange(_aa_method, new_aa_method)};
       (old_format == new_format) && (old_aa_method == new_aa_method)) {
      return;
   }

   _rt_sample_count = to_sample_count(_aa_method);
   _om_targets_dirty = true;

   _nearscene_depthstencil = {*_device, _swapchain.width(), _swapchain.height(),
                              _rt_sample_count};
   _patch_backbuffer = {};
   _shadow_msaa_rt = {};
   _backbuffer_cmaa2_views = {};

   if (_rt_sample_count > 1) {
      _patch_backbuffer =
         Game_rendertarget{*_device, _current_rt_format, _swapchain.width(),
                           _swapchain.height(), _rt_sample_count};
      _shadow_msaa_rt = {*_device,           shadow_texture_format,
                         _swapchain.width(), _swapchain.height(),
                         _rt_sample_count,   Game_rt_type::shadow};
   }
   else if (_effects_active || user_config.graphics.enable_16bit_color_rendering) {
      _patch_backbuffer =
         Game_rendertarget{*_device, _current_rt_format, _swapchain.width(),
                           _swapchain.height(), 1};
   }

   if (_aa_method == Antialiasing_method::cmaa2 &&
       !(user_config.graphics.enable_16bit_color_rendering || _effects_active)) {
      _patch_backbuffer = Game_rendertarget{*_device,
                                            DXGI_FORMAT_R8G8B8A8_TYPELESS,
                                            DXGI_FORMAT_R8G8B8A8_UNORM,
                                            _swapchain.width(),
                                            _swapchain.height(),
                                            D3D11_BIND_UNORDERED_ACCESS};
      _backbuffer_cmaa2_views =
         Backbuffer_cmaa2_views{*_device, *_patch_backbuffer.texture,
                                DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                DXGI_FORMAT_R8G8B8A8_UNORM};
   }

   _game_rendertargets[0] =
      _patch_backbuffer ? _patch_backbuffer : _swapchain.game_rendertarget();

   _rendertarget_allocator.reset();

   for (auto i = 1; i < _game_rendertargets.size(); ++i) {
      if (!_game_rendertargets[i] || _game_rendertargets[i].type != Game_rt_type::untyped)
         continue;

      _game_rendertargets[i] = Game_rendertarget{*_device, _current_rt_format,
                                                 _game_rendertargets[i].width,
                                                 _game_rendertargets[i].height};
   }
}

void Shader_patch::update_refraction_target() noexcept
{
   if (_refraction_rt.format == _current_rt_format &&
       (std::exchange(_refraction_quality, user_config.graphics.refraction_quality) ==
        user_config.graphics.refraction_quality)) {
      return;
   }

   const auto scale_factor = to_scale_factor(_refraction_quality);

   _refraction_rt = Game_rendertarget{*_device, _current_rt_format,
                                      _swapchain.width() / scale_factor,
                                      _swapchain.height() / scale_factor};
   _farscene_refraction_rt =
      Game_rendertarget{*_device, _current_rt_format, _swapchain.width() / 8,
                        _swapchain.height() / 8};

   _shader_resource_database.insert(_refraction_rt.srv, refraction_texture_name);

   if (use_depth_refraction_mask(_refraction_quality)) {
      _shader_resource_database.insert(_rt_sample_count != 1
                                          ? _farscene_depthstencil.srv
                                          : _nearscene_depthstencil.srv,
                                       depth_texture_name);
   }
   else {
      _shader_resource_database.insert(_shader_resource_database.at_if("$null_depth"sv),
                                       depth_texture_name);
   }

   update_material_resources();
}

void Shader_patch::update_samplers() noexcept
{
   _sampler_states.update_ansio_samplers(*_device);

   auto* const sampler = _sampler_states.aniso_wrap_sampler.get();

   _device_context->PSSetSamplers(0, 1, &sampler);
}

void Shader_patch::update_material_resources() noexcept
{
   for (auto& mat : _materials_pool) {
      mat->update_resources(_shader_resource_database);
   }
}

void Shader_patch::set_linear_rendering(bool linear_rendering) noexcept
{
   if (linear_rendering) {
      _cb_scene.input_color_srgb = true;
      _cb_draw_ps.input_color_srgb = true;
      _cb_scene.vs_lighting_scale = 1.0f;
      _cb_draw_ps.limit_normal_shader_bright_lights = false;
      _cb_draw_ps.ps_lighting_scale = 1.0f;
   }
   else {
      const bool limit_normal_shader_bright_lights =
         !(_effects.config().disable_light_brightness_rescaling |
           (user_config.graphics.disable_light_brightness_rescaling &
            !_effects.enabled()));

      _cb_scene.input_color_srgb = false;
      _cb_draw_ps.input_color_srgb = false;
      _cb_draw_ps.limit_normal_shader_bright_lights = limit_normal_shader_bright_lights;
      _cb_scene.vs_lighting_scale = 0.5f;
      _cb_draw_ps.ps_lighting_scale = 0.5f;
   }

   _cb_scene_dirty = true;
   _cb_draw_ps_dirty = true;
   _ps_textures_dirty = true;
   _linear_rendering = linear_rendering;
}

void Shader_patch::resolve_refraction_texture() noexcept
{
   const bool far_scene = _current_depthstencil_id == Game_depthstencil::farscene;

   if (far_scene) {
      if (std::exchange(_refraction_farscene_texture_resolve, true)) return;
   }
   else {
      if (std::exchange(_refraction_nearscene_texture_resolve, true)) return;
   }

   const auto& src_rt =
      far_scene ? _game_rendertargets[static_cast<int>(_current_game_rendertarget)]
                : _game_rendertargets[0];
   const auto& dest_rt = far_scene ? _farscene_refraction_rt : _refraction_rt;

   const D3D11_BOX src_box{0, 0, 0, src_rt.width, src_rt.height, 1};
   const D3D11_BOX dest_box{0, 0, 0, dest_rt.width, dest_rt.height, 1};

   _image_stretcher.stretch(*_device_context, src_box, src_rt, dest_box, dest_rt);

   restore_all_game_state();
}

void Shader_patch::resolve_oit() noexcept
{
   Expects(_oit_active);

   _oit_active = false;
   _om_targets_dirty = true;
   _on_stretch_rendertarget = nullptr;

   _oit_provider.resolve(*_device_context);
}

void Shader_patch::patch_backbuffer_resolve() noexcept
{
   if (DirectX::MakeTypeless(_game_rendertargets[0].format) ==
       DirectX::MakeTypeless(Swapchain::format)) {
      if (_rt_sample_count > 1) {
         _device_context->ResolveSubresource(_swapchain.texture(), 0,
                                             _game_rendertargets[0].texture.get(),
                                             0, _game_rendertargets[0].format);
      }
      else {
         if (_aa_method == Antialiasing_method::cmaa2) {
            _effects.cmaa2.apply(*_device_context, _effects.profiler,
                                 {.input = *_backbuffer_cmaa2_views.srv,
                                  .output = *_backbuffer_cmaa2_views.uav,
                                  .width = _game_rendertargets[0].width,
                                  .height = _game_rendertargets[0].height});
         }

         _device_context->CopyResource(_swapchain.texture(),
                                       _game_rendertargets[0].texture.get());
      }
   }
   else if (_aa_method == Antialiasing_method::cmaa2) {
      auto cmma_target = _rendertarget_allocator.allocate(
         {.format = DXGI_FORMAT_R8G8B8A8_TYPELESS,
          .width = _game_rendertargets[0].width,
          .height = _game_rendertargets[0].height,
          .bind_flags = effects::rendertarget_bind_srv_rtv_uav,
          .srv_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
          .rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM,
          .uav_format = DXGI_FORMAT_R8G8B8A8_UNORM});

      _late_backbuffer_resolver.resolve(*_device_context, _shader_resource_database,
                                        _effects_active && _effects.config().hdr_rendering,
                                        _game_rendertargets[0], cmma_target.rtv());

      _effects.cmaa2.apply(*_device_context, _effects.profiler,
                           {.input = cmma_target.srv(),
                            .output = cmma_target.uav(),
                            .width = _game_rendertargets[0].width,
                            .height = _game_rendertargets[0].height});

      _device_context->CopyResource(_swapchain.texture(), &cmma_target.texture());
   }
   else {
      _late_backbuffer_resolver.resolve(*_device_context, _shader_resource_database,
                                        _effects_active && _effects.config().hdr_rendering,
                                        _game_rendertargets[0], *_swapchain.rtv());
   }

   _game_rendertargets[0] = _swapchain.game_rendertarget();

   restore_all_game_state();
}

void Shader_patch::restore_all_game_state() noexcept
{
   _device_context->ClearState();

   _shader_dirty = true;
   _ia_index_buffer_dirty = true;
   _ia_vertex_buffer_dirty = true;
   _rs_state_dirty = true;
   _om_targets_dirty = true;
   _om_depthstencil_state_dirty = true;
   _om_blend_state_dirty = true;
   _ps_textures_dirty = true;
   _cb_scene_dirty = true;
   _cb_draw_dirty = true;
   _cb_skin_dirty = true;
   _cb_draw_ps_dirty = true;
   _primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

   bind_static_resources();

   if (_patch_material) {
      _patch_material->bind_constant_buffers(*_device_context);
      _patch_material->bind_shader_resources(*_device_context);
   }
}
}
