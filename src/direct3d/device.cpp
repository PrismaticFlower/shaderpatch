
#include "device.hpp"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../input_hooker.hpp"
#include "../logger.hpp"
#include "../material_resource.hpp"
#include "../resource_handle.hpp"
#include "../shader_constants.hpp"
#include "../shader_loader.hpp"
#include "../texture_loader.hpp"
#include "../window_helpers.hpp"
#include "patch_texture.hpp"
#include "sampler_state_block.hpp"
#include "shader_metadata.hpp"
#include "volume_resource.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <string_view>
#include <vector>

#include <gsl/gsl>

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

auto create_fs_vertex_buffer(IDirect3DDevice9& device, const glm::ivec2 resolution)
   -> Com_ptr<IDirect3DVertexBuffer9>
{
   const auto offset = 1.f / static_cast<glm::vec2>(resolution);

   const auto vertices = std::array<float, 12>{{
      -1.f - offset.x, -1.f + offset.y, // pos 0
      0.f, 1.f,                         // uv 0
      -1.f - offset.x, 3.f + offset.x,  // pos 1
      0.f, -1.f,                        // uv 1
      3.f - offset.x, -1.f + offset.x,  // pos 2
      2.f, 1.f                          // uv 2
   }};

   Com_ptr<IDirect3DVertexBuffer9> buffer;

   device.CreateVertexBuffer(sizeof(vertices), D3DUSAGE_WRITEONLY, 0,
                             D3DPOOL_DEFAULT, buffer.clear_and_assign(), nullptr);

   void* data_ptr{};

   buffer->Lock(0, sizeof(vertices), &data_ptr, 0);

   *static_cast<std::array<float, 12>*>(data_ptr) = vertices;

   buffer->Unlock();

   return buffer;
}

auto create_fs_vertex_decl(IDirect3DDevice9& device)
   -> Com_ptr<IDirect3DVertexDeclaration9>
{
   constexpr auto declaration = std::array<D3DVERTEXELEMENT9, 3>{
      {{0, 0, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_POSITION, 0},
       {0, 8, D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0},
       D3DDECL_END()}};

   Com_ptr<IDirect3DVertexDeclaration9> vertex_decl;

   device.CreateVertexDeclaration(declaration.data(), vertex_decl.clear_and_assign());

   return vertex_decl;
}

constexpr auto get_fs_buffer_stride()
{
   return 16u;
}
}

Device::Device(Com_ptr<IDirect3DDevice9> device, const HWND window,
               const glm::ivec2 resolution, const D3DCAPS9& caps) noexcept
   : _device{std::move(device)},
     _window{window},
     _resolution{resolution},
     _device_max_anisotropy{gsl::narrow_cast<int>(caps.MaxAnisotropy)},
     _fs_vertex_decl{create_fs_vertex_decl(*_device)}
{
   _water_texture =
      load_dds_from_file(*_device, L"data/shaderpatch/textures/water.dds"s);

   if (_config.rendering.custom_materials) {
      _materials_enabled_handle = win32::Unique_handle{
         CreateFileW(L"data/shaderpatch/materials_enabled", GENERIC_READ | GENERIC_WRITE,
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

   if (_using_fp_rendertargets) {
      presentation_parameters->MultiSampleType = D3DMULTISAMPLE_NONE;
      presentation_parameters->MultiSampleQuality = 0;
   }

   // drop resources

   _fp_backbuffer.reset(nullptr);
   _backbuffer_override.reset(nullptr);
   _refraction_texture.reset(nullptr);
   _fs_vertex_buffer.reset(nullptr);

   _textures.clean_lost_textures();
   _color_grading.drop_device_resources();

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

   const auto fl_res = static_cast<glm::vec2>(_resolution);

   _rt_resolution_const.set(*_device, {fl_res, glm::vec2{1.0f} / fl_res});

   _state_block = create_filled_render_state_block(*_device);

   // recreate resources

   const auto refraction_res = _resolution / _config.rendering.refraction_buffer_factor;

   if (_using_fp_rendertargets) {
      _device->CreateTexture(_resolution.x, _resolution.y, 1, D3DUSAGE_RENDERTARGET,
                             D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT,
                             _fp_backbuffer.clear_and_assign(), nullptr);

      _fp_backbuffer->GetSurfaceLevel(0, _backbuffer_override.clear_and_assign());
      _device->SetRenderTarget(0, _backbuffer_override.get());

      _linear_rendering = true;
   }

   _device->CreateTexture(refraction_res.x, refraction_res.y, 1, D3DUSAGE_RENDERTARGET,
                          _using_fp_rendertargets ? D3DFMT_A16B16G16R16F : D3DFMT_A8R8G8B8,
                          D3DPOOL_DEFAULT,
                          _refraction_texture.clear_and_assign(), nullptr);

   _fs_vertex_buffer = create_fs_vertex_buffer(*_device, _resolution);

   // rebind textures

   bind_water_texture();
   bind_refraction_texture();
   init_sampler_max_anisotropy();

   if (!_imgui_bootstrapped) {
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

   if (_using_fp_rendertargets) {
      set_linear_rendering(_fp_rt_resolved);

      if (!std::exchange(_fp_rt_resolved, false)) {
         apply_tonemapping("linear"s);
      }
   }

   if (_imgui_active) {
      _config.show_imgui(&_fake_device_loss);
      _color_grading.show_imgui();

      ImGui::Render();
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

   // update per frame state
   _water_refraction = false;

   _device->EndScene();

   const auto result =
      _device->Present(source_rect, dest_rect, dest_window_override, dirty_region);

   _device->BeginScene();

   if (_using_fp_rendertargets) {
      Com_ptr<IDirect3DSurface9> rt;
      _fp_backbuffer->GetSurfaceLevel(0, rt.clear_and_assign());
      _device->SetRenderTarget(0, rt.get());
   }

   if (result != D3D_OK) return result;

   if (_fake_device_loss) return D3DERR_DEVICELOST;

   return D3D_OK;
}

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
   if (type == D3DRTYPE_CUBETEXTURE) {
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
               std::make_shared<Texture>(_device, std::move(d3d_texture), sampler_info);

            _textures.add(name, texture);

            return texture;
         }
         catch (std::exception& e) {
            log(Log_level::error, "Exception occured while loading texture: "sv,
                e.what());

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

   return _device->CreateVolumeTexture(width, height, depth, levels, usage, format,
                                       pool, volume_texture, shared_handle);
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

      if (_using_fp_rendertargets) format = D3DFMT_A16B16G16R16F;
   }

   return _device->CreateTexture(width, height, levels, usage, format, pool,
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

   if (_using_fp_rendertargets) {
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

   if (_linear_rendering) {
      for (auto i = 0; i < 4; ++i) {
         _device->SetSamplerState(i, D3DSAMP_SRGBTEXTURE, _vs_metadata.srgb_state[i]);
      }
   }

   if (_material) {
      _refresh_material = true;

      return S_OK;
   }

   if (_vs_metadata.rendertype == "hdr"sv) {
      _game_doing_bloom_pass = true;
   }
   else if (std::exchange(_game_doing_bloom_pass, false) &&
            _vs_metadata.rendertype != "hdr"sv && _using_fp_rendertargets) {
      _fp_rt_resolved = true;
      set_linear_rendering(false);

      constexpr auto pp_start = constants::ps::post_processing_start;

      _color_grading.bind_constants<pp_start, pp_start + 1>();
      _color_grading.bind_lut(5, 6, 7);

      apply_tonemapping("color grading"s);
   }

   set_active_light_constants(*_device, vertex_shader.metadata.shader_flags);

   return _device->SetVertexShader(vertex_shader.get());
}

HRESULT Device::SetPixelShader(IDirect3DPixelShader9* shader) noexcept
{
   if (_on_ps_shader_set) _on_ps_shader_set();

   if (!shader) {
      _game_pixel_shader = nullptr;
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

      _device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      _device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

      _on_ps_shader_set = [this] {
         _device->SetRenderState(D3DRS_SRCBLEND, _state_block.at(D3DRS_SRCBLEND));
         _device->SetRenderState(D3DRS_DESTBLEND, _state_block.at(D3DRS_DESTBLEND));

         _on_ps_shader_set = nullptr;
      };
   }
   else if (!_water_refraction && metadata.rendertype == "water"sv) {
      if ((metadata.entry_point == "normal_map_distorted_reflection_ps") ||
          (metadata.entry_point ==
           "normal_map_distorted_reflection_specular_ps")) {
         update_refraction_texture();

         _water_refraction = true;
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

      if (_linear_rendering) color = glm::pow(color, glm::vec3{2.2f});

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

HRESULT Device::DrawPrimitive(D3DPRIMITIVETYPE primitive_type,
                              UINT start_vertex, UINT primitive_count) noexcept
{
   refresh_material();

   return _device->DrawPrimitive(primitive_type, start_vertex, primitive_count);
}

HRESULT Device::DrawIndexedPrimitive(D3DPRIMITIVETYPE primitive_type,
                                     INT base_vertex_index,
                                     UINT min_vertex_index, UINT num_vertices,
                                     UINT start_Index, UINT prim_Count) noexcept
{
   refresh_material();

   return _device->DrawIndexedPrimitive(primitive_type, base_vertex_index,
                                        min_vertex_index, num_vertices,
                                        start_Index, prim_Count);
}

void Device::init_sampler_max_anisotropy() noexcept
{
   for (int i = 0; i < 16; ++i) {
      _device->SetSamplerState(i, D3DSAMP_MAXANISOTROPY,
                               std::clamp(_config.rendering.anisotropic_filtering,
                                          1, _device_max_anisotropy));
   }
}

void Device::apply_tonemapping(const std::string& shader_state) noexcept
{
   Com_ptr<IDirect3DSurface9> backbuffer;

   _device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,
                          backbuffer.clear_and_assign());

   _device->SetRenderTarget(0, backbuffer.get());

   Com_ptr<IDirect3DStateBlock9> state;

   _device->CreateStateBlock(D3DSBT_ALL, state.clear_and_assign());

   _device->SetStreamSource(0, _fs_vertex_buffer.get(), 0, get_fs_buffer_stride());
   _device->SetVertexDeclaration(_fs_vertex_decl.get());

   _device->SetRenderState(D3DRS_ZENABLE, FALSE);
   _device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

   _device->SetTexture(4, _fp_backbuffer.get());

   _device->SetSamplerState(4, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   _device->SetSamplerState(4, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   _device->SetSamplerState(4, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
   _device->SetSamplerState(4, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   _device->SetSamplerState(4, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

   _shaders.at("tonemap"s).at(shader_state)[Shader_flags::none].bind(*_device);

   _device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

   state->Apply();
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
   _device->SetTexture(water_slot, _water_texture.get());

   _device->SetSamplerState(water_slot, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
   _device->SetSamplerState(water_slot, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
   _device->SetSamplerState(water_slot, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
   _device->SetSamplerState(water_slot, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
   _device->SetSamplerState(water_slot, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
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

void Device::set_linear_rendering(bool linear_rendering) noexcept
{
   _linear_rendering = linear_rendering;

   if (_linear_rendering) {
      _gamma_vs_const.set(*_device, 2.2f);

      for (auto i = 0; i < 4; ++i) {
         _device->SetSamplerState(i, D3DSAMP_SRGBTEXTURE, _vs_metadata.srgb_state[i]);
      }
   }
   else {
      _gamma_vs_const.set(*_device, 1.0f);

      for (auto i = 0; i < 4; ++i) {
         _device->SetSamplerState(i, D3DSAMP_SRGBTEXTURE, FALSE);
      }
   }

   SetRenderState(D3DRS_FOGCOLOR, _state_block.get(D3DRS_FOGCOLOR));
}

}
