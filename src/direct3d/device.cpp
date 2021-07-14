
#include "device.hpp"
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
#include <bit>
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
     _shader_patch{adapter, window, width, height},
     _adapter{copy_raw_com_ptr(adapter)},
     _window{window},
     _actual_width{width},
     _actual_height{height},
     _perceived_width{width},
     _perceived_height{height}
{
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

   const auto monitor = MonitorFromWindow(_window, MONITOR_DEFAULTTONEAREST);
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

   MONITORINFO info{sizeof(MONITORINFO)};
   GetMonitorInfoW(MonitorFromWindow(_window, MONITOR_DEFAULTTONEAREST), &info);

   const UINT monitor_width = info.rcMonitor.right - info.rcMonitor.left;
   const UINT monitor_height = info.rcMonitor.bottom - info.rcMonitor.top;

   const UINT window_width = monitor_width * user_config.display.screen_percent / 100;
   const UINT window_height =
      monitor_height * user_config.display.screen_percent / 100;

   const UINT dpi = GetDpiForWindow(_window);
   const UINT base_dpi = 96;
   std::uint32_t text_dpi = dpi;

   _actual_width = window_width;
   _actual_height = window_height;

   if (user_config.display.treat_800x600_as_interface &&
       params->BackBufferWidth == 800 && params->BackBufferHeight == 600) {
      _perceived_width = 800;
      _perceived_height = 600;

      if (user_config.display.windowed_interface) {
         if (user_config.display.dpi_aware && user_config.display.dpi_scaling) {
            _actual_width = std::min(800 * dpi / base_dpi, monitor_width);
            _actual_height = std::min(600 * dpi / base_dpi, monitor_height);
         }
         else {
            _actual_width = 800;
            _actual_height = 600;
         }
      }
      else {
         text_dpi = text_dpi_from_resolution(_actual_width, _actual_height,
                                             _perceived_width,
                                             _perceived_height, base_dpi);
      }
   }
   else {
      if (user_config.display.enable_game_perceived_resolution_override) {
         _perceived_width = user_config.display.game_perceived_resolution_override_width;
         _perceived_height =
            user_config.display.game_perceived_resolution_override_height;

         text_dpi = text_dpi_from_resolution(_actual_width, _actual_height,
                                             _perceived_width,
                                             _perceived_height, base_dpi);
      }
      else if (user_config.display.dpi_aware && user_config.display.dpi_scaling) {
         _perceived_width = _actual_width * base_dpi / dpi;
         _perceived_height = _actual_height * base_dpi / dpi;
      }
      else {
         _perceived_width = _actual_width;
         _perceived_height = _actual_height;
         text_dpi = base_dpi;
      }
   }

   params->BackBufferWidth = _perceived_width;
   params->BackBufferHeight = _perceived_height;

   win32::resize_window(_window, _actual_width, _actual_height);

   if (user_config.display.centred || user_config.display.screen_percent == 100) {
      win32::centre_window(_window);
   }
   else {
      win32::leftalign_window(_window);
   }

   win32::clip_cursor_to_window(_window);

   _render_state_manager.reset();
   _texture_stage_manager.reset();
   _shader_patch.reset(_actual_width, _actual_height);
   _shader_patch.set_text_dpi(text_dpi);
   _fixed_func_active = true;

   _backbuffer = Surface_backbuffer::create(_shader_patch.get_back_buffer(),
                                            _perceived_width, _perceived_height);
   _rendertarget = _backbuffer;
   _depthstencil = Surface_depthstencil::create(core::Game_depthstencil::nearscene,
                                                _perceived_width, _perceived_height);

   _viewport = {0, 0, _perceived_width, _perceived_height, 0.0f, 1.0f};

   return S_OK;
}

HRESULT Device::Present(const RECT*, const RECT*, HWND, const RGNDATA*) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   _shader_patch.present_async();

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

   *back_buffer = _backbuffer.unmanaged_copy();

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
      *volume_texture =
         Texture3d_resource::create(_shader_patch, width, height, depth).release();

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

   *surface = Surface_depthstencil::create(surface_name, width, height).release();

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

   const RECT default_source_rect{0, 0, static_cast<LONG>(src_desc.Width),
                                  static_cast<LONG>(src_desc.Height)};

   D3DSURFACE_DESC dest_desc{};
   dest_surface->GetDesc(&dest_desc);

   const RECT default_dest_rect{0, 0, static_cast<LONG>(dest_desc.Width),
                                static_cast<LONG>(dest_desc.Height)};

   if (!source_rect) source_rect = &default_source_rect;
   if (!dest_rect) dest_rect = &default_dest_rect;

   const auto rect_in_surface = [](RECT rect, D3DSURFACE_DESC desc) -> bool {
      if ((rect.right - rect.left) > desc.Width) return false;
      if ((rect.bottom - rect.top) > desc.Height) return false;

      return true;
   };

   if (!rect_in_surface(*source_rect, src_desc)) return D3DERR_INVALIDCALL;
   if (!rect_in_surface(*dest_rect, dest_desc)) return D3DERR_INVALIDCALL;

   Com_ptr<Rendertarget_accessor> source_rt;

   if (FAILED(source_surface->QueryInterface(IID_Rendertarget_accessor,
                                             source_rt.void_clear_and_assign()))) {
      return D3DERR_INVALIDCALL;
   }

   Com_ptr<Rendertarget_accessor> dest_rt;

   if (FAILED(dest_surface->QueryInterface(IID_Rendertarget_accessor,
                                           dest_rt.void_clear_and_assign()))) {
      return D3DERR_INVALIDCALL;
   }

   const auto perceived_rect_to_actual = [this](const D3DSURFACE_DESC& desc, RECT rect) {
      if (desc.Width == _perceived_width && desc.Height == _perceived_height) {
         rect.left = rect.left * _actual_width / _perceived_width;
         rect.right = rect.right * _actual_width / _perceived_width;
         rect.top = rect.top * _actual_height / _perceived_height;
         rect.bottom = rect.bottom * _actual_height / _perceived_height;
      }

      return rect;
   };

   _shader_patch.stretch_rendertarget_async(source_rt->rendertarget(),
                                            perceived_rect_to_actual(src_desc, *source_rect),
                                            dest_rt->rendertarget(),
                                            perceived_rect_to_actual(dest_desc,
                                                                     *dest_rect));

   return S_OK;
}

HRESULT Device::ColorFill(IDirect3DSurface9* surface, const RECT* rect,
                          D3DCOLOR color) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!surface) return D3DERR_INVALIDCALL;

   Com_ptr<Rendertarget_accessor> rendertarget;

   if (FAILED(surface->QueryInterface(IID_Rendertarget_accessor,
                                      rendertarget.void_clear_and_assign()))) {
      return D3DERR_INVALIDCALL;
   }

   _shader_patch.color_fill_rendertarget_async(rendertarget->rendertarget(),
                                               d3dcolor_to_clear_color(color),
                                               rect ? std::optional{*rect}
                                                    : std::nullopt);

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

   *surface = Surface_systemmem_dummy::create(width, height).release();

   return S_OK;
}

HRESULT Device::SetRenderTarget(DWORD rendertarget_index,
                                IDirect3DSurface9* rendertarget) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (rendertarget_index != 0) return D3DERR_NOTFOUND;
   if (!rendertarget) return D3DERR_INVALIDCALL;

   Com_ptr<Rendertarget_accessor> rendertarget_access;

   if (FAILED(rendertarget->QueryInterface(IID_Rendertarget_accessor,
                                           rendertarget_access.void_clear_and_assign()))) {
      return D3DERR_INVALIDCALL;
   }

   // update viewport
   {
      D3DSURFACE_DESC desc{};

      rendertarget->GetDesc(&desc);

      _viewport = {0, 0, desc.Width, desc.Height, 0.0f, 1.0f};
   }

   _shader_patch.set_rendertarget_async(rendertarget_access->rendertarget());

   _rendertarget = copy_raw_com_ptr(rendertarget);

   return S_OK;
}

HRESULT Device::GetRenderTarget(DWORD rendertarget_index,
                                IDirect3DSurface9** rendertarget) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (rendertarget_index != 0) return D3DERR_NOTFOUND;
   if (!rendertarget) return D3DERR_INVALIDCALL;

   *rendertarget = _rendertarget.unmanaged_copy();

   return S_OK;
}

HRESULT Device::SetDepthStencilSurface(IDirect3DSurface9* new_z_stencil) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!new_z_stencil) {
      _shader_patch.set_depthstencil_async(core::Game_depthstencil::none);
      _depthstencil = nullptr;

      return S_OK;
   }

   Com_ptr<Depthstencil_accessor> depthstencil_accessor;

   if (FAILED(new_z_stencil->QueryInterface(IID_Depthstencil_accessor,
                                            depthstencil_accessor.void_clear_and_assign()))) {
      return D3DERR_INVALIDCALL;
   }

   _shader_patch.set_depthstencil_async(depthstencil_accessor->depthstencil());

   _depthstencil = copy_raw_com_ptr(new_z_stencil);

   return S_OK;
}

HRESULT Device::GetDepthStencilSurface(IDirect3DSurface9** z_stencil_surface) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!z_stencil_surface) return D3DERR_INVALIDCALL;

   if (_depthstencil) {
      *z_stencil_surface = _depthstencil.unmanaged_copy();
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
      _shader_patch.clear_depthstencil_async(z, static_cast<UINT8>(stencil),
                                             (flags & (D3DCLEAR_ZBUFFER)) != 0,
                                             (flags & (D3DCLEAR_STENCIL)) != 0);
   }

   if (flags & D3DCLEAR_TARGET) {
      _shader_patch.clear_rendertarget_async(d3dcolor_to_clear_color(color));
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
      auto* async_param_location =
         _shader_patch.allocate_memory_for_async_data(sizeof(glm::mat4)).data();

      auto& async_matrix =
         *new (async_param_location) glm::mat4{bit_cast<glm::mat4>(*matrix)};

      _shader_patch.set_informal_projection_matrix_async(async_matrix);
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
      {D3DRS_FOGSTART, std::bit_cast<DWORD>(0.0f)},
      {D3DRS_FOGEND, std::bit_cast<DWORD>(0.0f)},
      {D3DRS_FOGDENSITY, std::bit_cast<DWORD>(1.0f)},
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
      {D3DRS_POINTSIZE, std::bit_cast<DWORD>(1.0f)},
      {D3DRS_POINTSIZE_MIN, std::bit_cast<DWORD>(1.0f)},
      {D3DRS_POINTSPRITEENABLE, FALSE},
      {D3DRS_POINTSCALEENABLE, FALSE},
      {D3DRS_POINTSCALE_A, std::bit_cast<DWORD>(1.0f)},
      {D3DRS_POINTSCALE_B, std::bit_cast<DWORD>(1.0f)},
      {D3DRS_POINTSCALE_C, std::bit_cast<DWORD>(1.0f)},
      {D3DRS_MULTISAMPLEANTIALIAS, TRUE},
      {D3DRS_MULTISAMPLEMASK, 0xffffffff},
      {D3DRS_PATCHEDGESTYLE, D3DPATCHEDGE_DISCRETE},
      {D3DRS_DEBUGMONITORTOKEN, D3DDMT_DISABLE},
      {D3DRS_POINTSIZE_MAX, std::bit_cast<DWORD>(1.0f)},
      {D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE},
      {D3DRS_COLORWRITEENABLE, 0xf},
      {D3DRS_TWEENFACTOR, std::bit_cast<DWORD>(0.0f)},
      {D3DRS_BLENDOP, D3DBLENDOP_ADD},
      {D3DRS_POSITIONDEGREE, D3DDEGREE_CUBIC},
      {D3DRS_NORMALDEGREE, D3DDEGREE_LINEAR},
      {D3DRS_SCISSORTESTENABLE, FALSE},
      {D3DRS_SLOPESCALEDEPTHBIAS, 0},
      {D3DRS_ANTIALIASEDLINEENABLE, FALSE},
      {D3DRS_MINTESSELLATIONLEVEL, std::bit_cast<DWORD>(1.0f)},
      {D3DRS_MAXTESSELLATIONLEVEL, std::bit_cast<DWORD>(1.0f)},
      {D3DRS_ADAPTIVETESS_X, std::bit_cast<DWORD>(0.0f)},
      {D3DRS_ADAPTIVETESS_Y, std::bit_cast<DWORD>(0.0f)},
      {D3DRS_ADAPTIVETESS_Z, std::bit_cast<DWORD>(0.0f)},
      {D3DRS_ADAPTIVETESS_W, std::bit_cast<DWORD>(0.0f)},
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
      {D3DRS_DEPTHBIAS, std::bit_cast<DWORD>(0.0f)},
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
      if (stage == 0) _shader_patch.set_patch_material_async(core::null_handle);

      _shader_patch.set_texture_async(stage, core::null_handle);

      return S_OK;
   }

   Com_ptr<Texture_accessor> bindable;

   if (FAILED(texture->QueryInterface(IID_Texture_accessor,
                                      bindable.void_clear_and_assign()))) {
      return D3DERR_INVALIDCALL;
   }

   const auto bindable_type = bindable->type();
   const auto bindable_dimension = bindable->dimension();

   if (stage == projtex_slot) {
      if (bindable_dimension == Texture_accessor_dimension::cube) {
         _shader_patch.set_projtex_type_async(core::Projtex_type::texcube);
         _shader_patch.set_projtex_cube_async(bindable->texture());
      }
      else {
         _shader_patch.set_projtex_type_async(core::Projtex_type::tex2d);
         _shader_patch.set_projtex_cube_async(core::null_handle);
      }
   }

   if (bindable_type == Texture_accessor_type::texture) {
      if (stage == 0) _shader_patch.set_patch_material_async(core::null_handle);

      _shader_patch.set_texture_async(stage, bindable->texture());
   }
   else if (bindable_type == Texture_accessor_type::texture_rendertarget) {
      if (stage == 0) _shader_patch.set_patch_material_async(core::null_handle);

      _shader_patch.set_texture_async(stage, bindable->texture_rendertarget());
   }
   else if (bindable_type == Texture_accessor_type::material && stage == 0) {
      _shader_patch.set_patch_material_async(bindable->material());
   }

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
      _shader_patch.set_projtex_mode_async(value == D3DTADDRESS_WRAP
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
   if (primitive_type == D3DPT_TRIANGLEFAN) [[unlikely]] {
      SetIndices(_triangle_fan_quad_ibuf.get());
      _shader_patch.draw_indexed_async(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, 6,
                                       0, start_vertex);
   }
   else {
      _shader_patch.draw_async(d3d_primitive_type_to_d3d11_topology(primitive_type),
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

   _shader_patch.draw_indexed_async(d3d_primitive_type_to_d3d11_topology(primitive_type),
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

   _shader_patch.set_input_layout_async(
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

   _shader_patch.set_game_shader_async(
      static_cast<Vertex_shader*>(shader)->game_shader());
   _fixed_func_active = false;

   return S_OK;
}

HRESULT Device::SetVertexShaderConstantF(UINT start_register, const float* constant_data,
                                         UINT vector4f_count) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (start_register < 2) return S_OK;

   const auto constants = _shader_patch.allocate_memory_for_async_data(
      vector4f_count * sizeof(std::array<float, 4>));

   std::memcpy(constants.data(), constant_data, constants.size());

   if (start_register < 12) {
      const auto start = start_register - 2u;
      const auto count = safe_min(vector4f_count, core::cb::scene_game_count - start);

      _shader_patch.set_constants_async(
         core::cb::scene, start,
         constants.subspan(0, count * sizeof(std::array<float, 4>)));
   }
   else if (start_register < 51) {
      const auto start = start_register - 12u;
      const auto count = safe_min(vector4f_count, core::cb::draw_game_count - start);

      _shader_patch.set_constants_async(
         core::cb::draw, start,
         constants.subspan(0, count * sizeof(std::array<float, 4>)));
   }
   else {
      const auto start = start_register - 51u;
      const auto count = safe_min(vector4f_count, core::cb::skin_game_count - start);

      _shader_patch.set_constants_async(
         core::cb::skin, start,
         constants.subspan(0, count * sizeof(std::array<float, 4>)));
   }

   return S_OK;
}

HRESULT Device::SetStreamSource(UINT stream_number, IDirect3DVertexBuffer9* stream_data,
                                UINT offset_in_bytes, UINT stride) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (stream_number != 0) return D3DERR_INVALIDCALL;
   if (!stream_data) return D3DERR_INVALIDCALL;

   const auto& vertex_buffer = *static_cast<Vertex_buffer*>(stream_data);

   _shader_patch.set_vertex_buffer_async(vertex_buffer.buffer(), offset_in_bytes, stride);

   return S_OK;
}

HRESULT Device::SetIndices(IDirect3DIndexBuffer9* index_data) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (!index_data) return D3DERR_INVALIDCALL;

   const auto& index_buffer = *static_cast<Index_buffer*>(index_data);

   _shader_patch.set_index_buffer_async(index_buffer.buffer(), 0);

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
   const auto constants = _shader_patch.allocate_memory_for_async_data(
      vector4f_count * sizeof(std::array<float, 4>));

   std::memcpy(constants.data(), constant_data, constants.size());

   _shader_patch.set_constants_async(core::cb::draw_ps, start_register, constants);

   return S_OK;
}

HRESULT Device::CreateQuery(D3DQUERYTYPE type, IDirect3DQuery9** query) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   switch (type) {
   case D3DQUERYTYPE_EVENT:
   case D3DQUERYTYPE_OCCLUSION:
      if (!query) return S_OK;

      *query = make_query(type).release();

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

   return Texture2d_managed::create(_shader_patch, width, height, mip_levels,
                                    format, d3d_format, std::move(format_patcher));
}

auto Device::create_texture2d_rendertarget(const UINT width, const UINT height) noexcept
   -> Com_ptr<IDirect3DTexture9>
{
   const UINT texture_actual_width = width == _perceived_width ? _actual_width : width;
   const UINT texture_actual_height =
      height == _perceived_height ? _actual_height : height;

   return Texture2d_rendertarget::create(_shader_patch, texture_actual_width,
                                         texture_actual_height, width, height);
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

   return Texture3d_managed::create(_shader_patch, width, height, depth,
                                    mip_levels, format, d3d_format);
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

   return Texturecube_managed::create(_shader_patch, width, mip_levels, format,
                                      d3d_format, std::move(format_patcher));
}

auto Device::create_vertex_buffer(const UINT size, const bool dynamic) noexcept
   -> Com_ptr<IDirect3DVertexBuffer9>
{
   return Vertex_buffer::create(_shader_patch, size, dynamic);
}

auto Device::create_index_buffer(const UINT size, const bool dynamic) noexcept
   -> Com_ptr<IDirect3DIndexBuffer9>
{
   return Index_buffer::create(_shader_patch, size, dynamic);
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
