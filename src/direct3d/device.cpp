
#include "device.hpp"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../input_hooker.hpp"
#include "../logger.hpp"
#include "../shader_constants.hpp"
#include "../shader_metadata.hpp"
#include "../texture_loader.hpp"
#include "../window_helpers.hpp"
#include "rendertarget.hpp"
#include "sampler_state_block.hpp"
#include "shader.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <string_view>
#include <vector>

#include <INIReader.h>
#include <gsl/gsl>

using namespace std::literals;

namespace sp::direct3d {

namespace {

void set_active_light_constants(IDirect3DDevice9& device, Vs_flags flags) noexcept
{
   std::array<BOOL, 5> constants{};

   if ((flags & Vs_flags::light_dir) == Vs_flags::light_dir) {
      constants[0] = true;
   }

   if ((flags & Vs_flags::light_point) == Vs_flags::light_point) {
      std::fill_n(std::begin(constants) + 1, 1, true);
   }

   if ((flags & Vs_flags::light_point2) == Vs_flags::light_point2) {
      std::fill_n(std::begin(constants) + 1, 2, true);
   }

   if ((flags & Vs_flags::light_point4) == Vs_flags::light_point4) {
      std::fill_n(std::begin(constants) + 1, 3, true);
   }

   if ((flags & Vs_flags::light_spot) == Vs_flags::light_spot) {
      constants[4] = true;
   }

   device.SetPixelShaderConstantB(0, constants.data(), constants.size());
}
}

Device::Device(Com_ptr<IDirect3DDevice9> device, const HWND window,
               const glm::ivec2 resolution) noexcept
   : _device{std::move(device)}, _window{window}, _resolution{resolution}
{
   _water_texture =
      load_dds_from_file(*_device, L"data/shaderpatch/textures/water.dds"s);
}

Device::~Device()
{
   if (_imgui_bootstrapped) ImGui_ImplDX9_Shutdown();
}

ULONG Device::AddRef() noexcept
{
   return _ref_count.fetch_add(1);
}

ULONG Device::Release() noexcept
{
   const auto ref_count = _ref_count.fetch_sub(1);

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

   // drop resources

   _refraction_texture.reset(nullptr);

   // reset device

   const auto result = _device->Reset(presentation_parameters);

   if (result != S_OK) return result;

   // update misc state

   _window_dirty = true;
   _fake_device_loss = false;

   _resolution.x = presentation_parameters->BackBufferWidth;
   _resolution.y = presentation_parameters->BackBufferHeight;

   const auto fl_res = static_cast<glm::vec2>(_resolution);

   _rt_resolution_const.set(*_device, {fl_res, glm::vec2{1.0f} / fl_res});

   _state_block = create_filled_render_state_block(*_device);

   // recreate textures

   const auto refraction_res = _resolution / _config.rendering.refraction_buffer_factor;

   _device->CreateTexture(refraction_res.x, refraction_res.y, 1,
                          D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
                          _refraction_texture.clear_and_assign(), nullptr);

   // rebind textures

   bind_water_texture();
   bind_refraction_texture();

   if (!_imgui_bootstrapped) {
      ImGui_ImplDX9_Init(_window, _device.get());
      ImGui_ImplDX9_CreateDeviceObjects();
      ImGui_ImplDX9_NewFrame();

      auto& io = ImGui::GetIO();
      io.MouseDrawCursor = true;

      set_input_window(GetCurrentThreadId(), _window);

      set_input_hotkey(_config.debugscreen.activate_key);
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

   return result;
}

HRESULT Device::Present(const RECT* source_rect, const RECT* dest_rect,
                        HWND dest_window_override, const RGNDATA* dirty_region) noexcept
{
   if (_window_dirty) {
      if (_config.display.borderless) make_borderless_window(_window);
      resize_window(_window, _resolution);
      centre_window(_window);
      if (GetFocus() == _window) clip_cursor_to_window(_window);

      _window_dirty = false;
   }

   if (_imgui_active) {
      _device->BeginScene();

      ImGui::Render();

      _device->EndScene();
   }
   else {
      ImGui::EndFrame();
   }

   ImGui_ImplDX9_NewFrame();

   // update time
   const auto now = std::chrono::steady_clock{}.now();
   const auto time_since_epoch = now - _device_start;

   _time_vs_const.set(*_device,
                      std::chrono::duration<float>{time_since_epoch}.count());

   // update water refraction
   _water_refraction = false;

   const auto result =
      _device->Present(source_rect, dest_rect, dest_window_override, dirty_region);

   if (result != D3D_OK) return result;

   if (_fake_device_loss) return D3DERR_DEVICELOST;

   return D3D_OK;
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

   return _device->CreateDepthStencilSurface(width, height, format,
                                             multi_sample, multi_sample_quality,
                                             discard, surface, shared_handle);
}

HRESULT Device::SetTexture(DWORD stage, IDirect3DBaseTexture9* texture) noexcept
{
   if (texture == nullptr) return _device->SetTexture(stage, nullptr);

   if (auto type = texture->GetType(); type == D3DRTYPE_CUBETEXTURE && stage == 2) {
      set_ps_bool_constant<constants::ps::cubemap_projection>(*_device, true);

      create_filled_sampler_state_block(*_device, 2).apply(*_device, cubemap_projection_slot);

      return _device->SetTexture(cubemap_projection_slot, texture);
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
      D3DSURFACE_DESC desc{};

      render_target->GetDesc(&desc);

      const auto fl_res = static_cast<glm::vec2>(glm::ivec2{desc.Width, desc.Height});

      _rt_resolution_const.set(*_device, {fl_res, glm::vec2{1.0f} / fl_res});
   }

   return _device->SetRenderTarget(render_target_index, render_target);
}

HRESULT Device::CreateTexture(UINT width, UINT height, UINT levels, DWORD usage,
                              D3DFORMAT format, D3DPOOL pool,
                              IDirect3DTexture9** texture, HANDLE* shared_handle) noexcept
{
   if (usage & D3DUSAGE_RENDERTARGET) {
      if (width == 512u && height == 256u) {
         if (_config.rendering.high_res_reflections) {
            width = _resolution.y * 2u;
            height = _resolution.y;
         }

         width /= _config.rendering.reflection_buffer_factor;
         height /= _config.rendering.reflection_buffer_factor;
      }
   }

   return _device->CreateTexture(width, height, levels, usage, format, pool,
                                 texture, shared_handle);
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

HRESULT Device::SetVertexShader(IDirect3DVertexShader9* shader) noexcept
{
   if (!shader) return _device->SetVertexShader(nullptr);

   auto* const vertex_shader = static_cast<Vertex_shader*>(shader);

   if (vertex_shader->metadata.vs_flags) {
      set_active_light_constants(*_device, *vertex_shader->metadata.vs_flags);
   }

   return _device->SetVertexShader(&vertex_shader->get());
}

HRESULT Device::SetPixelShader(IDirect3DPixelShader9* shader) noexcept
{
   if (_on_ps_shader_set) _on_ps_shader_set();

   if (!shader) return _device->SetPixelShader(nullptr);

   auto* const pixel_shader = static_cast<Pixel_shader*>(shader);

   if (pixel_shader->metadata.name == "shield"sv) {
      update_refraction_texture();

      _device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      _device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

      _on_ps_shader_set = [this] {
         _device->SetRenderState(D3DRS_SRCBLEND, _state_block.at(D3DRS_SRCBLEND));
         _device->SetRenderState(D3DRS_DESTBLEND, _state_block.at(D3DRS_DESTBLEND));

         _on_ps_shader_set = nullptr;
      };
   }
   else if (!_water_refraction && pixel_shader->metadata.name == "water"sv) {
      if ((pixel_shader->metadata.entry_point ==
           "normal_map_distorted_reflection_ps") ||
          (pixel_shader->metadata.entry_point ==
           "normal_map_distorted_reflection_specular_ps")) {
         update_refraction_texture();

         _water_refraction = true;
      }
   }

   return _device->SetPixelShader(&pixel_shader->get());
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

      _fog_color_const.set(*_device, color);

      break;
   }
   }

   _state_block.set(state, value);

   return _device->SetRenderState(state, value);
}

HRESULT Device::SetVertexShaderConstantF(UINT start_register, const float* constant_data,
                                         UINT vector4f_count) noexcept
{
   if (start_register == 2) {
      _device->SetPixelShaderConstantF(constants::ps::projection_matrix,
                                       constant_data, 4);
      _device->SetPixelShaderConstantF(6, constant_data + 16, vector4f_count - 4);
   }
   else if (start_register != 2 && start_register != 51) {
      _device->SetPixelShaderConstantF(start_register, constant_data, vector4f_count);
   }

   return _device->SetVertexShaderConstantF(start_register, constant_data,
                                            vector4f_count);
}

void Device::bind_water_texture() noexcept
{
   _device->SetTexture(water_slot, _water_texture.get());

   _device->SetSamplerState(water_slot, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
   _device->SetSamplerState(water_slot, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
   _device->SetSamplerState(water_slot, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
   _device->SetSamplerState(water_slot, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
   _device->SetSamplerState(water_slot, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
   _device->SetSamplerState(water_slot, D3DSAMP_MAXANISOTROPY, 0x8);
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
}
