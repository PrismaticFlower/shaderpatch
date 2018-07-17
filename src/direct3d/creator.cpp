
#include "creator.hpp"
#include "../fps_unlock.hpp"
#include "check_required_features.hpp"
#include "device.hpp"

namespace sp::direct3d {

using namespace std::literals;

Com_ptr<Creator> Creator::create(Com_ptr<IDirect3D9> actual) noexcept
{
   check_required_features(*actual);

   if (User_config{"shader patch.yml"s}.game.unlock_fps) fps_unlock();

   return Com_ptr{new Creator{std::move(actual)}};
}

HRESULT Creator::CreateDevice(UINT adapter, D3DDEVTYPE device_type,
                              HWND focus_window, DWORD behavior_flags,
                              D3DPRESENT_PARAMETERS* presentation_parameters,
                              IDirect3DDevice9** returned_device_interface) noexcept
{
   Com_ptr<IDirect3DDevice9> device;

   const auto result =
      _creator->CreateDevice(adapter, device_type, focus_window, behavior_flags,
                             presentation_parameters, device.clear_and_assign());

   D3DCAPS9 caps{};

   device->GetDeviceCaps(&caps);

   if (result == S_OK) {
      auto stencil_shadow_format = D3DFMT_A8R8G8B8;

      if (_creator->CheckDeviceFormat(adapter, device_type, D3DFMT_X8R8G8B8,
                                      D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE,
                                      D3DFMT_A8) == S_OK) {
         stencil_shadow_format = D3DFMT_A8;
      }

      const auto resolution = glm::ivec2{presentation_parameters->BackBufferWidth,
                                         presentation_parameters->BackBufferHeight};

      *returned_device_interface =
         new Device{*device, focus_window, resolution, caps, stencil_shadow_format};
   }

   return result;
}

HRESULT Creator::QueryInterface(const IID& iid, void** object) noexcept
{
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

   return E_NOINTERFACE;
}

ULONG Creator::AddRef() noexcept
{
   return _ref_count += 1;
}

ULONG Creator::Release() noexcept
{
   const auto ref_count = _ref_count -= 1;

   if (ref_count == 0) delete this;

   return ref_count;
}

HRESULT Creator::RegisterSoftwareDevice(void* initialize_function) noexcept
{
   return _creator->RegisterSoftwareDevice(initialize_function);
}

UINT Creator::GetAdapterCount() noexcept
{
   return _creator->GetAdapterCount();
}

HRESULT Creator::GetAdapterIdentifier(UINT adapter, DWORD flags,
                                      D3DADAPTER_IDENTIFIER9* identifier) noexcept
{
   return _creator->GetAdapterIdentifier(adapter, flags, identifier);
}

UINT Creator::GetAdapterModeCount(UINT adapter, D3DFORMAT format) noexcept
{
   return _creator->GetAdapterModeCount(adapter, format);
}

HRESULT Creator::EnumAdapterModes(UINT adapter, D3DFORMAT format, UINT mode,
                                  D3DDISPLAYMODE* display_mode) noexcept
{
   return _creator->EnumAdapterModes(adapter, format, mode, display_mode);
}

HRESULT Creator::GetAdapterDisplayMode(UINT adapter, D3DDISPLAYMODE* display_mode) noexcept
{
   return _creator->GetAdapterDisplayMode(adapter, display_mode);
}

HRESULT Creator::CheckDeviceType(UINT adapter, D3DDEVTYPE dev_type,
                                 D3DFORMAT adapter_format,
                                 D3DFORMAT back_buffer_format, BOOL windowed) noexcept
{
   return _creator->CheckDeviceType(adapter, dev_type, adapter_format,
                                    back_buffer_format, windowed);
}

HRESULT Creator::CheckDeviceFormat(UINT adapter, D3DDEVTYPE device_type,
                                   D3DFORMAT adapter_format, DWORD usage,
                                   D3DRESOURCETYPE res_type, D3DFORMAT check_format) noexcept

{
   return _creator->CheckDeviceFormat(adapter, device_type, adapter_format,
                                      usage, res_type, check_format);
}

HRESULT Creator::CheckDeviceMultiSampleType(UINT adapter, D3DDEVTYPE device_type,
                                            D3DFORMAT surface_format, BOOL windowed,
                                            D3DMULTISAMPLE_TYPE multi_sample_type,
                                            DWORD* quality_levels) noexcept
{
   return _creator->CheckDeviceMultiSampleType(adapter, device_type,
                                               surface_format, windowed,
                                               multi_sample_type, quality_levels);
}

HRESULT Creator::CheckDepthStencilMatch(UINT adapter, D3DDEVTYPE device_type,
                                        D3DFORMAT adapter_format,
                                        D3DFORMAT render_target_format,
                                        D3DFORMAT depth_stencil_format) noexcept
{
   return _creator->CheckDepthStencilMatch(adapter, device_type, adapter_format,
                                           render_target_format, depth_stencil_format);
}

HRESULT Creator::CheckDeviceFormatConversion(UINT adapter, D3DDEVTYPE device_type,
                                             D3DFORMAT source_format,
                                             D3DFORMAT target_format) noexcept
{
   return _creator->CheckDeviceFormatConversion(adapter, device_type,
                                                source_format, target_format);
}

HRESULT Creator::GetDeviceCaps(UINT adapter, D3DDEVTYPE device_type, D3DCAPS9* caps) noexcept
{
   return _creator->GetDeviceCaps(adapter, device_type, caps);
}

HMONITOR Creator::GetAdapterMonitor(UINT adapter) noexcept
{
   return _creator->GetAdapterMonitor(adapter);
}

}
