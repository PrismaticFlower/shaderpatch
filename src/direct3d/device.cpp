
#include "device.hpp"
#include "../game_support/memory_hacks.hpp"
#include "../user_config.hpp"
#include "../window_helpers.hpp"
#include "buffer.hpp"
#include "debug_trace.hpp"
#include "helpers.hpp"
#include "pixel_shader.hpp"
#include "query.hpp"
#include "surface_systemmem_dummy.hpp"
#include "texture2d_managed.hpp"
#include "texture2d_rendertarget.hpp"
#include "texture3d_managed.hpp"
#include "texture3d_resource.hpp"
#include "texturecube_managed.hpp"
#include "utility.hpp"
#include "vertex_declaration.hpp"
#include "vertex_shader.hpp"
#include "volume_resource.hpp"

#include <algorithm>
#include <cassert>

using namespace std::literals;

namespace sp::d3d9 {

namespace {

constexpr auto projtex_slot = 2;

auto text_dpi_from_resolution(const double actual_width, const double actual_height,
                              const double perceived_width, const double perceived_height,
                              const double base_dpi) -> std::uint32_t
{
   return static_cast<std::uint32_t>(std::ceil(
      std::max(actual_width / perceived_width, actual_height / perceived_height) *
      base_dpi));
}

}

Com_ptr<Device> Device::create(IDirect3D9& parent, IDXGIAdapter4& adapter,
                               const HWND window, const UINT width,
                               const UINT height) noexcept
{
   return Com_ptr{new Device{parent, adapter, window, width, height}};
}

Device::Device(IDirect3D9& parent, IDXGIAdapter4& adapter, const HWND window,
               const UINT width, const UINT height) noexcept
   : _parent{parent},
     _shader_patch{adapter, window, width, height, width, height},
     _adapter{copy_raw_com_ptr(adapter)},
     _window{window},
     _actual_width{width},
     _actual_height{height},
     _perceived_width{width},
     _perceived_height{height}
{
   win32::make_borderless_window(_window);

   MONITORINFO info{sizeof(MONITORINFO)};
   GetMonitorInfoW(MonitorFromWindow(_window, MONITOR_DEFAULTTOPRIMARY), &info);

   _monitor_width = info.rcMonitor.right - info.rcMonitor.left;
   _monitor_height = info.rcMonitor.bottom - info.rcMonitor.top;

   _parent.AddRef();
}

HRESULT Device::QueryInterface(const IID& iid, void** object) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!object) return E_INVALIDARG;

   if (iid == IID_IUnknown) {
      *object = static_cast<IUnknown*>(this);
   }
   else if (iid == IID_IDirect3DDevice9) {
      *object = this;
   }
   else {
      *object = nullptr;

      return E_NOINTERFACE;
   }

   AddRef();

   return S_OK;
}

ULONG Device::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   _parent.AddRef();

   return _ref_count += 1;
}

ULONG Device::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   _parent.Release();

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

HRESULT Device::TestCooperativeLevel() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return S_OK;
}

UINT Device::GetAvailableTextureMem() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   DXGI_ADAPTER_DESC2 desc{};

   _adapter->GetDesc2(&desc);

   return static_cast<UINT>(std::clamp(desc.DedicatedVideoMemory,
                                       SIZE_T{std::numeric_limits<UINT>::min()},
                                       SIZE_T{std::numeric_limits<UINT>::max()}));
}

HRESULT Device::GetDirect3D(IDirect3D9** d3d9) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!d3d9) return D3DERR_INVALIDCALL;

   _parent.AddRef();

   *d3d9 = &_parent;

   return S_OK;
}

HRESULT Device::GetDeviceCaps(D3DCAPS9* caps) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!caps) return D3DERR_INVALIDCALL;

   *caps = device_caps;

   return S_OK;
}

HRESULT Device::GetDisplayMode(UINT swap_chain, D3DDISPLAYMODE* mode) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (swap_chain != 0) return D3DERR_INVALIDCALL;
   if (!mode) return D3DERR_INVALIDCALL;

   const auto monitor = MonitorFromWindow(_window, MONITOR_DEFAULTTOPRIMARY);
   const auto monitor_info = [&] {
      MONITORINFOEXW info;
      info.cbSize = sizeof(MONITORINFOEXW);
      GetMonitorInfoW(monitor, &info);

      return info;
   }();
   const auto display_mode = [&] {
      DEVMODEW mode;

      EnumDisplaySettingsW(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &mode);

      return mode;
   }();

   mode->Format = D3DFMT_X8R8G8B8;
   mode->Width = display_mode.dmPelsWidth;
   mode->Height = display_mode.dmPelsHeight;
   mode->RefreshRate = display_mode.dmDisplayFrequency;

   return S_OK;
}

HRESULT Device::Reset(D3DPRESENT_PARAMETERS* params) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!params) return D3DERR_INVALIDCALL;

   const UINT base_dpi = 96;
   const UINT dpi = [&] {
      UINT dpi = GetDpiForWindow(_window);

      dpi = dpi * user_config.ui.extra_ui_scaling / 100;

      return dpi;
   }();

   std::uint32_t text_dpi = dpi;

   core::Reset_flags reset_flags{.legacy_fullscreen = !params->Windowed};

   _actual_width = params->BackBufferWidth;
   _actual_height = params->BackBufferHeight;

   if (user_config.display.override_resolution) {
      if (user_config.display.treat_800x600_as_interface &&
          _actual_width == 800 && _actual_height == 600) {
         // Do Nothing!
      }
      else {
         _actual_width = _monitor_width *
                         user_config.display.override_resolution_screen_percent / 100;
         _actual_height = _monitor_height *
                          user_config.display.override_resolution_screen_percent / 100;
      }

      reset_flags.legacy_fullscreen = false;
   }

   _perceived_width = _actual_width;
   _perceived_height = _actual_height;

   UINT window_width = _actual_width;
   UINT window_height = _actual_height;

   if (user_config.display.treat_800x600_as_interface &&
       params->BackBufferWidth == 800 && params->BackBufferHeight == 600) {
      if (!params->Windowed) {
         if (user_config.display.stretch_interface) {
            _actual_width = _monitor_width;
            _actual_height = _monitor_height;
            window_width = _monitor_width;
            window_height = _monitor_height;
         }
         else {
            if (_monitor_width > _monitor_height) {
               _actual_width = _monitor_height * 4 / 3;
               _actual_height = _monitor_height;
            }
            else {
               _actual_width = _monitor_width;
               _actual_height = _monitor_width * 3 / 4;
            }

            window_width = _monitor_width;
            window_height = _monitor_height;
         }

         text_dpi = text_dpi_from_resolution(_actual_width, _actual_height,
                                             _perceived_width,
                                             _perceived_height, base_dpi);

         reset_flags.legacy_fullscreen = false;
      }
      else if (user_config.display.dpi_aware && user_config.display.dpi_scaling) {
         _actual_width = std::min(800 * dpi / base_dpi, _monitor_width);
         _actual_height = std::min(600 * dpi / base_dpi, _monitor_height);

         window_width = _actual_width;
         window_height = _actual_height;
      }
   }
   else if (user_config.display.enable_game_perceived_resolution_override) {
      _perceived_width = user_config.display.game_perceived_resolution_override_width;
      _perceived_height = user_config.display.game_perceived_resolution_override_height;

      text_dpi = text_dpi_from_resolution(_actual_width, _actual_height, _perceived_width,
                                          _perceived_height, base_dpi);
   }
   else if (user_config.display.dpi_aware && user_config.display.dpi_scaling) {
      if (user_config.display.dsr_vsr_scaling && _actual_width > _monitor_width ||
          _actual_height > _monitor_height) { // Scaling for DSR/VSR.
         text_dpi =
            text_dpi_from_resolution(_actual_width, _actual_height,
                                     _monitor_width, _monitor_height, base_dpi);
         text_dpi = text_dpi * user_config.ui.extra_ui_scaling / 100;

         _perceived_width = _actual_width * base_dpi / text_dpi;
         _perceived_height = _actual_height * base_dpi / text_dpi;
      }
      else { // Normal DPI scaling.
         _perceived_width = _actual_width * base_dpi / dpi;
         _perceived_height = _actual_height * base_dpi / dpi;
      }
   }
   else {
      _perceived_width = _actual_width;
      _perceived_height = _actual_height;
      text_dpi = base_dpi;
   }

   // The game's UI layout code starts to struggle with resolutions below 640x480. So we clamp to it here to avoid issues.
   if (_perceived_width < 640u) {
      _perceived_height =
         std::max(_perceived_height, _perceived_height * 640u / _perceived_width);
      _perceived_width = std::max(_perceived_width, 640u);

      text_dpi = text_dpi_from_resolution(_actual_width, _actual_height, _perceived_width,
                                          _perceived_height, base_dpi);
   }

   if (user_config.display.aspect_ratio_hack) {
      switch (user_config.display.aspect_ratio_hack_hud) {
      case Aspect_ratio_hud::centre_4_3:
      case Aspect_ratio_hud::stretch_4_3: {
         if (_perceived_width > _perceived_height) {
            _perceived_width = _perceived_height * 4 / 3;
         }
         else {
            _perceived_height = _perceived_width * 3 / 4;
         }
      } break;
      case Aspect_ratio_hud::centre_16_9:
      case Aspect_ratio_hud::stretch_16_9: {
         if (_perceived_width > _perceived_height) {
            _perceived_width = _perceived_height * 16 / 9;
         }
         else {
            _perceived_height = _perceived_width * 9 / 16;
         }
      } break;
      }

      game_support::set_aspect_ratio_search_ptr(
         std::launder(reinterpret_cast<float*>(params)));
      _shader_patch.set_expected_aspect_ratio(static_cast<float>(_perceived_height) /
                                              static_cast<float>(_perceived_width));

      reset_flags.aspect_ratio_hack = true;
   }

   params->BackBufferWidth = _perceived_width;
   params->BackBufferHeight = _perceived_height;

   if (window_width == _monitor_width && window_height == _monitor_height) {
      reset_flags.legacy_fullscreen = false;
   }

   _render_state_manager.reset();
   _texture_stage_manager.reset();
   _shader_patch.reset(reset_flags, _actual_width, _actual_height, window_width,
                       window_height);
   _shader_patch.set_text_dpi(text_dpi);
   _fixed_func_active = true;

   _backbuffer = Surface_backbuffer::create(_shader_patch.get_back_buffer(),
                                            _perceived_width, _perceived_height);
   _rendertarget = _backbuffer;
   _depthstencil = Surface_depthstencil::create(core::Game_depthstencil::nearscene,
                                                _perceived_width, _perceived_height);

   _viewport = {0, 0, _perceived_width, _perceived_height, 0.0f, 1.0f};

   if (!reset_flags.legacy_fullscreen) {
      win32::position_window(_window, window_width, window_height);
      win32::clip_cursor_to_window(_window);
   }

   return S_OK;
}

HRESULT Device::Present(const RECT*, const RECT*, HWND, const RGNDATA*) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   _shader_patch.present();

   Debug_trace::reset();

   return S_OK;
}

HRESULT Device::GetBackBuffer(UINT swap_chain, UINT back_buffer_index,
                              D3DBACKBUFFER_TYPE type,
                              IDirect3DSurface9** back_buffer) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (swap_chain != 0) return D3DERR_INVALIDCALL;
   if (back_buffer_index != 0) return D3DERR_INVALIDCALL;
   if (type != D3DBACKBUFFER_TYPE_MONO) return D3DERR_INVALIDCALL;

   *back_buffer = static_cast<IDirect3DSurface9*>(_backbuffer.unmanaged_copy());

   return S_OK;
}

HRESULT Device::CreateTexture(UINT width, UINT height, UINT levels, DWORD usage,
                              D3DFORMAT d3d_format, D3DPOOL pool,
                              IDirect3DTexture9** texture, HANDLE*) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!texture) return D3DERR_INVALIDCALL;

   if (usage & D3DUSAGE_DYNAMIC) {
      log_and_terminate("Attempt to create dynamic texture.");
   }

   if (pool == D3DPOOL_SYSTEMMEM || pool == D3DPOOL_SCRATCH) {
      log_and_terminate("Attempt to create texture in unexpected memory pool.");
   }

   if (usage & D3DUSAGE_RENDERTARGET) {
      *texture = create_texture2d_rendertarget(width, height).release();
   }
   else {
      *texture =
         create_texture2d_managed(width, height, levels, d3d_format).release();
   }

   return S_OK;
}

HRESULT Device::CreateVolumeTexture(UINT width, UINT height, UINT depth, UINT levels,
                                    DWORD usage, D3DFORMAT format, D3DPOOL pool,
                                    IDirect3DVolumeTexture9** volume_texture,
                                    HANDLE*) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!volume_texture) return D3DERR_INVALIDCALL;

   if (usage & D3DUSAGE_DYNAMIC) {
      log_and_terminate("Attempt to create dynamic volume texture.");
   }

   if (pool == D3DPOOL_SYSTEMMEM || pool == D3DPOOL_SCRATCH) {
      log_and_terminate(
         "Attempt to create volume texture in unexpected memory pool.");
   }

   if (format == volume_resource_format) {
      *volume_texture = reinterpret_cast<IDirect3DVolumeTexture9*>(
         Texture3d_resource::create(_shader_patch, width, height, depth).release());

      return S_OK;
   }

   *volume_texture =
      create_texture3d_managed(width, height, depth, levels, format).release();

   return S_OK;
}

HRESULT Device::CreateCubeTexture(UINT edge_length, UINT levels, DWORD usage,
                                  D3DFORMAT format, D3DPOOL pool,
                                  IDirect3DCubeTexture9** cube_texture, HANDLE*) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!cube_texture) return D3DERR_INVALIDCALL;

   if (usage & D3DUSAGE_DYNAMIC) {
      log_and_terminate("Attempt to create dynamic cube texture.");
   }
   else if (usage & D3DUSAGE_RENDERTARGET) {
      log_and_terminate("Attempt to create rendertarget cube texture.");
   }

   if (pool == D3DPOOL_SYSTEMMEM || pool == D3DPOOL_SCRATCH) {
      log_and_terminate(
         "Attempt to create volume texture in unexpected memory pool.");
   }

   *cube_texture = create_texturecube_managed(edge_length, levels, format).release();

   return S_OK;
}

HRESULT Device::CreateVertexBuffer(UINT length, DWORD usage, DWORD fvf, D3DPOOL pool,
                                   IDirect3DVertexBuffer9** vertex_buffer, HANDLE*) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (length == 0) return D3DERR_INVALIDCALL;
   if (!vertex_buffer) return D3DERR_INVALIDCALL;

   if (fvf) {
      log_and_terminate("Attempt to create FVF vertex buffer.");
   }

   if (pool == D3DPOOL_SYSTEMMEM || pool == D3DPOOL_SCRATCH) {
      log_and_terminate(
         "Attempt to create vertex buffer in unexpected memory pool.");
   }

   *vertex_buffer = create_vertex_buffer(length, usage & D3DUSAGE_DYNAMIC).release();

   return S_OK;
}

HRESULT Device::CreateIndexBuffer(UINT length, DWORD usage, D3DFORMAT format,
                                  D3DPOOL pool,
                                  IDirect3DIndexBuffer9** index_buffer, HANDLE*) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (length == 0) return D3DERR_INVALIDCALL;
   if (!index_buffer) return D3DERR_INVALIDCALL;

   if (format != D3DFMT_INDEX16) {
      log_and_terminate(
         "Attempt to create index buffer with unexpected format.");
   }

   if (pool == D3DPOOL_SYSTEMMEM || pool == D3DPOOL_SCRATCH) {
      log_and_terminate(
         "Attempt to create index buffer in unexpected memory pool.");
   }

   *index_buffer = create_index_buffer(length, usage & D3DUSAGE_DYNAMIC).release();

   return S_OK;
}

HRESULT Device::CreateDepthStencilSurface(UINT width, UINT height, D3DFORMAT,
                                          D3DMULTISAMPLE_TYPE, DWORD, BOOL,
                                          IDirect3DSurface9** surface, HANDLE*) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!surface) return D3DERR_INVALIDCALL;

   const auto surface_name = (width == 512 && height == 256)
                                ? core::Game_depthstencil::reflectionscene
                                : core::Game_depthstencil::farscene;

   *surface = reinterpret_cast<IDirect3DSurface9*>(
      Surface_depthstencil::create(surface_name, width, height).release());

   return S_OK;
}

HRESULT Device::GetRenderTargetData(IDirect3DSurface9*, IDirect3DSurface9*) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   log(Log_level::warning,
       "Attempt to GetRenderTargetData has been called and is unimplemented, any screenshots you take using the game's hotkey will likely be blank."sv);

   return S_OK;
}

HRESULT Device::StretchRect(IDirect3DSurface9* source_surface,
                            const RECT* source_rect, IDirect3DSurface9* dest_surface,
                            const RECT* dest_rect, D3DTEXTUREFILTERTYPE) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!source_surface || !dest_surface) return D3DERR_INVALIDCALL;
   if (source_surface == dest_surface) return D3DERR_INVALIDCALL;

   D3DSURFACE_DESC src_desc{};
   source_surface->GetDesc(&src_desc);

   D3DSURFACE_DESC dest_desc{};
   dest_surface->GetDesc(&dest_desc);

   const auto rect_in_surface = [](RECT rect, D3DSURFACE_DESC desc) -> bool {
      if ((rect.right - rect.left) > desc.Width) return false;
      if ((rect.bottom - rect.top) > desc.Height) return false;

      return true;
   };

   if (source_rect && !rect_in_surface(*source_rect, src_desc)) {
      return D3DERR_INVALIDCALL;
   }
   if (dest_rect && !rect_in_surface(*dest_rect, dest_desc)) {
      return D3DERR_INVALIDCALL;
   }

   const auto* const source_id =
      reinterpret_cast<Resource*>(source_surface)->get_if<core::Game_rendertarget_id>();

   const auto* const dest_id =
      reinterpret_cast<Resource*>(dest_surface)->get_if<core::Game_rendertarget_id>();

   if (!source_id || !dest_id) return D3DERR_INVALIDCALL;

   _shader_patch.stretch_rendertarget(
      *source_id,
      source_rect
         ? core::make_normalized_rect(*source_rect, src_desc.Width, src_desc.Height)
         : core::Normalized_rect{0.0, 0.0, 1.0, 1.0},
      *dest_id,
      dest_rect
         ? core::make_normalized_rect(*dest_rect, dest_desc.Width, dest_desc.Height)
         : core::Normalized_rect{0.0, 0.0, 1.0, 1.0});

   return S_OK;
}

HRESULT Device::ColorFill(IDirect3DSurface9* surface, const RECT* rect,
                          D3DCOLOR color) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!surface) return D3DERR_INVALIDCALL;

   const auto* const rendertarget_id =
      reinterpret_cast<Resource*>(surface)->get_if<core::Game_rendertarget_id>();

   if (!rendertarget_id) return D3DERR_INVALIDCALL;

   D3DSURFACE_DESC surface_desc{};
   surface->GetDesc(&surface_desc);

   auto normalized_rect =
      rect ? core::make_normalized_rect(*rect, surface_desc.Width, surface_desc.Height)
           : core::Normalized_rect{};

   _shader_patch.color_fill_rendertarget(*rendertarget_id, unpack_d3dcolor(color),
                                         rect ? &normalized_rect : nullptr);

   return S_OK;
}

HRESULT Device::CreateOffscreenPlainSurface(UINT width, UINT height,
                                            D3DFORMAT format, D3DPOOL pool,
                                            IDirect3DSurface9** surface, HANDLE*) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!surface) return D3DERR_INVALIDCALL;

   if (pool != D3DPOOL_SYSTEMMEM) {
      log_and_terminate("Attempt to create offscreen plain surface in "
                        "unexpected memory pool.");
   }

   if (format != Surface_systemmem_dummy::dummy_format) {
      log_and_terminate("Attempt to create offscreen plain surface with "
                        "unexpected format.");
   }

   *surface = reinterpret_cast<IDirect3DSurface9*>(
      Surface_systemmem_dummy::create(width, height).release());

   return S_OK;
}

HRESULT Device::SetRenderTarget(DWORD rendertarget_index,
                                IDirect3DSurface9* rendertarget) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (rendertarget_index != 0) return D3DERR_NOTFOUND;
   if (!rendertarget) return D3DERR_INVALIDCALL;

   const auto* const rendertarget_id =
      reinterpret_cast<Resource*>(rendertarget)->get_if<core::Game_rendertarget_id>();

   if (!rendertarget_id) return D3DERR_INVALIDCALL;

   // update viewport
   {
      D3DSURFACE_DESC desc{};

      rendertarget->GetDesc(&desc);

      _viewport = {0, 0, desc.Width, desc.Height, 0.0f, 1.0f};
   }

   _shader_patch.set_rendertarget(*rendertarget_id);

   rendertarget->AddRef();
   _rendertarget = Com_ptr{static_cast<IUnknown*>(rendertarget)};

   return S_OK;
}

HRESULT Device::GetRenderTarget(DWORD rendertarget_index,
                                IDirect3DSurface9** rendertarget) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (rendertarget_index != 0) return D3DERR_NOTFOUND;
   if (!rendertarget) return D3DERR_INVALIDCALL;

   *rendertarget = static_cast<IDirect3DSurface9*>(_rendertarget.unmanaged_copy());

   return S_OK;
}

HRESULT Device::SetDepthStencilSurface(IDirect3DSurface9* new_z_stencil) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!new_z_stencil) {
      _shader_patch.set_depthstencil(core::Game_depthstencil::none);
      _depthstencil = nullptr;

      return S_OK;
   }

   const auto* const depthstencil =
      reinterpret_cast<Resource*>(new_z_stencil)->get_if<core::Game_depthstencil>();

   if (!depthstencil) return D3DERR_INVALIDCALL;

   _shader_patch.set_depthstencil(*depthstencil);

   new_z_stencil->AddRef();
   _depthstencil = Com_ptr{static_cast<IUnknown*>(new_z_stencil)};

   return S_OK;
}

HRESULT Device::GetDepthStencilSurface(IDirect3DSurface9** z_stencil_surface) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!z_stencil_surface) return D3DERR_INVALIDCALL;

   if (_depthstencil) {
      *z_stencil_surface =
         static_cast<IDirect3DSurface9*>(_depthstencil.unmanaged_copy());
   }
   else {
      *z_stencil_surface = nullptr;
   }

   return S_OK;
}

HRESULT Device::Clear(DWORD count, const D3DRECT* rects, DWORD flags,
                      D3DCOLOR color, float z, DWORD stencil) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (count != 0 || rects) {
      log_and_terminate(
         "Attempt to only partially clear rendertarget/depthstencil.");
   }

   if (flags & D3DCLEAR_ZBUFFER || flags & D3DCLEAR_STENCIL) {
      _shader_patch.clear_depthstencil(z, static_cast<UINT8>(stencil),
                                       (flags & (D3DCLEAR_ZBUFFER)) != 0,
                                       (flags & (D3DCLEAR_STENCIL)) != 0);
   }

   if (flags & D3DCLEAR_TARGET) {
      _shader_patch.clear_rendertarget(unpack_d3dcolor(color));
   }

   return S_OK;
}

HRESULT Device::BeginScene() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return S_OK;
}

HRESULT Device::EndScene() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return S_OK;
}

HRESULT Device::SetTransform(D3DTRANSFORMSTATETYPE type, const D3DMATRIX* matrix) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (type == D3DTS_PROJECTION) {
      _shader_patch.set_informal_projection_matrix(bit_cast<glm::mat4>(*matrix));
   }

   return S_OK;
}

HRESULT Device::GetTransform(D3DTRANSFORMSTATETYPE, D3DMATRIX* matrix) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!matrix) return D3DERR_INVALIDCALL;

   *matrix = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
              0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};

   return S_OK;
}

HRESULT Device::SetViewport(const D3DVIEWPORT9* viewport) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!viewport) return D3DERR_INVALIDCALL;

   _viewport = *viewport;

   return S_OK;
}

HRESULT Device::GetViewport(D3DVIEWPORT9* viewport) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!viewport) return D3DERR_INVALIDCALL;

   *viewport = _viewport;

   return S_OK;
}

HRESULT Device::SetRenderState(D3DRENDERSTATETYPE state, DWORD value) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   _render_state_manager.set(state, value);

   return S_OK;
}

HRESULT Device::GetRenderState(D3DRENDERSTATETYPE state, DWORD* value) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!value) return D3DERR_INVALIDCALL;

   const auto float_zero = 0.0f;
   const auto float_one = 1.0f;

   const static std::unordered_map<D3DRENDERSTATETYPE, DWORD> values = {
      {D3DRS_ZENABLE, D3DZB_TRUE},
      {D3DRS_FILLMODE, D3DFILL_SOLID},
      {D3DRS_SHADEMODE, D3DSHADE_GOURAUD},
      {D3DRS_ZWRITEENABLE, TRUE},
      {D3DRS_ALPHATESTENABLE, FALSE},
      {D3DRS_LASTPIXEL, TRUE},
      {D3DRS_SRCBLEND, D3DBLEND_ONE},
      {D3DRS_DESTBLEND, D3DBLEND_ZERO},
      {D3DRS_CULLMODE, D3DCULL_CCW},
      {D3DRS_ZFUNC, D3DCMP_LESSEQUAL},
      {D3DRS_ALPHAREF, 0},
      {D3DRS_ALPHAFUNC, D3DCMP_ALWAYS},
      {D3DRS_DITHERENABLE, FALSE},
      {D3DRS_ALPHABLENDENABLE, FALSE},
      {D3DRS_FOGENABLE, FALSE},
      {D3DRS_SPECULARENABLE, FALSE},
      {D3DRS_FOGCOLOR, 0},
      {D3DRS_FOGTABLEMODE, D3DFOG_NONE},
      {D3DRS_FOGSTART, reinterpret_cast<const DWORD&>(float_zero)},
      {D3DRS_FOGEND, reinterpret_cast<const DWORD&>(float_zero)},
      {D3DRS_FOGDENSITY, reinterpret_cast<const DWORD&>(float_one)},
      {D3DRS_RANGEFOGENABLE, FALSE},
      {D3DRS_STENCILENABLE, FALSE},
      {D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP},
      {D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP},
      {D3DRS_STENCILPASS, D3DSTENCILOP_KEEP},
      {D3DRS_STENCILFUNC, D3DCMP_ALWAYS},
      {D3DRS_STENCILREF, 0},
      {D3DRS_STENCILMASK, 0xffffffff},
      {D3DRS_STENCILWRITEMASK, 0xffffffff},
      {D3DRS_TEXTUREFACTOR, 0xffffffff},
      {D3DRS_WRAP0, 0},
      {D3DRS_WRAP1, 0},
      {D3DRS_WRAP2, 0},
      {D3DRS_WRAP3, 0},
      {D3DRS_WRAP4, 0},
      {D3DRS_WRAP5, 0},
      {D3DRS_WRAP6, 0},
      {D3DRS_WRAP7, 0},
      {D3DRS_CLIPPING, TRUE},
      {D3DRS_LIGHTING, TRUE},
      {D3DRS_AMBIENT, 0},
      {D3DRS_FOGVERTEXMODE, D3DFOG_NONE},
      {D3DRS_COLORVERTEX, TRUE},
      {D3DRS_LOCALVIEWER, TRUE},
      {D3DRS_NORMALIZENORMALS, FALSE},
      {D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1},
      {
         D3DRS_SPECULARMATERIALSOURCE,
         D3DMCS_COLOR2,
      },
      {D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL},
      {D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL},
      {D3DRS_VERTEXBLEND, D3DVBF_DISABLE},
      {D3DRS_CLIPPLANEENABLE, 0},
      {D3DRS_POINTSIZE, reinterpret_cast<const DWORD&>(float_one)},
      {D3DRS_POINTSIZE_MIN, reinterpret_cast<const DWORD&>(float_one)},
      {D3DRS_POINTSPRITEENABLE, FALSE},
      {D3DRS_POINTSCALEENABLE, FALSE},
      {D3DRS_POINTSCALE_A, reinterpret_cast<const DWORD&>(float_one)},
      {D3DRS_POINTSCALE_B, reinterpret_cast<const DWORD&>(float_one)},
      {D3DRS_POINTSCALE_C, reinterpret_cast<const DWORD&>(float_one)},
      {D3DRS_MULTISAMPLEANTIALIAS, TRUE},
      {D3DRS_MULTISAMPLEMASK, 0xffffffff},
      {D3DRS_PATCHEDGESTYLE, D3DPATCHEDGE_DISCRETE},
      {D3DRS_DEBUGMONITORTOKEN, D3DDMT_DISABLE},
      {D3DRS_POINTSIZE_MAX, reinterpret_cast<const DWORD&>(float_one)},
      {D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE},
      {D3DRS_COLORWRITEENABLE, 0xf},
      {D3DRS_TWEENFACTOR, reinterpret_cast<const DWORD&>(float_zero)},
      {D3DRS_BLENDOP, D3DBLENDOP_ADD},
      {D3DRS_POSITIONDEGREE, D3DDEGREE_CUBIC},
      {D3DRS_NORMALDEGREE, D3DDEGREE_LINEAR},
      {D3DRS_SCISSORTESTENABLE, FALSE},
      {D3DRS_SLOPESCALEDEPTHBIAS, 0},
      {D3DRS_ANTIALIASEDLINEENABLE, FALSE},
      {D3DRS_MINTESSELLATIONLEVEL, reinterpret_cast<const DWORD&>(float_one)},
      {D3DRS_MAXTESSELLATIONLEVEL, reinterpret_cast<const DWORD&>(float_one)},
      {D3DRS_ADAPTIVETESS_X, reinterpret_cast<const DWORD&>(float_zero)},
      {D3DRS_ADAPTIVETESS_Y, reinterpret_cast<const DWORD&>(float_zero)},
      {D3DRS_ADAPTIVETESS_Z, reinterpret_cast<const DWORD&>(float_zero)},
      {D3DRS_ADAPTIVETESS_W, reinterpret_cast<const DWORD&>(float_zero)},
      {D3DRS_ENABLEADAPTIVETESSELLATION, FALSE},
      {D3DRS_TWOSIDEDSTENCILMODE, FALSE},
      {D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_KEEP},
      {D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_KEEP},
      {D3DRS_CCW_STENCILPASS, D3DSTENCILOP_KEEP},
      {D3DRS_CCW_STENCILFUNC, D3DCMP_ALWAYS},
      {D3DRS_COLORWRITEENABLE1, 0xf},
      {D3DRS_COLORWRITEENABLE2, 0xf},
      {D3DRS_COLORWRITEENABLE3, 0xf},
      {D3DRS_BLENDFACTOR, 0xffffffff},
      {D3DRS_SRGBWRITEENABLE, FALSE},
      {D3DRS_DEPTHBIAS, reinterpret_cast<const DWORD&>(float_zero)},
      {D3DRS_WRAP8, 0},
      {D3DRS_WRAP9, 0},
      {D3DRS_WRAP10, 0},
      {D3DRS_WRAP11, 0},
      {D3DRS_WRAP12, 0},
      {D3DRS_WRAP13, 0},
      {D3DRS_WRAP14, 0},
      {D3DRS_WRAP15, 0},
      {D3DRS_SEPARATEALPHABLENDENABLE, FALSE},
      {D3DRS_SRCBLENDALPHA, D3DBLEND_ONE},
      {D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO},
      {D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD},
   };

   if (const auto value_it = values.find(state); value_it != values.cend()) {
      *value = value_it->second;
      return S_OK;
   }

   return D3DERR_INVALIDCALL;
}

HRESULT Device::GetTexture(DWORD stage, IDirect3DBaseTexture9** texture) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (stage >= 4) return D3DERR_INVALIDCALL;
   if (!texture) return D3DERR_INVALIDCALL;

   *texture = nullptr;

   return S_OK;
}

HRESULT Device::SetTexture(DWORD stage, IDirect3DBaseTexture9* texture) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (stage >= 4) return D3DERR_INVALIDCALL;

   if (!texture) {
      if (stage == 0) _shader_patch.set_patch_material(nullptr);

      _shader_patch.set_texture(stage, core::nullgametex);

      return S_OK;
   }

   const auto& base_texture = *reinterpret_cast<Base_texture*>(texture);

   if (stage == projtex_slot) {
      if (base_texture.texture_type == Texture_type::texturecube) {
         _shader_patch.set_projtex_type(core::Projtex_type::texcube);

         base_texture.visit([&](const core::Game_texture& texture) {
            _shader_patch.set_projtex_cube(texture);
         });
      }
      else {
         _shader_patch.set_projtex_type(core::Projtex_type::tex2d);
      }
   }

   base_texture.visit(
      [&](const core::Game_texture& texture) {
         if (stage == 0) _shader_patch.set_patch_material(nullptr);

         _shader_patch.set_texture(stage, texture);
      },
      [&](const core::Game_rendertarget_id& rendertarget) {
         if (stage == 0) _shader_patch.set_patch_material(nullptr);

         _shader_patch.set_texture(stage, rendertarget);
      },
      [&](const core::Material_handle& material) {
         if (stage != 0) return;

         _shader_patch.set_patch_material(material.get());
      });

   return S_OK;
}

HRESULT Device::GetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE state,
                                     DWORD* value) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (stage >= 4) return D3DERR_INVALIDCALL;
   if (!value) return D3DERR_INVALIDCALL;

   *value = _texture_stage_manager.get(stage, state);

   return S_OK;
}

HRESULT Device::SetTextureStageState(DWORD stage, D3DTEXTURESTAGESTATETYPE state,
                                     DWORD value) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   _texture_stage_manager.set(stage, state, value);

   return S_OK;
}

HRESULT Device::GetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE state,
                                DWORD* value) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (sampler >= 4) return D3DERR_INVALIDCALL;
   if (!value) return D3DERR_INVALIDCALL;

   switch (state) {
   case D3DSAMP_ADDRESSU:
      *value = D3DTADDRESS_WRAP;
      return S_OK;
   case D3DSAMP_ADDRESSV:
      *value = D3DTADDRESS_WRAP;
      return S_OK;
   case D3DSAMP_ADDRESSW:
      *value = D3DTADDRESS_WRAP;
      return S_OK;
   case D3DSAMP_BORDERCOLOR:
      *value = 0x00000000;
      return S_OK;
   case D3DSAMP_MAGFILTER:
      *value = D3DTEXF_POINT;
      return S_OK;
   case D3DSAMP_MINFILTER:
      *value = D3DTEXF_POINT;
      return S_OK;
   case D3DSAMP_MIPFILTER:
      *value = D3DTEXF_NONE;
      return S_OK;
   case D3DSAMP_MIPMAPLODBIAS:
      *value = bit_cast<DWORD>(0.0f);
      return S_OK;
   case D3DSAMP_MAXMIPLEVEL:
      *value = 0;
      return S_OK;
   case D3DSAMP_MAXANISOTROPY:
      *value = 16;
      return S_OK;
   case D3DSAMP_SRGBTEXTURE:
      *value = false;
      return S_OK;
   case D3DSAMP_ELEMENTINDEX:
      *value = 0;
      return S_OK;
   case D3DSAMP_DMAPOFFSET:
      *value = 0;
      return S_OK;
   }

   return D3DERR_INVALIDCALL;
}

HRESULT Device::SetSamplerState(DWORD sampler, D3DSAMPLERSTATETYPE state, DWORD value) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (sampler >= 4) return D3DERR_INVALIDCALL;

   if (sampler == projtex_slot && state == D3DSAMP_ADDRESSU) {
      _shader_patch.set_projtex_mode(value == D3DTADDRESS_WRAP
                                        ? core::Projtex_mode::wrap
                                        : core::Projtex_mode::clamp);
   }

   return S_OK;
}

HRESULT Device::DrawPrimitive(D3DPRIMITIVETYPE primitive_type,
                              UINT start_vertex, UINT primitive_count) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   draw_common();

   const auto vertex_count =
      d3d_primitive_count_to_vertex_count(primitive_type, primitive_count);

   // Emulate a triangle fan for underwater overlay quad. As far as I know this
   // is the only time that the game uses triangle fans, unless some jerk modder
   // sets the primitive topology in a `modl` `segm` to triangle fans.
   if (primitive_type == D3DPT_TRIANGLEFAN) {
      SetIndices(static_cast<IDirect3DIndexBuffer9*>(_triangle_fan_quad_ibuf.get()));
      _shader_patch.draw_indexed(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 6, 0,
                                 start_vertex);
   }
   else {
      _shader_patch.draw(d3d_primitive_type_to_d3d11_topology(primitive_type),
                         vertex_count, start_vertex);
   }

   return S_OK;
}

HRESULT Device::DrawIndexedPrimitive(D3DPRIMITIVETYPE primitive_type,
                                     INT base_vertex_index, UINT, UINT,
                                     UINT start_index, UINT primitive_count) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   draw_common();

   const auto vertex_count =
      d3d_primitive_count_to_vertex_count(primitive_type, primitive_count);

   _shader_patch.draw_indexed(d3d_primitive_type_to_d3d11_topology(primitive_type),
                              vertex_count, start_index, base_vertex_index);

   return S_OK;
}

HRESULT Device::CreateVertexDeclaration(const D3DVERTEXELEMENT9* vertex_elements,
                                        IDirect3DVertexDeclaration9** decl) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!vertex_elements) return D3DERR_INVALIDCALL;
   if (!decl) return D3DERR_INVALIDCALL;

   *decl = create_vertex_declaration(vertex_elements).release();

   return S_OK;
}

HRESULT Device::SetVertexDeclaration(IDirect3DVertexDeclaration9* decl) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!decl) return D3DERR_INVALIDCALL;

   _shader_patch.set_input_layout(
      static_cast<Vertex_declaration*>(decl)->input_layout());

   return S_OK;
}

HRESULT Device::SetFVF(DWORD) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return S_OK;
}

HRESULT Device::GetFVF(DWORD* fvf) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!fvf) return D3DERR_INVALIDCALL;

   *fvf = 0;

   return S_OK;
}

HRESULT Device::CreateVertexShader(const DWORD* function,
                                   IDirect3DVertexShader9** shader) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!function) return D3DERR_INVALIDCALL;
   if (!shader) return D3DERR_INVALIDCALL;

   *shader = Vertex_shader::create(std::uint32_t{*function}).release();

   return S_OK;
}

HRESULT Device::SetVertexShader(IDirect3DVertexShader9* shader) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!shader) {
      _fixed_func_active = true;
      return S_OK;
   }

   _shader_patch.set_game_shader(static_cast<Vertex_shader*>(shader)->game_shader());
   _fixed_func_active = false;

   return S_OK;
}

HRESULT Device::SetVertexShaderConstantF(UINT start_register, const float* constant_data,
                                         UINT vector4f_count) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (start_register < 2) return S_OK;

   if (const std::span constants{reinterpret_cast<const std::array<float, 4>*>(constant_data),
                                 vector4f_count};
       start_register < 12) {
      const auto start = start_register - 2u;
      const auto count = safe_min(vector4f_count, core::cb::scene_game_count - start);

      _shader_patch.set_constants(core::cb::scene, start, constants.subspan(0, count));
   }
   else if (start_register < 51) {
      const auto start = start_register - 12u;
      const auto count = safe_min(vector4f_count, core::cb::draw_game_count - start);

      _shader_patch.set_constants(core::cb::draw, start, constants.subspan(0, count));
   }
   else {
      const auto start = start_register - 51u;
      const auto count = safe_min(vector4f_count, core::cb::skin_game_count - start);

      _shader_patch.set_constants(core::cb::skin, start, constants.subspan(0, count));
   }

   return S_OK;
}

HRESULT Device::SetStreamSource(UINT stream_number, IDirect3DVertexBuffer9* stream_data,
                                UINT offset_in_bytes, UINT stride) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (stream_number != 0) return D3DERR_INVALIDCALL;
   if (!stream_data) return D3DERR_INVALIDCALL;

   const auto& resource = *reinterpret_cast<Resource*>(stream_data);

   resource.visit([&](ID3D11Buffer* buffer) {
      _shader_patch.set_vertex_buffer(*buffer, offset_in_bytes, stride);
   });

   return S_OK;
}

HRESULT Device::SetIndices(IDirect3DIndexBuffer9* index_data) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!index_data) return D3DERR_INVALIDCALL;

   const auto& resource = *reinterpret_cast<Resource*>(index_data);

   resource.visit(
      [&](ID3D11Buffer* buffer) { _shader_patch.set_index_buffer(*buffer, 0); });

   return S_OK;
}

HRESULT Device::CreatePixelShader(const DWORD* function,
                                  IDirect3DPixelShader9** shader) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!function) return D3DERR_INVALIDCALL;
   if (!shader) return D3DERR_INVALIDCALL;

   *shader = Pixel_shader::create().release();

   return S_OK;
}

HRESULT Device::SetPixelShader(IDirect3DPixelShader9*) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return S_OK;
}

HRESULT Device::SetPixelShaderConstantF(UINT start_register, const float* constant_data,
                                        UINT vector4f_count) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto count =
      safe_min(vector4f_count, core::cb::draw_ps_game_count - start_register);
   const auto constants =
      std::span{reinterpret_cast<const std::array<float, 4>*>(constant_data), count};

   _shader_patch.set_constants(core::cb::draw_ps, start_register, constants);

   return S_OK;
}

HRESULT Device::CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9** query) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   switch (type) {
   case D3DQUERYTYPE_EVENT:
   case D3DQUERYTYPE_OCCLUSION:
      if (!query) return S_OK;

      *query = make_query(_shader_patch, type).release();

      return S_OK;
   case D3DQUERYTYPE_VCACHE:
   case D3DQUERYTYPE_RESOURCEMANAGER:
   case D3DQUERYTYPE_VERTEXSTATS:
   case D3DQUERYTYPE_TIMESTAMP:
   case D3DQUERYTYPE_TIMESTAMPDISJOINT:
   case D3DQUERYTYPE_TIMESTAMPFREQ:
   case D3DQUERYTYPE_PIPELINETIMINGS:
   case D3DQUERYTYPE_INTERFACETIMINGS:
   case D3DQUERYTYPE_VERTEXTIMINGS:
   case D3DQUERYTYPE_PIXELTIMINGS:
   case D3DQUERYTYPE_BANDWIDTHTIMINGS:
   case D3DQUERYTYPE_CACHEUTILIZATION:
   case D3DQUERYTYPE_MEMORYPRESSURE:
      log(Log_level::warning,
          "Unsupported query type requested: ", static_cast<int>(type));
      return D3DERR_NOTAVAILABLE;
   }

   return D3DERR_INVALIDCALL;
}

auto Device::create_texture2d_managed(const UINT width, const UINT height,
                                      const UINT mip_levels,
                                      const D3DFORMAT d3d_format) noexcept
   -> Com_ptr<IDirect3DTexture9>
{
   const auto format = d3d_to_dxgi_format(d3d_format);

   if (format == DXGI_FORMAT_UNKNOWN) {
      log_and_terminate("Attempt to create texture using unsupported format: ",
                        static_cast<int>(d3d_format));
   }

   std::unique_ptr<Format_patcher> format_patcher = nullptr;

   if (d3d_format == D3DFMT_L8) {
      format_patcher = make_l8_format_patcher();
   }
   else if (d3d_format == D3DFMT_A8L8) {
      format_patcher = make_a8l8_format_patcher();
   }

   return Com_ptr{reinterpret_cast<IDirect3DTexture9*>(
      Texture2d_managed::create(_shader_patch, width, height, mip_levels,
                                format, d3d_format, std::move(format_patcher))
         .release())};
}

auto Device::create_texture2d_rendertarget(const UINT width, const UINT height) noexcept
   -> Com_ptr<IDirect3DTexture9>
{
   const UINT texture_actual_width = width == _perceived_width ? _actual_width : width;
   const UINT texture_actual_height =
      height == _perceived_height ? _actual_height : height;

   return Com_ptr{reinterpret_cast<IDirect3DTexture9*>(
      Texture2d_rendertarget::create(_shader_patch, texture_actual_width,
                                     texture_actual_height, width, height)
         .release())};
}

auto Device::create_texture3d_managed(const UINT width, const UINT height,
                                      const UINT depth, const UINT mip_levels,
                                      const D3DFORMAT d3d_format) noexcept
   -> Com_ptr<IDirect3DVolumeTexture9>
{
   const auto format = d3d_to_dxgi_format(d3d_format);

   if (format == DXGI_FORMAT_UNKNOWN) {
      log_and_terminate("Attempt to create texture using unsupported format: ",
                        static_cast<int>(d3d_format));
   }

   return Com_ptr{reinterpret_cast<IDirect3DVolumeTexture9*>(
      Texture3d_managed::create(_shader_patch, width, height, depth, mip_levels,
                                format, d3d_format)
         .release())};
}

auto Device::create_texturecube_managed(const UINT width, const UINT mip_levels,
                                        const D3DFORMAT d3d_format) noexcept
   -> Com_ptr<IDirect3DCubeTexture9>
{
   const auto format = d3d_to_dxgi_format(d3d_format);

   if (format == DXGI_FORMAT_UNKNOWN) {
      log_and_terminate("Attempt to create texture using unsupported format: ",
                        static_cast<int>(d3d_format));
   }

   std::unique_ptr<Format_patcher> format_patcher = nullptr;

   if (d3d_format == D3DFMT_L8) {
      format_patcher = make_l8_format_patcher();
   }
   else if (d3d_format == D3DFMT_A8L8) {
      format_patcher = make_a8l8_format_patcher();
   }

   return Com_ptr{reinterpret_cast<IDirect3DCubeTexture9*>(
      Texturecube_managed::create(_shader_patch, width, mip_levels, format,
                                  d3d_format, std::move(format_patcher))
         .release())};
}

auto Device::create_vertex_buffer(const UINT size, const bool dynamic) noexcept
   -> Com_ptr<IDirect3DVertexBuffer9>
{
   if (dynamic) {
      return Com_ptr{reinterpret_cast<IDirect3DVertexBuffer9*>(
         Vertex_buffer_dynamic::create(_shader_patch, size, false).release())};
   }
   else {
      return Com_ptr{reinterpret_cast<IDirect3DVertexBuffer9*>(
         Vertex_buffer_managed::create(_shader_patch, size).release())};
   }
}

auto Device::create_index_buffer(const UINT size, const bool dynamic) noexcept
   -> Com_ptr<IDirect3DIndexBuffer9>
{
   if (dynamic) {
      return Com_ptr{reinterpret_cast<IDirect3DIndexBuffer9*>(
         Index_buffer_dynamic::create(_shader_patch, size, false).release())};
   }
   else {
      return Com_ptr{reinterpret_cast<IDirect3DIndexBuffer9*>(
         Index_buffer_managed::create(_shader_patch, size).release())};
   }
}

auto Device::create_vertex_declaration(const D3DVERTEXELEMENT9* const vertex_elements) noexcept
   -> Com_ptr<IDirect3DVertexDeclaration9>
{
   constexpr D3DVERTEXELEMENT9 decl_end D3DDECL_END();

   std::size_t decl_count = 0;
   for (; std::memcmp(&vertex_elements[decl_count], &decl_end,
                      sizeof(D3DVERTEXELEMENT9)) != 0;
        ++decl_count) {
      if (decl_count > 0xff) {
         log_and_terminate("Runaway vertex declaration!");
      }
   }

   return Vertex_declaration::create(_shader_patch,
                                     std::span{vertex_elements, decl_count});
}

void Device::draw_common() noexcept
{
   if (_fixed_func_active) {
      _texture_stage_manager.update(_shader_patch,
                                    _render_state_manager.texture_factor(), _viewport);
   }

   _render_state_manager.update_dirty(_shader_patch);
}
}
