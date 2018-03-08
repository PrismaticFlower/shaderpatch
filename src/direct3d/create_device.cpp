
#include <type_traits>

#include "create_device.hpp"
#include "device.hpp"

namespace sp::direct3d {
std::atomic<Create_type*> create_device = nullptr;

HRESULT __stdcall create_device_hook(IDirect3D9& self, UINT adapter,
                                     D3DDEVTYPE device_type, HWND focus_window,
                                     DWORD behavior_flags,
                                     D3DPRESENT_PARAMETERS* presentation_parameters,
                                     IDirect3DDevice9** returned_device_interface) noexcept
{
   const auto create = create_device.load();

   Com_ptr<IDirect3DDevice9> device;

   glm::ivec2 resolution{};

   if (presentation_parameters) {
      resolution = glm::ivec2{presentation_parameters->BackBufferWidth,
                              presentation_parameters->BackBufferHeight};
   }

   const auto result = create(self, adapter, device_type, focus_window, behavior_flags,
                              presentation_parameters, device.clear_and_assign());

   if (result == S_OK) {
      *returned_device_interface =
         new Device{std::move(device), focus_window, resolution};
   }

   return result;
}
}
