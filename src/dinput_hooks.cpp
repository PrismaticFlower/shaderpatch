#include "dinput_hooks.hpp"
#include "com_ptr.hpp"
#include "input_config.hpp"
#include "logger.hpp"
#include "user_config.hpp"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

namespace sp {

namespace {

enum class Device_type { keyboard, mouse, other };

struct IDirectInputDevice8A_overlay : IDirectInputDevice8A {
   static Com_ptr<IDirectInputDevice8A_overlay> create(const GUID& guid,
                                                       IDirectInput8A& dinput) noexcept
   {
      auto dinput_overlay = Com_ptr{new IDirectInputDevice8A_overlay{guid, dinput}};

      return dinput_overlay;
   }

   IDirectInputDevice8A_overlay(const GUID& guid, IDirectInput8A& dinput)
   {
      if (guid == GUID_SysKeyboard)
         _type = Device_type::keyboard;
      else if (guid == GUID_SysMouse)
         _type = Device_type::mouse;

      if (FAILED(dinput.CreateDevice(guid, _device.clear_and_assign(), nullptr))) {
         log_and_terminate("Failed to create IDirectInputDevice8A interface.");
      }
   }

   HRESULT __stdcall SetCooperativeLevel(HWND window,
                                         [[maybe_unused]] DWORD flags) noexcept override
   {
      return _device->SetCooperativeLevel(window, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
   }

   HRESULT __stdcall GetDeviceState(DWORD size, LPVOID data) noexcept override
   {
      const HRESULT result = _device->GetDeviceState(size, data);

      if (_type == Device_type::keyboard && size > DIK_SYSRQ) {
         // Fix print screen crash on some game versions.

         static_cast<char*>(data)[DIK_SYSRQ] = 0;
      }

      if (input_config.mode == Input_mode::imgui) {
         std::memset(data, 0, size);

         return DI_OK;
      }

      return result;
   }

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept
   {
      if (iid == IID_IDirectInputDevice8A) {
         AddRef();

         *object = static_cast<IDirectInputDevice8A*>(this);

         return S_OK;
      }
      else if (iid == IID_IUnknown) {
         AddRef();

         *object = static_cast<IUnknown*>(this);

         return S_OK;
      }

      *object = nullptr;

      return E_NOINTERFACE;
   }

   ULONG __stdcall AddRef() noexcept
   {
      return _ref_count += 1;
   }

   ULONG __stdcall Release() noexcept
   {
      const auto ref_count = _ref_count -= 1;

      if (ref_count == 0) delete this;

      return ref_count;
   }

   HRESULT __stdcall GetCapabilities(LPDIDEVCAPS device_caps) noexcept override
   {
      return _device->GetCapabilities(device_caps);
   }

   HRESULT __stdcall EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA callback,
                                 LPVOID user_data, DWORD flags) noexcept override
   {
      return _device->EnumObjects(callback, user_data, flags);
   }

   HRESULT __stdcall GetProperty(const GUID& guid, LPDIPROPHEADER out_property) noexcept override
   {
      return _device->GetProperty(guid, out_property);
   }

   HRESULT __stdcall SetProperty(const GUID& guid, LPCDIPROPHEADER property) noexcept override
   {
      return _device->SetProperty(guid, property);
   }

   HRESULT __stdcall Acquire() noexcept override
   {
      return _device->Acquire();
   }

   HRESULT __stdcall Unacquire() noexcept override
   {
      return _device->Unacquire();
   }

   HRESULT __stdcall GetDeviceData(DWORD object_data_size,
                                   LPDIDEVICEOBJECTDATA object_data,
                                   LPDWORD object_data_count, DWORD flags) noexcept override
   {
      return _device->GetDeviceData(object_data_size, object_data,
                                    object_data_count, flags);
   }

   HRESULT __stdcall SetDataFormat(LPCDIDATAFORMAT format) noexcept override
   {
      return _device->SetDataFormat(format);
   }

   HRESULT __stdcall SetEventNotification(HANDLE event) noexcept override
   {
      return _device->SetEventNotification(event);
   }

   HRESULT __stdcall GetObjectInfo(LPDIDEVICEOBJECTINSTANCE info_out,
                                   DWORD object, DWORD how) noexcept override
   {
      return _device->GetObjectInfo(info_out, object, how);
   }

   HRESULT __stdcall GetDeviceInfo(LPDIDEVICEINSTANCEA instance_out) noexcept override
   {
      return _device->GetDeviceInfo(instance_out);
   }

   HRESULT __stdcall RunControlPanel(HWND window, DWORD flags) noexcept override
   {
      return _device->RunControlPanel(window, flags);
   }

   HRESULT __stdcall Initialize(HINSTANCE instance, DWORD version,
                                const GUID& guid) noexcept override
   {
      return _device->Initialize(instance, version, guid);
   }

   HRESULT __stdcall CreateEffect(const GUID& guid, LPCDIEFFECT effect_params,
                                  LPDIRECTINPUTEFFECT* effect_out,
                                  LPUNKNOWN unknown_outer) noexcept override
   {
      return _device->CreateEffect(guid, effect_params, effect_out, unknown_outer);
   }

   HRESULT __stdcall EnumEffects(LPDIENUMEFFECTSCALLBACKA callback,
                                 LPVOID user_data, DWORD effect_type) noexcept override
   {
      return _device->EnumEffects(callback, user_data, effect_type);
   }

   HRESULT __stdcall GetEffectInfo(LPDIEFFECTINFOA info_out, const GUID& guid) noexcept override
   {
      return _device->GetEffectInfo(info_out, guid);
   }

   HRESULT __stdcall GetForceFeedbackState(LPDWORD state_out) noexcept override
   {
      return _device->GetForceFeedbackState(state_out);
   }

   HRESULT __stdcall SendForceFeedbackCommand(DWORD command) noexcept override
   {
      return _device->SendForceFeedbackCommand(command);
   }

   HRESULT __stdcall EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback,
                                              LPVOID user_data, DWORD flags) noexcept override
   {
      return _device->EnumCreatedEffectObjects(callback, user_data, flags);
   }

   HRESULT __stdcall Escape(LPDIEFFESCAPE escape) noexcept override
   {
      return _device->Escape(escape);
   }

   HRESULT __stdcall Poll() noexcept override
   {
      return _device->Poll();
   }

   HRESULT __stdcall SendDeviceData(DWORD object_data_size,
                                    LPCDIDEVICEOBJECTDATA object_data,
                                    LPDWORD object_data_count, DWORD flags) noexcept override
   {
      return _device->SendDeviceData(object_data_size, object_data,
                                     object_data_count, flags);
   }

   HRESULT __stdcall EnumEffectsInFile(LPCSTR filename,
                                       LPDIENUMEFFECTSINFILECALLBACK callback,
                                       LPVOID user_data, DWORD flags) noexcept override
   {
      return _device->EnumEffectsInFile(filename, callback, user_data, flags);
   }

   HRESULT __stdcall WriteEffectToFile(LPCSTR filename, DWORD effects_count,
                                       LPDIFILEEFFECT effects, DWORD flags) noexcept override
   {
      return _device->WriteEffectToFile(filename, effects_count, effects, flags);
   }

   HRESULT __stdcall BuildActionMap(LPDIACTIONFORMATA format_out,
                                    LPCSTR user_name, DWORD flags) noexcept override
   {
      return _device->BuildActionMap(format_out, user_name, flags);
   }

   HRESULT __stdcall SetActionMap(LPDIACTIONFORMATA action_format,
                                  LPCSTR user_name, DWORD flags) noexcept override
   {
      return _device->SetActionMap(action_format, user_name, flags);
   }

   HRESULT __stdcall GetImageInfo(LPDIDEVICEIMAGEINFOHEADERA image_info_out) noexcept override
   {
      return _device->GetImageInfo(image_info_out);
   }

private:
   Device_type _type = Device_type::other;
   Com_ptr<IDirectInputDevice8A> _device;

   std::atomic<ULONG> _ref_count = 1;
};

struct IDirectInput8A_overlay : IDirectInput8A {
   static Com_ptr<IDirectInput8A_overlay> create(HINSTANCE instance, DWORD version) noexcept
   {
      auto dinput_overlay = Com_ptr{new IDirectInput8A_overlay{instance, version}};

      return dinput_overlay;
   }

   IDirectInput8A_overlay(HINSTANCE instance, DWORD version)
   {
      if (FAILED(DirectInput8Create(instance, version, IID_IDirectInput8A,
                                    _dinput.void_clear_and_assign(), nullptr))) {
         log_and_terminate("Failed to create IDirectInput8A interface.");
      }
   }

   HRESULT __stdcall QueryInterface(const IID& iid, void** object) noexcept
   {
      if (iid == IID_IDirectInput8A) {
         AddRef();

         *object = static_cast<IDirectInput8A*>(this);

         return S_OK;
      }
      else if (iid == IID_IUnknown) {
         AddRef();

         *object = static_cast<IUnknown*>(this);

         return S_OK;
      }

      *object = nullptr;

      return E_NOINTERFACE;
   }

   ULONG __stdcall AddRef() noexcept
   {
      return _ref_count += 1;
   }

   ULONG __stdcall Release() noexcept
   {
      const auto ref_count = _ref_count -= 1;

      if (ref_count == 0) delete this;

      return ref_count;
   }

   HRESULT __stdcall CreateDevice(const GUID& guid, LPDIRECTINPUTDEVICE8A* out_device,
                                  LPUNKNOWN unknown_outer) noexcept override
   {
      if (unknown_outer) {
         return _dinput->CreateDevice(guid, out_device, unknown_outer);
      }

      if (out_device && (guid == GUID_SysKeyboard || guid == GUID_SysMouse)) {
         *out_device = IDirectInputDevice8A_overlay::create(guid, *_dinput).release();

         return DI_OK;
      }

      return _dinput->CreateDevice(guid, out_device, unknown_outer);
   }

   HRESULT __stdcall EnumDevices(DWORD device_type, LPDIENUMDEVICESCALLBACKA callback,
                                 LPVOID user_data, DWORD flags) noexcept override
   {
      return _dinput->EnumDevices(device_type, callback, user_data, flags);
   }

   HRESULT __stdcall GetDeviceStatus(const GUID& guid) noexcept override
   {
      return _dinput->GetDeviceStatus(guid);
   }

   HRESULT __stdcall RunControlPanel(HWND owner, DWORD flags) noexcept override
   {
      return _dinput->RunControlPanel(owner, flags);
   }

   HRESULT __stdcall Initialize(HINSTANCE instance, DWORD version) noexcept override
   {
      return _dinput->Initialize(instance, version);
   }

   HRESULT __stdcall FindDevice(const GUID& guid_class, LPCSTR name,
                                LPGUID out_guid_instance) noexcept override
   {
      return _dinput->FindDevice(guid_class, name, out_guid_instance);
   }

   HRESULT __stdcall EnumDevicesBySemantics(LPCSTR user_name,
                                            LPDIACTIONFORMATA action_format,
                                            LPDIENUMDEVICESBYSEMANTICSCBA callback,
                                            LPVOID user_data, DWORD flags) noexcept override
   {

      return _dinput->EnumDevicesBySemantics(user_name, action_format, callback,
                                             user_data, flags);
   }

   HRESULT __stdcall ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK callback,
                                      LPDICONFIGUREDEVICESPARAMSA params,
                                      DWORD flags, LPVOID user_data) noexcept override
   {
      return _dinput->ConfigureDevices(callback, params, flags, user_data);
   }

private:
   Com_ptr<IDirectInput8A> _dinput;

   std::atomic<ULONG> _ref_count = 1;
};

}

}

extern "C" HRESULT WINAPI DirectInput8Create_hook(HINSTANCE hinst, DWORD dwVersion,
                                                  REFIID riidltf, LPVOID* ppvOut,
                                                  LPUNKNOWN punkOuter)
{
   if (sp::user_config.enabled && ppvOut && !punkOuter && riidltf == IID_IDirectInput8A) {
      *ppvOut = sp::IDirectInput8A_overlay::create(hinst, dwVersion).release();

      return DI_OK;
   }

   return DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}
