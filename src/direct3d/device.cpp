
#include "device.hpp"
#include "../effects/helpers.hpp"
#include "../imgui/imgui_impl_dx9.h"
#include "../input_hooker.hpp"
#include "../logger.hpp"
#include "../material_resource.hpp"
#include "../resource_handle.hpp"
#include "../shader_constants.hpp"
#include "../shader_loader.hpp"
#include "../window_helpers.hpp"
#include "copyable_finally.hpp"
#include "patch_texture.hpp"
#include "shader_metadata.hpp"
#include "volume_resource.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <limits>
#include <memory>
#include <string_view>
#include <vector>

#include <gsl/gsl>
#include <imgui.h>

using namespace std::literals;

namespace sp::direct3d {

namespace {

void set_active_light_constants(IDirect3DDevice9& device, Shader_flags flags) noexcept
{
   std::array<BOOL, 5> constants{};

   if ((flags & Shader_flags::light_dir) == Shader_flags::light_dir) {
      constants[0] = true;
   }

   if ((flags & Shader_flags::light_point) == Shader_flags::light_point) {
      std::fill_n(std::begin(constants) + 1, 1, true);
   }

   if ((flags & Shader_flags::light_point2) == Shader_flags::light_point2) {
      std::fill_n(std::begin(constants) + 1, 2, true);
   }

   if ((flags & Shader_flags::light_point4) == Shader_flags::light_point4) {
      std::fill_n(std::begin(constants) + 1, 3, true);
   }

   if ((flags & Shader_flags::light_spot) == Shader_flags::light_spot) {
      constants[4] = true;
   }

   device.SetPixelShaderConstantB(0, constants.data(), constants.size());
}

constexpr bool is_constant_being_set(int constant, int start, int count) noexcept
{
   return constant >= start && constant < (start + count);
}

bool is_blur_blend_state(const std::array<Texture_state_block, 8>& texture_states)
{
   if (texture_states[1].at(D3DTSS_COLOROP) != D3DTOP_DISABLE) return false;
   if (texture_states[1].at(D3DTSS_ALPHAOP) != D3DTOP_DISABLE) return false;
   if (texture_states[0].at(D3DTSS_COLOROP) != D3DTOP_MODULATE) return false;
   if (texture_states[0].at(D3DTSS_COLORARG0) != D3DTA_CURRENT) return false;
   if (texture_states[0].at(D3DTSS_COLORARG1) != D3DTA_TFACTOR) return false;
   if (texture_states[0].at(D3DTSS_COLORARG2) != D3DTA_TEXTURE) return false;
   if (texture_states[0].at(D3DTSS_ALPHAOP) != D3DTOP_SELECTARG1) return false;
   if (texture_states[0].at(D3DTSS_ALPHAARG0) != D3DTA_CURRENT) return false;
   if (texture_states[0].at(D3DTSS_ALPHAARG1) != D3DTA_TFACTOR) return false;
   if (texture_states[0].at(D3DTSS_ALPHAARG2) != D3DTA_TEXTURE) return false;

   return true;
}

}

Device::Device(IDirect3DDevice9& device, const HWND window,
               const glm::ivec2 resolution, DWORD max_anisotropy) noexcept
   : _device{device},
     _window{window},
     _imgui_context{ImGui::CreateContext(), &ImGui::DestroyContext},
     _resolution{resolution},
     _device_max_anisotropy{gsl::narrow_cast<int>(max_anisotropy)},
     _fs_vertex_decl{effects::create_fs_triangle_decl(*_device)}
{
   init_optional_format_types();

   if (_config.rendering.advertise_presence) {
      _sp_advertise_handle = win32::Unique_handle{
         CreateFileW(L"data/shaderpatch/installed", GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     nullptr, OPEN_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, nullptr)};
   }
}

Device::~Device()
{
   if (_imgui_bootstrapped) ImGui_ImplDX9_Shutdown();
}

ULONG Device::AddRef() noexcept
{
   return _ref_count += 1;
}

ULONG Device::Release() noexcept
{
   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

HRESULT Device::TestCooperativeLevel() noexcept
{
   if (_fake_device_loss) return D3DERR_DEVICENOTRESET;

   return _device->TestCooperativeLevel();
}

HRESULT Device::Reset(D3DPRESENT_PARAMETERS* presentation_parameters) noexcept
{
   if (presentation_parameters == nullptr) return D3DERR_INVALIDCALL;

   if (const auto& display = _config.display; display.enabled) {
      presentation_parameters->Windowed = display.windowed;
      presentation_parameters->BackBufferWidth = display.resolution.x;
      presentation_parameters->BackBufferHeight = display.resolution.y;
   }

   if (_effects.enabled()) {
      presentation_parameters->MultiSampleType = D3DMULTISAMPLE_NONE;
      presentation_parameters->MultiSampleQuality = 0;
   }

   // drop resources

   _effects_backbuffer.reset(nullptr);
   _backbuffer_override.reset(nullptr);
   _linear_depth_surface.reset(nullptr);
   _linear_depth_texture.reset(nullptr);
   _shadow_texture.reset(nullptr);
   _blur_texture.reset(nullptr);
   _blur_surface.reset(nullptr);
   _refraction_texture.reset(nullptr);
   _fs_vertex_buffer.reset(nullptr);
   _water_texture = Texture{};
   _vertex_input_state = {};

   _textures.clean_lost_textures();
   _effects.drop_device_resources();
   _rt_allocator.reset();

   _material = nullptr;

   // reset device

   const auto result = _device->Reset(presentation_parameters);

   if (result != S_OK) return result;

   // update misc state

   _window_dirty = true;
   _fake_device_loss = false;
   _refresh_material = true;

   _resolution.x = presentation_parameters->BackBufferWidth;
   _resolution.y = presentation_parameters->BackBufferHeight;

   _created_full_rendertargets = 0;
   _created_2_to_1_rendertargets = 0;

   _rt_format = presentation_parameters->BackBufferFormat;

   const auto fl_res = static_cast<glm::vec2>(_resolution);

   _rt_resolution_const.set(*_device, {fl_res, glm::vec2{1.0f} / fl_res});

   init_sampler_max_anisotropy();

   _state_block = create_filled_render_state_block(*_device);
   _sampler_states = create_filled_sampler_state_blocks(*_device);
   _texture_states = create_filled_texture_state_blocks(*_device);
   _transform_state = create_filled_transform_state_block(*_device);
   _vertex_input_state = create_filled_vertex_input_state(*_device);

   // recreate resources

   if (_effects.enabled()) {
      if (_effects.config().hdr_rendering) {
         _rt_format = fp_texture_format;

         set_hdr_rendering(true);
      }
      else if (_config.effects.color_quality == Color_quality::high) {
         _rt_format = _effects_high_stock_hdr_format;
      }
      else if (_config.effects.color_quality == Color_quality::ultra) {
         _rt_format = _effects_ultra_stock_hdr_format;
      }

      _device->CreateTexture(_resolution.x, _resolution.y, 1,
                             D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT,
                             _linear_depth_texture.clear_and_assign(), nullptr);

      _linear_depth_texture->GetSurfaceLevel(0, _linear_depth_surface.clear_and_assign());

      _device->CreateTexture(_resolution.x, _resolution.y, 1,
                             D3DUSAGE_RENDERTARGET, _rt_format, D3DPOOL_DEFAULT,
                             _effects_backbuffer.clear_and_assign(), nullptr);

      _effects_backbuffer->GetSurfaceLevel(0, _backbuffer_override.clear_and_assign());
      _device->SetRenderTarget(0, _backbuffer_override.get());

      _effects.active(true);
   }
   else {
      set_hdr_rendering(false);
      _effects.active(false);
   }

   const auto blur_res = _resolution / blur_buffer_factor;

   _device->CreateTexture(blur_res.x, blur_res.y, 1, D3DUSAGE_RENDERTARGET,
                          _rt_format, D3DPOOL_DEFAULT,
                          _blur_texture.clear_and_assign(), nullptr);
   _blur_texture->GetSurfaceLevel(0, _blur_surface.clear_and_assign());

   const auto refraction_res = _resolution / _config.rendering.refraction_buffer_factor;

   _device->CreateTexture(refraction_res.x, refraction_res.y, 1,
                          D3DUSAGE_RENDERTARGET, _rt_format, D3DPOOL_DEFAULT,
                          _refraction_texture.clear_and_assign(), nullptr);

   _fs_vertex_buffer = effects::create_fs_triangle_buffer(*_device, _resolution);

   if (!_imgui_bootstrapped) {
      ImGui::SetCurrentContext(_imgui_context.get());
      ImGui_ImplDX9_Init(_window, _device.get());
      ImGui_ImplDX9_CreateDeviceObjects();
      ImGui_ImplDX9_NewFrame();

      auto& io = ImGui::GetIO();
      io.MouseDrawCursor = true;

      set_input_window(GetCurrentThreadId(), _window);

      set_input_hotkey(_config.developer.toggle_key);
      set_input_hotkey_func([this] {
         _imgui_active = !_imgui_active;

         if (_imgui_active) {
            set_input_mode(Input_mode::imgui);
         }
         else {
            set_input_mode(Input_mode::normal);
         }
      });

      _imgui_bootstrapped = true;
   }

   _device->BeginScene();

   return result;
}

HRESULT Device::Present(const RECT* source_rect, const RECT* dest_rect,
                        HWND dest_window_override, const RGNDATA* dirty_region) noexcept
{
   if (std::exchange(_window_dirty, false)) {
      _config.display.borderless ? make_borderless_window(_window)
                                 : make_bordered_window(_window);

      resize_window(_window, _resolution);
      centre_window(_window);
      if (GetFocus() == _window) clip_cursor_to_window(_window);
   }

   if (_effects.active()) {
      if (!(_effects_rt_resolved, false)) late_fp_resolve();
   }

   if (_imgui_active) {
      _config.show_imgui(&_fake_device_loss);
      _effects.show_imgui(_window);
      _effects.user_config(_config.effects);

      ImGui::Render();
      ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
   }
   else {
      ImGui::EndFrame();
   }

   ImGui_ImplDX9_NewFrame();

   _device->EndScene();

   const auto result =
      _device->Present(source_rect, dest_rect, dest_window_override, dirty_region);

   _device->BeginScene();

   // update time
   const auto now = std::chrono::steady_clock{}.now();
   const auto time_since_epoch = now - _device_start;

   _time_vs_const.set(*_device,
                      std::chrono::duration<float>{time_since_epoch}.count());
   _time_ps_const.set(*_device,
                      std::chrono::duration<float>{time_since_epoch}.count());

   // update per frame state
   _water_refraction = false;
   _ice_refraction = false;
   _particles_blur = false;
   _render_depth_texture = _effects.active() && _effects.shadows_blur.enabled();

   if (_fake_device_loss) return D3DERR_DEVICELOST;

   if (_effects.active()) {
      set_hdr_rendering(_effects.config().hdr_rendering);

      _effects_backbuffer->GetSurfaceLevel(0, _backbuffer_override.clear_and_assign());
      _device->SetRenderTarget(0, _backbuffer_override.get());
   }

   if (result != D3D_OK) return result;

   return D3D_OK;
}

// Device Resource Creators

HRESULT Device::CreateVolumeTexture(UINT width, UINT height, UINT depth, UINT levels,
                                    DWORD usage, D3DFORMAT format, D3DPOOL pool,
                                    IDirect3DVolumeTexture9** volume_texture,
                                    HANDLE* shared_handle) noexcept
{
   if (const auto type = static_cast<Volume_resource_type>(width);
       type == Volume_resource_type::shader) {
      auto post_upload = [&, this](gsl::span<std::byte> data) {
         try {
            auto [rendertype, shader_group] =
               load_shader(ucfb::Reader{data}, *_device);

            _shaders.add(rendertype, std::move(shader_group));
         }
         catch (std::exception& e) {
            log(Log_level::error,
                "Exception occured while loading shader resource: "sv, e.what());
         }
      };

      auto uploader =
         make_resource_uploader(unpack_resource_size(height, depth), post_upload);

      *volume_texture = uploader.release();

      return S_OK;
   }
   else if (type == Volume_resource_type::texture) {
      auto handle_resource =
         [&, this](gsl::span<std::byte> data) -> std::shared_ptr<Texture> {
         try {
            auto [d3d_texture, name, sampler_info] =
               load_patch_texture(ucfb::Reader{data}, *_device, D3DPOOL_MANAGED);

            auto texture =
               std::make_shared<Texture>(static_cast<Com_ptr<IDirect3DDevice9>>(_device),
                                         std::move(d3d_texture), sampler_info);

            _textures.add(name, texture);

            return texture;
         }
         catch (std::exception& e) {
            log(Log_level::error,
                "Exception occured while loading FX Config: "sv, e.what());

            return nullptr;
         }
      };

      auto handler = make_resource_handler(unpack_resource_size(height, depth),
                                           handle_resource);

      *volume_texture = handler.release();

      return S_OK;
   }
   else if (type == Volume_resource_type::material) {
      *volume_texture = new Material_resource{unpack_resource_size(height, depth),
                                              _device, _shaders, _textures};

      return S_OK;
   }
   else if (type == Volume_resource_type::fx_config) {
      auto handle_resource =
         [&, this](gsl::span<std::byte> data) -> std::optional<Copyable_finally> {
         const auto fx_id = _active_fx_id += 1;

         const auto on_destruction = [fx_id, this] {
            if (fx_id != _active_fx_id.load()) return;

            _effects.enabled(false);
            _fake_device_loss = true;

            set_hdr_rendering(false);
         };

         try {
            std::string config_str{reinterpret_cast<char*>(data.data()),
                                   static_cast<std::size_t>(data.size())};

            while (config_str.back() == '\0') config_str.pop_back();

            _effects.read_config(YAML::Load(config_str));
            _effects.enabled(true);
            _fake_device_loss = true;

            return copyable_finally(on_destruction);
         }
         catch (std::exception& e) {
            log(Log_level::error,
                "Exception occured while loading FX Config: "sv, e.what());

            return std::nullopt;
         }
      };

      auto handler = make_resource_handler(unpack_resource_size(height, depth),
                                           handle_resource);

      *volume_texture = handler.release();

      return S_OK;
   }

   return _device->CreateVolumeTexture(width, height, depth, levels, usage, format,
                                       pool, volume_texture, shared_handle);
}

HRESULT Device::CreateTexture(UINT width, UINT height, UINT levels, DWORD usage,
                              D3DFORMAT format, D3DPOOL pool,
                              IDirect3DTexture9** texture, HANDLE* shared_handle) noexcept
{
   glm::ivec2 size = {static_cast<int>(width), static_cast<int>(height)};

   if (usage & D3DUSAGE_RENDERTARGET) {
      // Reflection Buffer Enhancement
      if (size == glm::ivec2{512, 256} && ++_created_2_to_1_rendertargets == 2) {
         if (_config.rendering.high_res_reflections) {
            size.x = _resolution.y * 2;
            size.y = _resolution.y;
         }

         size /= _config.rendering.reflection_buffer_factor;
      }
      // Shadow Buffer Optimization
      else if (size == _resolution) {
         if (++_created_full_rendertargets == 2) {
            if (_effects.active()) format = _stencil_shadow_format;

            const auto result =
               _device->CreateTexture(size.x, size.y, levels, usage, format,
                                      pool, _shadow_texture.clear_and_assign(),
                                      shared_handle);

            if (FAILED(result)) return result;

            _shadow_texture->AddRef();
            *texture = _shadow_texture.get();

            return result;
         }
      }

      format = _rt_format;
   }

   return _device->CreateTexture(size.x, size.y, levels, usage, format, pool,
                                 texture, shared_handle);
}

HRESULT Device::CreateDepthStencilSurface(UINT width, UINT height, D3DFORMAT format,
                                          D3DMULTISAMPLE_TYPE multi_sample,
                                          DWORD multi_sample_quality, BOOL discard,
                                          IDirect3DSurface9** surface,
                                          HANDLE* shared_handle) noexcept
{
   if (width == 512u && height == 256u) {
      if (_config.rendering.high_res_reflections) {
         width = _resolution.y * 2u;
         height = _resolution.y;
      }

      width /= _config.rendering.reflection_buffer_factor;
      height /= _config.rendering.reflection_buffer_factor;
   }

   if (_effects.active()) {
      multi_sample = D3DMULTISAMPLE_NONE;
      multi_sample_quality = 0;
   }

   return _device->CreateDepthStencilSurface(width, height, format,
                                             multi_sample, multi_sample_quality,
                                             discard, surface, shared_handle);
}

HRESULT Device::CreateVertexShader(const DWORD* function,
                                   IDirect3DVertexShader9** shader) noexcept
{
   Com_ptr<IDirect3DVertexShader9> vertex_shader;

   const auto result =
      _device->CreateVertexShader(function, vertex_shader.clear_and_assign());

   if (result != S_OK) return result;

   const auto metadata = get_shader_metadata(function);

   *shader = new Vertex_shader{std::move(vertex_shader),
                               metadata.value_or(Shader_metadata{})};

   return S_OK;
}

HRESULT Device::CreatePixelShader(const DWORD* function,
                                  IDirect3DPixelShader9** shader) noexcept
{
   Com_ptr<IDirect3DPixelShader9> pixel_shader;

   const auto result =
      _device->CreatePixelShader(function, pixel_shader.clear_and_assign());

   if (result != S_OK) return result;

   const auto metadata = get_shader_metadata(function);

   *shader = new Pixel_shader{std::move(pixel_shader),
                              metadata.value_or(Shader_metadata{})};

   return S_OK;
}

// Device State Setters

HRESULT Device::SetTexture(DWORD stage, IDirect3DBaseTexture9* texture) noexcept
{
   if (texture == nullptr) {
      if (stage == 0 && _material) {
         clear_material();
      }

      return _device->SetTexture(stage, nullptr);
   }

   auto type = texture->GetType();

   // Custom Material Handling
   if (stage == 0 && type == Material_resource::id) {
      auto& material_resource = static_cast<Material_resource&>(*texture);
      _material = material_resource.get_material();

      _material->bind();

      _refresh_material = true;

      return S_OK;
   }
   else if (stage != 0 && type == Material_resource::id) {
      log(Log_level::warning, "Attempt to bind material to texture slot other than 0."sv);

      return S_OK;
   }
   else if (stage == 0 && _material) {
      clear_material();
   }

   // Projected Cube Texture Workaround
   if (type == D3DRTYPE_CUBETEXTURE && stage == 2) {
      set_ps_bool_constant<constants::ps::cubemap_projection>(*_device, true);

      apply_sampler_state(*_device, _sampler_states[2], cubemap_projection_slot);

      _device->SetTexture(cubemap_projection_slot, texture);
   }
   else if (stage == 2) {
      set_ps_bool_constant<constants::ps::cubemap_projection>(*_device, false);
   }

   return _device->SetTexture(stage, texture);
}

HRESULT Device::SetRenderTarget(DWORD render_target_index,
                                IDirect3DSurface9* render_target) noexcept
{
   if (render_target != nullptr) {
      auto [_, res] = effects::get_surface_metrics(*render_target);

      const auto fl_res = static_cast<glm::vec2>(res);

      _rt_resolution_const.set(*_device, {fl_res, glm::vec2{1.0f} / fl_res});
   }

   return _device->SetRenderTarget(render_target_index, render_target);
}

HRESULT Device::SetVertexShader(IDirect3DVertexShader9* shader) noexcept
{
   if (!shader) {
      _game_vertex_shader = nullptr;
      return _device->SetVertexShader(nullptr);
   }

   auto& vertex_shader = static_cast<Vertex_shader&>(*shader);

   _vs_metadata = vertex_shader.metadata;
   vertex_shader.get()->AddRef();
   _game_vertex_shader.reset(vertex_shader.get());

   if (_hdr_rendering) {
      for (auto i = 0; i < 4; ++i) {
         _device->SetSamplerState(i, D3DSAMP_SRGBTEXTURE, _vs_metadata.srgb_state[i]);
      }
   }

   _refresh_material = true;

   if (_material && _material->target_rendertype() == _vs_metadata.rendertype) {
      return S_OK;
   }

   if (_effects.active()) {
      if (_render_depth_texture && !_zprepass && _vs_metadata.rendertype == "zprepass"sv) {
         _device->SetRenderTarget(0, _linear_depth_surface.get());
         _device->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 0.0, 0);
         _device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED);
         _zprepass = true;
      }
      else if (_render_depth_texture && _zprepass &&
               _vs_metadata.rendertype != "zprepass"sv) {
         Com_ptr<IDirect3DSurface9> backbuffer;
         this->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,
                             backbuffer.clear_and_assign());
         _device->SetRenderTarget(0, backbuffer.get());
         _device->SetRenderState(D3DRS_COLORWRITEENABLE,
                                 _state_block.get(D3DRS_COLORWRITEENABLE));
         _zprepass = false;
      }
      else if (_vs_metadata.rendertype == "shadowquad"sv) {

         Com_ptr<IDirect3DSurface9> shadow_surface;
         Com_ptr<IDirect3DSurface9> rt;

         _shadow_texture->GetSurfaceLevel(0, shadow_surface.clear_and_assign());
         _device->GetRenderTarget(0, rt.clear_and_assign());

         _device->SetRenderTarget(0, shadow_surface.get());
         _device->ColorFill(shadow_surface.get(), nullptr, 0xffffffff);

         _stretch_rect_hook = [this,
                               rt{std::move(rt)}](IDirect3DSurface9*, const RECT*,
                                                  IDirect3DSurface9*, const RECT*,
                                                  D3DTEXTUREFILTERTYPE) {
            _stretch_rect_hook = nullptr;

            if (_effects.shadows_blur.enabled()) blur_shadows();

            _device->SetRenderTarget(0, rt.get());

            return S_OK;
         };
      }
      else if (_vs_metadata.rendertype == "hdr"sv) {
         _game_doing_bloom_pass = true;
         _discard_draw_calls = _effects.active();
      }
      else if (std::exchange(_game_doing_bloom_pass, false) &&
               _vs_metadata.rendertype != "hdr"sv) {
         _effects_rt_resolved = true;
         _discard_draw_calls = false;
         set_hdr_rendering(false);

         post_process();

         _backbuffer_override = nullptr;
      }
   }

   set_active_light_constants(*_device, vertex_shader.metadata.shader_flags);

   return _device->SetVertexShader(vertex_shader.get());
}

HRESULT Device::SetPixelShader(IDirect3DPixelShader9* shader) noexcept
{
   if (_on_ps_shader_set) _on_ps_shader_set();

   if (!shader) {
      _game_pixel_shader = nullptr;

      if (_config.rendering.gaussian_scene_blur &&
          is_blur_blend_state(_texture_states)) {
         prepapre_gaussian_scene_blur();
      }

      return _device->SetPixelShader(nullptr);
   }

   auto& pixel_shader = static_cast<Pixel_shader&>(*shader);
   const auto& metadata = pixel_shader.metadata;

   pixel_shader.get()->AddRef();
   _game_pixel_shader.reset(pixel_shader.get());

   if (_material) {
      _refresh_material = true;

      return S_OK;
   }

   if (metadata.rendertype == "shield"sv) {
      update_refraction_texture();
      bind_refraction_texture();
      bind_water_texture();

      _device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      _device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
      _device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

      _on_ps_shader_set = [this] {
         _device->SetRenderState(D3DRS_SRCBLEND, _state_block.at(D3DRS_SRCBLEND));
         _device->SetRenderState(D3DRS_DESTBLEND, _state_block.at(D3DRS_DESTBLEND));
         _device->SetRenderState(D3DRS_CULLMODE, _state_block.at(D3DRS_CULLMODE));

         _on_ps_shader_set = nullptr;
      };
   }
   else if (!_water_refraction && metadata.rendertype == "water"sv) {
      if ((metadata.entry_point == "normal_map_distorted_reflection_ps") ||
          (metadata.entry_point ==
           "normal_map_distorted_reflection_specular_ps")) {
         update_refraction_texture();
         bind_refraction_texture();
         bind_water_texture();

         _water_refraction = true;
      }
   }
   else if (metadata.rendertype == "refraction"sv && metadata.state_name != "far"sv &&
            metadata.state_name != "nodistortion"sv) {
      if (!std::exchange(_ice_refraction, true)) update_refraction_texture();

      bind_refraction_texture();
   }
   else if (_config.rendering.gaussian_blur_blur_particles &&
            metadata.rendertype == "particle"sv && metadata.state_name == "blur"sv) {
      if (!std::exchange(_particles_blur, true)) resolve_blur_surface();

      _device->SetTexture(1, _blur_texture.get());
   }
   else if (_config.rendering.smooth_bloom && !_effects.active() &&
            metadata.rendertype == "hdr"sv) {
      if (auto ext_shader = _shaders.at("stock bloom ext"s).find(metadata.state_name);
          ext_shader != nullptr) {
         ext_shader->bind(*_device);

         return S_OK;
      }
   }

   return _device->SetPixelShader(pixel_shader.get());
}

HRESULT Device::SetRenderState(D3DRENDERSTATETYPE state, DWORD value) noexcept
{
   switch (state) {
   case D3DRS_FOGENABLE: {
      BOOL enabled = value ? true : false;

      _device->SetPixelShaderConstantB(constants::ps::fog_enabled, &enabled, 1);

      break;
   }
   case D3DRS_FOGSTART: {
      auto fog_state = _fog_range_const.get();

      fog_state.x = reinterpret_cast<float&>(value);

      _fog_range_const.local_update(fog_state);

      break;
   }
   case D3DRS_FOGEND: {
      auto fog_state = _fog_range_const.get();

      fog_state.y = reinterpret_cast<float&>(value);

      if (fog_state.x == 0.0f && fog_state.y == 0.0f) {
         fog_state.y += 1000000.f;
      }

      if (fog_state.x == fog_state.y) {
         fog_state.y += 0.00001f;
      }

      value = reinterpret_cast<DWORD&>(fog_state.y);

      fog_state.z = 1.0f / (fog_state.y - fog_state.x);

      _fog_range_const.set(*_device, fog_state);

      break;
   }
   case D3DRS_FOGCOLOR: {
      auto color = _fog_color_const.get();

      color.x = static_cast<float>((0x00ff0000 & value) >> 16) / 255.0f;
      color.y = static_cast<float>((0x0000ff00 & value) >> 8) / 255.0f;
      color.z = static_cast<float>((0x000000ff & value)) / 255.0f;

      if (_hdr_rendering) color = glm::pow(color, glm::vec3{2.2f});

      _fog_color_const.set(*_device, color);

      break;
   }
   case D3DRS_SRCBLEND: {
      if (_hdr_rendering && value == D3DBLEND_DESTCOLOR) {
         _multiply_blendstate_ps_const.set(_device, 1.0f);
      }
      else if (_multiply_blendstate_ps_const.get() != 0.0f) {
         _multiply_blendstate_ps_const.set(_device, 0.0f);
      }
      break;
   }
   }

   _state_block.set(state, value);

   return _device->SetRenderState(state, value);
}

HRESULT Device::SetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE state, DWORD value) noexcept
{
   if (sampler >= 16) return D3DERR_INVALIDCALL;

   switch (state) {
   case D3DSAMP_MINFILTER:
      if (value == D3DTEXF_LINEAR && _config.rendering.force_anisotropic_filtering) {
         value = D3DTEXF_ANISOTROPIC;
      }
   }

   _sampler_states[sampler].at(state) = value;

   return _device->SetSamplerState(sampler, state, value);
}

HRESULT Device::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE type,
                                     DWORD value) noexcept
{
   if (stage >= 8) return D3DERR_INVALIDCALL;

   _texture_states[stage].set(type, value);

   return _device->SetTextureStageState(stage, type, value);
}

HRESULT Device::SetTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX* matrix) noexcept
{
   _transform_state.set(state, *matrix);

   return _device->SetTransform(state, matrix);
}

HRESULT Device::SetStreamSource(UINT stream_number, IDirect3DVertexBuffer9* stream_data,
                                UINT offset_in_bytes, UINT stride) noexcept
{
   if (stream_data != nullptr) stream_data->AddRef();

   _vertex_input_state.buffer = Com_ptr{stream_data};
   _vertex_input_state.offset = offset_in_bytes;
   _vertex_input_state.stride = stride;

   return _device->SetStreamSource(stream_number, stream_data, offset_in_bytes, stride);
}

HRESULT Device::SetVertexDeclaration(IDirect3DVertexDeclaration9* decl) noexcept
{
   if (decl != nullptr) decl->AddRef();

   _vertex_input_state.declaration = Com_ptr{decl};

   return _device->SetVertexDeclaration(decl);
}

HRESULT Device::SetVertexShaderConstantF(UINT start_register, const float* constant_data,
                                         UINT vector4f_count) noexcept
{
   if (int offset = constants::stock::hdr - start_register;
       is_constant_being_set(constants::stock::hdr, start_register, vector4f_count) &&
       offset > -1 && _hdr_rendering) {
      const_cast<float*>(constant_data)[offset * 4 + 2] = 1.0f;
   }

   if (start_register == 2) {
      _device->SetPixelShaderConstantF(6, constant_data + 16, vector4f_count - 4);
   }
   else if (start_register != 2 && start_register != 51) {
      _device->SetPixelShaderConstantF(start_register, constant_data, vector4f_count);
   }

   return _device->SetVertexShaderConstantF(start_register, constant_data,
                                            vector4f_count);
}

// Device State Getters

HRESULT Device::GetBackBuffer(UINT swap_chain, UINT back_buffer_index,
                              D3DBACKBUFFER_TYPE type,
                              IDirect3DSurface9** back_buffer) noexcept
{
   if (swap_chain || back_buffer_index) return D3DERR_INVALIDCALL;

   if (_backbuffer_override) {
      *back_buffer = _backbuffer_override.get();
      _backbuffer_override->AddRef();

      return S_OK;
   }

   return _device->GetBackBuffer(swap_chain, back_buffer_index, type, back_buffer);
}

// Device Command Issuers

HRESULT Device::StretchRect(IDirect3DSurface9* source_surface,
                            const RECT* source_rect, IDirect3DSurface9* dest_surface,
                            const RECT* dest_rect, D3DTEXTUREFILTERTYPE filter) noexcept
{
   if (_stretch_rect_hook) {
      return _stretch_rect_hook(source_surface, source_rect, dest_surface,
                                dest_rect, filter);
   }

   return _device->StretchRect(source_surface, source_rect, dest_surface,
                               dest_rect, filter);
}

HRESULT Device::DrawPrimitive(D3DPRIMITIVETYPE primitive_type,
                              UINT start_vertex, UINT primitive_count) noexcept
{
   if (_discard_draw_calls || std::exchange(_discard_next_nonindexed_draw, false)) {
      return S_OK;
   }

   refresh_material();

   return _device->DrawPrimitive(primitive_type, start_vertex, primitive_count);
}

HRESULT Device::DrawIndexedPrimitive(D3DPRIMITIVETYPE primitive_type,
                                     INT base_vertex_index,
                                     UINT min_vertex_index, UINT num_vertices,
                                     UINT start_Index, UINT prim_Count) noexcept
{
   if (_discard_draw_calls) return S_OK;

   refresh_material();

   return _device->DrawIndexedPrimitive(primitive_type, base_vertex_index,
                                        min_vertex_index, num_vertices,
                                        start_Index, prim_Count);
}

// Shader Patch Functions

void Device::init_sampler_max_anisotropy() noexcept
{
   for (int i = 0; i < 16; ++i) {
      _device->SetSamplerState(i, D3DSAMP_MAXANISOTROPY,
                               std::clamp(_config.rendering.anisotropic_filtering,
                                          1, _device_max_anisotropy));
   }
}

void Device::init_optional_format_types() noexcept
{
   Com_ptr<IDirect3D9> d3d;
   _device->GetDirect3D(d3d.clear_and_assign());

   if (d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                              D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET,
                              D3DRTYPE_TEXTURE, D3DFMT_A8) == S_OK) {
      _stencil_shadow_format = D3DFMT_A8;
   }
   else {
      _stencil_shadow_format = D3DFMT_A8R8G8B8;
   }

   if (d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                              D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET,
                              D3DRTYPE_TEXTURE, D3DFMT_A2B10G10R10) == S_OK) {
      _effects_high_stock_hdr_format = D3DFMT_A2B10G10R10;
   }
   else if (d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                   D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET,
                                   D3DRTYPE_TEXTURE, D3DFMT_A2R10G10B10) == S_OK) {
      _effects_high_stock_hdr_format = D3DFMT_A2R10G10B10;
   }
   else {
      _effects_high_stock_hdr_format = D3DFMT_A8R8G8B8;
   }

   if (d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                              D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET,
                              D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16) == S_OK) {
      _effects_ultra_stock_hdr_format = D3DFMT_A16B16G16R16;
   }
   else {
      _effects_ultra_stock_hdr_format = D3DFMT_A16B16G16R16F;
   }
}

void Device::post_process() noexcept
{
   Com_ptr<IDirect3DSurface9> backbuffer;
   _device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,
                          backbuffer.clear_and_assign());
   _device->SetRenderTarget(0, backbuffer.get());

   Com_ptr<IDirect3DStateBlock9> state;

   _device->CreateStateBlock(D3DSBT_ALL, state.clear_and_assign());
   _device->SetRenderState(D3DRS_ZENABLE, FALSE);
   _device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

   _effects.postprocess.apply(_shaders, _rt_allocator, _textures,
                              *_effects_backbuffer, *backbuffer);

   state->Apply();
}

void Device::late_fp_resolve() noexcept
{
   Com_ptr<IDirect3DSurface9> backbuffer;
   _device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,
                          backbuffer.clear_and_assign());
   _device->SetRenderTarget(0, backbuffer.get());

   _device->StretchRect(_backbuffer_override.get(), nullptr, backbuffer.get(),
                        nullptr, D3DTEXF_NONE);
}

void Device::refresh_material() noexcept
{
   if (std::exchange(_refresh_material, false) && _material) {
      if (_vs_metadata.rendertype == _material->target_rendertype()) {
         _material->update(_vs_metadata.state_name, _vs_metadata.shader_flags);
      }
   }
}

void Device::clear_material() noexcept
{
   if (_material) {
      _material = nullptr;
      _refresh_material = false;

      _device->SetVertexShader(_game_vertex_shader.get());
      _device->SetPixelShader(_game_pixel_shader.get());

      set_active_light_constants(*_device, _vs_metadata.shader_flags);
   }
}

void Device::bind_water_texture() noexcept
{
   if (!_water_texture) {
      _water_texture = _textures.get("_SP_BUILTIN_water"s);
   }

   _water_texture.bind(water_slot);
}

void Device::bind_refraction_texture() noexcept
{
   _device->SetTexture(refraction_slot, _refraction_texture.get());

   _device->SetSamplerState(refraction_slot, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   _device->SetSamplerState(refraction_slot, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   _device->SetSamplerState(refraction_slot, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
   _device->SetSamplerState(refraction_slot, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
   _device->SetSamplerState(refraction_slot, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
   _device->SetSamplerState(refraction_slot, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
}

void Device::update_refraction_texture() noexcept
{
   Com_ptr<IDirect3DSurface9> rt;

   _device->GetRenderTarget(0, rt.clear_and_assign());

   Com_ptr<IDirect3DSurface9> dest;
   _refraction_texture->GetSurfaceLevel(0, dest.clear_and_assign());

   _device->StretchRect(rt.get(), nullptr, dest.get(), nullptr, D3DTEXF_LINEAR);
}

void Device::blur_shadows() noexcept
{
   _device->SetRenderState(D3DRS_STENCILENABLE, FALSE);

   _effects.shadows_blur.apply(_shaders.at("shadows blur"s), _rt_allocator,
                               *_linear_depth_texture, *_shadow_texture);

   apply_sampler_state(*_device, _sampler_states[0], 0);
   apply_sampler_state(*_device, _sampler_states[1], 1);
   apply_blend_state(*_device, _state_block);
   apply_vertex_input_state(*_device, _vertex_input_state);
}

void Device::resolve_blur_surface() noexcept
{
   Com_ptr<IDirect3DSurface9> backbuffer;
   GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, backbuffer.clear_and_assign());

   Com_ptr<IDirect3DBaseTexture9> actual_texture;
   _device->GetTexture(0, actual_texture.clear_and_assign());

   const auto [format, _] = effects::get_surface_metrics(*_blur_surface);
   auto resolve = _rt_allocator.allocate(format, _resolution / blur_resolve_factor);

   _device->StretchRect(backbuffer.get(), nullptr, resolve.surface(), nullptr,
                        D3DTEXF_LINEAR);
   _device->StretchRect(resolve.surface(), nullptr, _blur_surface.get(),
                        nullptr, D3DTEXF_LINEAR);

   Com_ptr<IDirect3DSurface9> rt;

   _device->GetRenderTarget(0, rt.clear_and_assign());

   _effects.scene_blur.apply(_shaders, _rt_allocator, *_blur_texture);

   _device->SetVertexShader(_game_vertex_shader.get());
   _device->SetPixelShader(_game_pixel_shader.get());
   _device->SetRenderTarget(0, rt.get());

   _device->SetTexture(0, actual_texture.get());
   apply_sampler_state(*_device, _sampler_states[0], 0);
   apply_blend_state(*_device, _state_block);
   apply_vertex_input_state(*_device, _vertex_input_state);
}

void Device::prepapre_gaussian_scene_blur() noexcept
{
   resolve_blur_surface();

   _device->SetTexture(0, _blur_texture.get());
   _device->SetStreamSource(0, _fs_vertex_buffer.get(), 0, effects::fs_triangle_stride);
   _device->SetVertexDeclaration(_fs_vertex_decl.get());

   constexpr D3DMATRIX transform = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
                                    0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};

   _device->SetTransform(D3DTS_PROJECTION, &transform);
   _device->SetTransform(D3DTS_VIEW, &transform);

   _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

   apply_transform_state_block_partial(*_device, _transform_state);
   apply_vertex_input_state(*_device, _vertex_input_state);

   _discard_next_nonindexed_draw = true;
}

void Device::set_hdr_rendering(bool hdr_rendering) noexcept
{
   _hdr_rendering = hdr_rendering;

   if (_hdr_rendering) {
      _hdr_state_vs_const.set(*_device, {2.2f, 1.0f});
      _hdr_state_ps_const.set(*_device, {2.2f, 1.0f});

      for (auto i = 0; i < 4; ++i) {
         _device->SetSamplerState(i, D3DSAMP_SRGBTEXTURE, _vs_metadata.srgb_state[i]);
      }
   }
   else {
      _hdr_state_vs_const.set(*_device, {1.0f, 0.0f});
      _hdr_state_ps_const.set(*_device, {1.0f, 0.0f});

      for (auto i = 0; i < 4; ++i) {
         _device->SetSamplerState(i, D3DSAMP_SRGBTEXTURE, FALSE);
      }
   }

   SetRenderState(D3DRS_FOGCOLOR, _state_block.get(D3DRS_FOGCOLOR));
}
}
