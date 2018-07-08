#pragma once

#include "com_ptr.hpp"
#include "smart_win32_handle.hpp"

#include <atomic>

#include <d3d9.h>

namespace sp::direct3d {

class Creator : public IDirect3D9 {
public:
   static Com_ptr<Creator> create(Com_ptr<IDirect3D9> actual) noexcept;

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept override;
   ULONG __stdcall AddRef() noexcept override;
   ULONG __stdcall Release() noexcept override;

   HRESULT __stdcall RegisterSoftwareDevice(void* initialize_function) noexcept override;
   UINT __stdcall GetAdapterCount() noexcept override;
   HRESULT __stdcall GetAdapterIdentifier(UINT adapter, DWORD flags,
                                          D3DADAPTER_IDENTIFIER9* identifier) noexcept override;
   UINT __stdcall GetAdapterModeCount(UINT adapter, D3DFORMAT format) noexcept override;
   HRESULT __stdcall EnumAdapterModes(UINT adapter, D3DFORMAT format, UINT mode,
                                      D3DDISPLAYMODE* display_mode) noexcept override;
   HRESULT __stdcall GetAdapterDisplayMode(UINT adapter,
                                           D3DDISPLAYMODE* mode) noexcept override;
   HRESULT __stdcall CheckDeviceType(UINT adapter, D3DDEVTYPE dev_type,
                                     D3DFORMAT adapter_format,
                                     D3DFORMAT back_buffer_format,
                                     BOOL windowed) noexcept override;
   HRESULT __stdcall CheckDeviceFormat(UINT adapter, D3DDEVTYPE deviceType,
                                       D3DFORMAT adapter_format, DWORD usage,
                                       D3DRESOURCETYPE res_type,
                                       D3DFORMAT check_format) noexcept override;
   HRESULT __stdcall CheckDeviceMultiSampleType(UINT adapter, D3DDEVTYPE device_type,
                                                D3DFORMAT surface_format, BOOL windowed,
                                                D3DMULTISAMPLE_TYPE multi_sample_type,
                                                DWORD* quality_levels) noexcept override;
   HRESULT __stdcall CheckDepthStencilMatch(UINT adapter, D3DDEVTYPE device_type,
                                            D3DFORMAT adapter_format,
                                            D3DFORMAT render_target_format,
                                            D3DFORMAT depth_stencil_format) noexcept override;
   HRESULT __stdcall CheckDeviceFormatConversion(UINT adapter, D3DDEVTYPE device_type,
                                                 D3DFORMAT source_format,
                                                 D3DFORMAT target_format) noexcept override;
   HRESULT __stdcall GetDeviceCaps(UINT adapter, D3DDEVTYPE device_type,
                                   D3DCAPS9* caps) noexcept override;
   HMONITOR __stdcall GetAdapterMonitor(UINT adapter) noexcept override;
   HRESULT __stdcall CreateDevice(UINT adapter, D3DDEVTYPE device_type,
                                  HWND focus_window, DWORD behavior_flags,
                                  D3DPRESENT_PARAMETERS* presentation_parameters,
                                  IDirect3DDevice9** returned_device_interface) noexcept override;

private:
   Creator(Com_ptr<IDirect3D9> actual) : _creator{std::move(actual)} {}
   ~Creator() = default;

   Com_ptr<IDirect3D9> _creator{};
   std::atomic<ULONG> _ref_count{1};
};

}
