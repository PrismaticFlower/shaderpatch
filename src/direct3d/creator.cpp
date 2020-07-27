
#include "creator.hpp"
#include "../logger.hpp"
#include "../user_config.hpp"
#include "../window_helpers.hpp"
#include "debug_trace.hpp"
#include "device.hpp"
#include "helpers.hpp"
#include "utility.hpp"

#include <algorithm>
#include <codecvt>
#include <cstdint>
#include <cwchar>
#include <float.h>
#include <limits>
#include <locale>
#include <queue>

#include <d3d11_4.h>

namespace sp::d3d9 {

using namespace std::literals;

namespace {

auto select_adapter_by_preference(IDXGIFactory6& factory,
                                  const DXGI_GPU_PREFERENCE preference) noexcept
   -> Com_ptr<IDXGIAdapter4>
{
   Com_ptr<IDXGIAdapter4> adapater;

   factory.EnumAdapterByGpuPreference(0, preference, __uuidof(IDXGIAdapter4),
                                      adapater.void_clear_and_assign());

   if (!adapater) {
      log_and_terminate("Failed to find suitable GPU adapter!");
   }

   return adapater;
}

auto select_adapter_by_feature_level(IDXGIFactory6& factory) noexcept
   -> Com_ptr<IDXGIAdapter4>
{
   using Adapter_pair = std::pair<D3D_FEATURE_LEVEL, Com_ptr<IDXGIAdapter4>>;

   const auto comparator = [](const Adapter_pair& left, const Adapter_pair& right) {
      return left.first < right.first;
   };

   std::priority_queue<Adapter_pair, std::vector<Adapter_pair>, decltype(comparator)> adapaters{
      comparator};

   constexpr std::array feature_levels{D3D_FEATURE_LEVEL_9_1,
                                       D3D_FEATURE_LEVEL_9_2,
                                       D3D_FEATURE_LEVEL_9_3,
                                       D3D_FEATURE_LEVEL_10_0,
                                       D3D_FEATURE_LEVEL_10_1,
                                       D3D_FEATURE_LEVEL_11_0,
                                       D3D_FEATURE_LEVEL_11_1,
                                       D3D_FEATURE_LEVEL_12_0,
                                       D3D_FEATURE_LEVEL_12_1};

   Com_ptr<IDXGIAdapter4> adapter;
   for (int i = 0;
        factory.EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_UNSPECIFIED,
                                           __uuidof(IDXGIAdapter4),
                                           adapter.void_clear_and_assign()) !=
        DXGI_ERROR_NOT_FOUND;
        ++i) {
      D3D_FEATURE_LEVEL fl;
      Com_ptr<ID3D11Device> device;

      if (const auto hr = D3D11CreateDevice(adapter.get(), D3D_DRIVER_TYPE_UNKNOWN,
                                            nullptr, 0, feature_levels.data(),
                                            feature_levels.size(), D3D11_SDK_VERSION,
                                            device.clear_and_assign(), &fl, nullptr);
          FAILED(hr)) {
         continue;
      }

      adapaters.push(std::pair{fl, adapter});
   }

   if (adapaters.empty()) {
      log_and_terminate("Failed to find suitable GPU adapter!");
   }

   return adapaters.top().second;
}

auto select_adapter_by_memory(IDXGIFactory6& factory) noexcept -> Com_ptr<IDXGIAdapter4>
{
   const auto comparator = [](const Com_ptr<IDXGIAdapter4>& left,
                              const Com_ptr<IDXGIAdapter4>& right) {
      DXGI_ADAPTER_DESC2 left_desc, right_desc;

      left->GetDesc2(&left_desc);
      right->GetDesc2(&right_desc);

      const auto left_mem = std::uint64_t{left_desc.DedicatedVideoMemory} +
                            left_desc.DedicatedSystemMemory +
                            left_desc.SharedSystemMemory;

      const auto right_mem = std::uint64_t{right_desc.DedicatedVideoMemory} +
                             right_desc.DedicatedSystemMemory +
                             right_desc.SharedSystemMemory;

      return left_mem < right_mem;
   };

   std::priority_queue<Com_ptr<IDXGIAdapter4>, std::vector<Com_ptr<IDXGIAdapter4>>, decltype(comparator)> adapaters{
      comparator};

   Com_ptr<IDXGIAdapter4> adapter;
   for (int i = 0;
        factory.EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_UNSPECIFIED,
                                           __uuidof(IDXGIAdapter4),
                                           adapter.void_clear_and_assign()) !=
        DXGI_ERROR_NOT_FOUND;
        ++i) {
      adapaters.push(std::move(adapter));
   }

   if (adapaters.empty()) {
      log_and_terminate("Failed to find suitable GPU adapter!");
   }

   return adapaters.top();
}

auto get_adapater(IDXGIFactory6& factory) noexcept -> Com_ptr<IDXGIAdapter4>
{
   switch (user_config.graphics.gpu_selection_method) {
   case GPU_selection_method::highest_performance:
      return select_adapter_by_preference(factory, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE);
   case GPU_selection_method::lowest_power_usage:
      return select_adapter_by_preference(factory, DXGI_GPU_PREFERENCE_MINIMUM_POWER);
   case GPU_selection_method::highest_feature_level:
      return select_adapter_by_feature_level(factory);
   case GPU_selection_method::most_memory:
      return select_adapter_by_memory(factory);
   case GPU_selection_method::use_cpu:
      Com_ptr<IDXGIAdapter4> adapter;
      factory.EnumWarpAdapter(__uuidof(IDXGIAdapter4),
                              adapter.void_clear_and_assign());
      return adapter;
   }

   std::terminate();
}

void update_fp_control() noexcept
{
   unsigned int old_fp;
   auto result =
      _controlfp_s(&old_fp, _MCW_DN | _MCW_EM | _MCW_PC,
                   _DN_FLUSH | _EM_INEXACT | _EM_UNDERFLOW | _EM_OVERFLOW |
                      _EM_ZERODIVIDE | _EM_INVALID | _EM_DENORMAL | _PC_24);

   if (result)
      log(Log_level::warning,
          "Failed to set FP control flags, strange bugs may appear.");
}

}

Com_ptr<Creator> Creator::create() noexcept
{
   static auto creator = Com_ptr{new Creator{}};

   return creator;
}

Creator::Creator() noexcept
{
   if (const auto dxgi_create_flags =
          user_config.developer.use_d3d11_debug_layer ? DXGI_CREATE_FACTORY_DEBUG : 0x0;
       user_config.developer.use_dxgi_1_2_factory) {
      legacy_create_adapter(dxgi_create_flags);
   }
   else {
      create_adapter(dxgi_create_flags);
   }

   log(Log_level::info, "Selected GPU "sv, gpu_desc());

   MONITORINFO info{sizeof(MONITORINFO)};
   GetMonitorInfoW(MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY), &info);

   pseudo_display_modes[2].Width = (info.rcMonitor.right - info.rcMonitor.left);
   pseudo_display_modes[2].Height = (info.rcMonitor.bottom - info.rcMonitor.top);
}

HRESULT Creator::CreateDevice(UINT adapter_index, D3DDEVTYPE, HWND focus_window,
                              DWORD behavior_flags, D3DPRESENT_PARAMETERS* parameters,
                              IDirect3DDevice9** returned_device_interface) noexcept
{
   Debug_trace::func(__FUNCSIG__);
   std::lock_guard lock{_mutex};

   if (adapter_index != 0) return D3DERR_INVALIDCALL;
   if (!parameters) return D3DERR_INVALIDCALL;
   if (!returned_device_interface) return D3DERR_INVALIDCALL;

   if (!_device) {
      win32::make_borderless_window(focus_window);
      win32::clip_cursor_to_window(focus_window);
      ShowWindow(focus_window, SW_NORMAL);

      _device =
         Device::create(*this, *_adapter,
                        parameters->hDeviceWindow ? parameters->hDeviceWindow : focus_window,
                        parameters->BackBufferWidth, parameters->BackBufferHeight);
   }
   else {
      _device->Reset(parameters);
   }

   *returned_device_interface = _device.unmanaged_copy();

   if (!((behavior_flags & D3DCREATE_FPU_PRESERVE) == D3DCREATE_FPU_PRESERVE)) {
      update_fp_control();
   }

   return S_OK;
}

HRESULT Creator::QueryInterface(const IID& iid, void** object) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (iid == IID_IDirect3D9) {
      AddRef();

      *object = this;

      return S_OK;
   }
   else if (iid == IID_IUnknown) {
      AddRef();

      *object = this;

      return S_OK;
   }

   *object = nullptr;

   return E_NOINTERFACE;
}

ULONG Creator::AddRef() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return _ref_count += 1;
}

ULONG Creator::Release() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

HRESULT Creator::RegisterSoftwareDevice(void*) noexcept
{
   log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
}

UINT Creator::GetAdapterCount() noexcept
{
   Debug_trace::func(__FUNCSIG__);

   return 1;
}

HRESULT Creator::GetAdapterIdentifier(UINT adapter_index, DWORD,
                                      D3DADAPTER_IDENTIFIER9* identifier) noexcept
{
   Debug_trace::func(__FUNCSIG__);
   std::lock_guard lock{_mutex};

   if (!identifier) return D3DERR_INVALIDCALL;
   if (adapter_index != 0) return D3DERR_INVALIDCALL;

   DXGI_ADAPTER_DESC2 desc{};

   _adapter->GetDesc2(&desc);

   *identifier = D3DADAPTER_IDENTIFIER9{};

   const auto description = gpu_desc();

   *identifier = D3DADAPTER_IDENTIFIER9{};

   std::copy_n(description.data(),
               safe_min(description.size() + 1, sizeof(identifier->Description)),
               std::begin(identifier->Description));
   *(std::end(identifier->Description) - 1) = '\0';

   identifier->DriverVersion.QuadPart = 1;
   identifier->VendorId = desc.VendorId;
   identifier->DeviceId = desc.DeviceId;
   identifier->SubSysId = desc.SubSysId;
   identifier->Revision = desc.Revision;
   identifier->DeviceIdentifier = {0x5b03f4cc,
                                   0xfa34,
                                   0x45d6,
                                   {0xa1, 0xb0, 0x96, 0xf3, 0xfe, 0xd2, 0xb9, 0x21}};
   identifier->WHQLLevel = 1;

   return S_OK;
}

UINT Creator::GetAdapterModeCount(UINT adapter_index, D3DFORMAT d3d_format) noexcept
{
   Debug_trace::func(__FUNCSIG__);
   std::lock_guard lock{_mutex};

   if (adapter_index != 0) return 0;
   if (d3d_format != D3DFMT_X8R8G8B8) return 0;

   return pseudo_display_modes.size();
}

HRESULT Creator::EnumAdapterModes(UINT adapter_index, D3DFORMAT d3d_format,
                                  UINT mode_index, D3DDISPLAYMODE* display_mode) noexcept
{
   Debug_trace::func(__FUNCSIG__);
   std::lock_guard lock{_mutex};

   if (!display_mode) return D3DERR_INVALIDCALL;
   if (adapter_index != 0) return D3DERR_INVALIDCALL;
   if (d3d_format != D3DFMT_X8R8G8B8) return D3DERR_NOTAVAILABLE;
   if (mode_index >= pseudo_display_modes.size()) return D3DERR_INVALIDCALL;

   *display_mode = pseudo_display_modes.begin()[mode_index];

   return S_OK;
}

HRESULT Creator::GetAdapterDisplayMode(UINT, D3DDISPLAYMODE*) noexcept
{
   log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
}

HRESULT Creator::CheckDeviceType(UINT adapter_index, D3DDEVTYPE, D3DFORMAT adapter_format,
                                 D3DFORMAT back_buffer_format, BOOL) noexcept
{
   Debug_trace::func(__FUNCSIG__);
   std::lock_guard lock{_mutex};

   if (adapter_index != 0) return D3DERR_INVALIDCALL;
   if (adapter_format != D3DFMT_X8R8G8B8) return D3DERR_NOTAVAILABLE;
   if (back_buffer_format != D3DFMT_A8R8G8B8) return D3DERR_NOTAVAILABLE;

   return S_OK;
}

HRESULT Creator::CheckDeviceFormat(UINT adapter_index, D3DDEVTYPE, D3DFORMAT, DWORD,
                                   D3DRESOURCETYPE, D3DFORMAT check_format) noexcept

{
   Debug_trace::func(__FUNCSIG__);

   if (adapter_index != 0) return D3DERR_INVALIDCALL;

   if (d3d_to_dxgi_format(check_format) != DXGI_FORMAT_UNKNOWN) return S_OK;

   return D3DERR_NOTAVAILABLE;
}

HRESULT Creator::CheckDeviceMultiSampleType(UINT adapter_index, D3DDEVTYPE,
                                            D3DFORMAT, BOOL,
                                            D3DMULTISAMPLE_TYPE multi_sample_type,
                                            DWORD* quality_levels) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (adapter_index != 0) return D3DERR_INVALIDCALL;

   switch (multi_sample_type) {
   case D3DMULTISAMPLE_NONE:
   case D3DMULTISAMPLE_2_SAMPLES:
   case D3DMULTISAMPLE_4_SAMPLES:
   case D3DMULTISAMPLE_8_SAMPLES:
      if (quality_levels) *quality_levels = 1;
      return S_OK;
   }

   return D3DERR_NOTAVAILABLE;
}

HRESULT Creator::CheckDepthStencilMatch(UINT adapter_index, D3DDEVTYPE,
                                        D3DFORMAT, D3DFORMAT, D3DFORMAT) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (adapter_index != 0) return D3DERR_INVALIDCALL;

   return S_OK;
}

HRESULT Creator::CheckDeviceFormatConversion(UINT adapter_index, D3DDEVTYPE,
                                             D3DFORMAT, D3DFORMAT) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (adapter_index != 0) return D3DERR_INVALIDCALL;

   return S_OK;
}

HRESULT Creator::GetDeviceCaps(UINT adapter_index, D3DDEVTYPE, D3DCAPS9* caps) noexcept
{
   Debug_trace::func(__FUNCSIG__);

   if (adapter_index != 0) return D3DERR_INVALIDCALL;
   if (!caps) return D3DERR_INVALIDCALL;

   *caps = device_caps;

   return S_OK;
}

HMONITOR Creator::GetAdapterMonitor(UINT) noexcept
{
   log_and_terminate("Unimplemented function \"" __FUNCSIG__ "\" called.");
}

void Creator::create_adapter(const UINT dxgi_create_flags) noexcept
{
   Com_ptr<IDXGIFactory6> factory;

   if (FAILED(CreateDXGIFactory2(dxgi_create_flags, __uuidof(IDXGIFactory6),
                                 factory.void_clear_and_assign()))) {
      log_and_terminate(
         "Failed to create DXGI 1.6 factory! This likely means you're not "
         "running Windows 10 or it is not up to date.");
   }

   _adapter = get_adapater(*factory);
}

void Creator::legacy_create_adapter(const UINT dxgi_create_flags) noexcept
{
   Com_ptr<IDXGIFactory2> factory;

   if (FAILED(CreateDXGIFactory2(dxgi_create_flags, __uuidof(IDXGIFactory2),
                                 factory.void_clear_and_assign()))) {
      log_and_terminate("Failed to create DXGI factory!");
   }

   Com_ptr<IDXGIAdapter1> adapater;

   if (FAILED(factory->EnumAdapters1(0, adapater.clear_and_assign()))) {
      log_and_terminate("Failed to create DXGI adapter!");
   }

   adapater->QueryInterface(_adapter.clear_and_assign());
}

auto Creator::gpu_desc() noexcept -> std::string
{
   DXGI_ADAPTER_DESC desc{};
   _adapter->GetDesc(&desc);

   try {
      std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
      return converter.to_bytes(std::cbegin(desc.Description),
                                std::find(std::cbegin(desc.Description),
                                          std::cend(desc.Description), L'\0'));
   }
   catch (std::exception&) {
      return "Unknown"s;
   }
}

}
