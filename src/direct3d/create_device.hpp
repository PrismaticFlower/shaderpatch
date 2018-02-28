#pragma once

#include <atomic>
#include <type_traits>

#include <d3d9.h>

namespace sp::direct3d {

using Create_type = HRESULT __stdcall(IDirect3D9&, UINT, D3DDEVTYPE, HWND, DWORD,
                                      D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);

extern std::atomic<Create_type*> create_device;

HRESULT __stdcall create_device_hook(IDirect3D9& self, UINT adapter,
                                     D3DDEVTYPE device_type, HWND focus_window,
                                     DWORD behavior_flags,
                                     D3DPRESENT_PARAMETERS* presentation_parameters,
                                     IDirect3DDevice9** returned_device_interface) noexcept;
}
