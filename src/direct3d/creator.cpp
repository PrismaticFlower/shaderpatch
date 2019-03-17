
#include "creator.hpp"
#include "../logger.hpp"
#include "../user_config.hpp"
#include "../window_helpers.hpp"
#include "check_required_features.hpp"
#include "debug_trace.hpp"
#include "device.hpp"
#include "helpers.hpp"

#include <algorithm>
#include <cwchar>
#include <limits>
#include <queue>

namespace sp::d3d9 {

using namespace std::literals;

namespace {

auto enum_adapters_dxgi_1_6(IDXGIFactory6& factory) noexcept
   -> std::vector<Com_ptr<IDXGIAdapter2>>
{
   Com_ptr<IDXGIAdapter2> adapater;

   std::vector<Com_ptr<IDXGIAdapter2>> adapaters;

   for (int i = 0;
        factory.EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                           __uuidof(IDXGIAdapter2),
                                           adapater.void_clear_and_assign()) !=
        DXGI_ERROR_NOT_FOUND;
        ++i) {
      adapaters.emplace_back(std::move(adapater));
   }

   return adapaters;
}

auto enum_adapaters(IDXGIFactory2& factory) noexcept
   -> std::vector<Com_ptr<IDXGIAdapter2>>
{
   std::vector<Com_ptr<IDXGIAdapter2>> adapaters;

   Com_ptr<IDXGIAdapter1> adapater;

   for (int i = 0;
        factory.EnumAdapters1(i, adapater.clear_and_assign()) != DXGI_ERROR_NOT_FOUND;
        ++i) {

      if (FAILED(adapater->QueryInterface(adapaters.emplace_back().clear_and_assign()))) {
         adapaters.pop_back();
      }
   }

   return adapaters;
}

auto select_adapater(IDXGIFactory2& factory) noexcept -> Com_ptr<IDXGIAdapter2>
{
   if (Com_ptr<IDXGIFactory6> factory_1_6;
       !IsDebuggerPresent() &&
       SUCCEEDED(factory.QueryInterface(factory_1_6.clear_and_assign()))) {
      const auto adapters = enum_adapters_dxgi_1_6(*factory_1_6);

      return adapters[0];
   }

   const auto compare_adapaters = [](const Com_ptr<IDXGIAdapter2>& left,
                                     const Com_ptr<IDXGIAdapter2>& right) {
      DXGI_ADAPTER_DESC2 ldesc;
      left->GetDesc2(&ldesc);

      DXGI_ADAPTER_DESC2 rdesc;
      right->GetDesc2(&rdesc);

      return ldesc.DedicatedVideoMemory < rdesc.DedicatedVideoMemory;
   };

   return std::priority_queue{compare_adapaters, enum_adapaters(factory)}.top();
}

auto enum_outputs(IDXGIAdapter2& adapter) noexcept
   -> std::vector<Com_ptr<IDXGIOutput1>>
{
   std::vector<Com_ptr<IDXGIOutput1>> outputs;

   Com_ptr<IDXGIOutput> output;

   for (int i = 0;
        adapter.EnumOutputs(i, output.clear_and_assign()) != DXGI_ERROR_NOT_FOUND;
        ++i) {

      if (FAILED(output->QueryInterface(outputs.emplace_back().clear_and_assign()))) {
         outputs.pop_back();
      }
   }

   return outputs;
}

auto enum_display_modes(IDXGIOutput1& output, DXGI_FORMAT format) noexcept
{
   UINT mode_count{};

   output.GetDisplayModeList1(format, 0, &mode_count, nullptr);

   std::vector<DXGI_MODE_DESC1> display_modes{mode_count};

   if (FAILED(output.GetDisplayModeList1(format, 0, &mode_count, display_modes.data()))) {
      display_modes.clear();
   }

   return display_modes;
}

}

Com_ptr<Creator> Creator::create() noexcept
{
   static auto creator = Com_ptr{new Creator{}};

   return creator;
}

Creator::Creator() noexcept
{
   if (FAILED(CreateDXGIFactory1(IID_IDXGIFactory2, _factory.void_clear_and_assign()))) {
      log_and_terminate("Failed to create IDXGIFactory2!");
   }

   _adapter = select_adapater(*_factory);
   _outputs = enum_outputs(*_adapter);

   for (auto output : _outputs) {
      _display_modes[output] = enum_display_modes(*output, _desired_backbuffer_format);
   }
}

HRESULT Creator::CreateDevice(UINT adapter_index, D3DDEVTYPE, HWND focus_window,
                              DWORD, D3DPRESENT_PARAMETERS* parameters,
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
   Debug_trace::func(__FUNCSIG__);
   std::lock_guard lock{_mutex};

   return E_NOTIMPL;
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

   std::mbstate_t mbstate{};
   const wchar_t* description = &desc.Description[0];
   std::size_t converted_count;
   wcsrtombs_s(&converted_count, &identifier->Description[0],
               sizeof(identifier->Description), &description,
               sizeof(identifier->Description), &mbstate);

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

   if (_outputs.size() > 0) {
      if (auto output_modes = _display_modes.find(_outputs[_desired_output]);
          output_modes != _display_modes.end())
         return static_cast<UINT>(output_modes->second.size());
   }

   return 0;
}

HRESULT Creator::EnumAdapterModes(UINT adapter_index, D3DFORMAT d3d_format,
                                  UINT mode_index, D3DDISPLAYMODE* display_mode) noexcept
{
   Debug_trace::func(__FUNCSIG__);
   std::lock_guard lock{_mutex};

   if (!display_mode) return D3DERR_INVALIDCALL;
   if (adapter_index != 0) return D3DERR_INVALIDCALL;
   if (d3d_format != D3DFMT_X8R8G8B8) return D3DERR_NOTAVAILABLE;

   const auto& modes = _display_modes[_outputs[_desired_output]];

   if (mode_index >= modes.size()) return D3DERR_INVALIDCALL;

   const auto& mode = modes[mode_index];

   *display_mode = {mode.Width, mode.Height,
                    mode.RefreshRate.Denominator / mode.RefreshRate.Numerator,
                    d3d_format};

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
   case D3DMULTISAMPLE_NONMASKABLE:
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
}
