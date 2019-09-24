

#include "input_hooker.hpp"
#include "com_ptr.hpp"
#include "hook_vtable.hpp"
#include "logger.hpp"
#include "utility.hpp"
#include "window_helpers.hpp"

#include <dinput.h>

#include <array>
#include <atomic>

#include <detours.h>
#include <imgui.h>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                                      WPARAM wParam, LPARAM lParam);

namespace sp {

namespace {

HWND game_window = nullptr;
Input_mode input_mode = Input_mode::normal;

WNDPROC game_window_proc = nullptr;

WPARAM input_hotkey = 0;
Small_function<void() noexcept> input_hotkey_func;
Small_function<void() noexcept> screenshot_func;

LRESULT call_game_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept
{
   return CallWindowProcA(game_window_proc, hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK patch_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept
{
   if (hwnd != game_window) {
      call_game_window_proc(hwnd, msg, wparam, lparam);
   }

   switch (msg) {
   case WM_KEYUP:
   case WM_SYSKEYUP:
      if (wparam == input_hotkey && input_hotkey_func) {
         input_hotkey_func();
      }
      else if (wparam == VK_SNAPSHOT && screenshot_func) {
         screenshot_func();
      }
      break;
   case WM_ACTIVATEAPP:
      if (wparam) {
         ShowWindow(hwnd, SW_NORMAL);
         win32::clip_cursor_to_window(hwnd);
      }
      else {
         ClipCursor(nullptr);
      }
      break;
   case WM_SETCURSOR:
      SetCursor(nullptr);
      return TRUE;
   }

   if (input_mode == Input_mode::imgui) {
      const auto result = ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

      if (result) return result;
   }

   return call_game_window_proc(hwnd, msg, wparam, lparam);
}

// DirectInput 8 Hooks

struct IDirectInputDevice8AHook final : public IDirectInputDevice8A {
   HRESULT __stdcall QueryInterface(const IID& riid, LPVOID* ppvObj) noexcept override
   {
      if (!ppvObj) return E_POINTER;

      if (riid == IID_IDirectInputDevice8A) {
         AddRef();

         *ppvObj = this;
      }
      else if (riid == IID_IUnknown) {
         AddRef();

         *ppvObj = static_cast<IUnknown*>(this);
      }
      else {
         return E_NOINTERFACE;
      }

      return S_OK;
   }

   ULONG __stdcall AddRef() noexcept override
   {
      return _ref_count += 1;
   }

   ULONG __stdcall Release() noexcept override
   {
      const auto ref_count = _ref_count -= 1;

      if (ref_count == 0) delete this;

      return ref_count;
   }

   HRESULT __stdcall GetCapabilities(LPDIDEVCAPS pCaps) noexcept override
   {
      return _actual->GetCapabilities(pCaps);
   }

   HRESULT
   __stdcall EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
                         LPVOID pvRef, DWORD dwFlags) noexcept override
   {
      return _actual->EnumObjects(lpCallback, pvRef, dwFlags);
   }

   HRESULT
   __stdcall GetProperty(const GUID& rguid, LPDIPROPHEADER pPropHdr) noexcept override
   {
      return _actual->GetProperty(rguid, pPropHdr);
   }

   HRESULT
   __stdcall SetProperty(const GUID& rguid, LPCDIPROPHEADER pPropHdr) noexcept override
   {
      return _actual->SetProperty(rguid, pPropHdr);
   }

   HRESULT __stdcall Acquire() noexcept override
   {
      return _actual->Acquire();
   }

   HRESULT __stdcall Unacquire() noexcept override
   {
      return _actual->Unacquire();
   }

   HRESULT __stdcall GetDeviceState(DWORD cbData, LPVOID lpvData) noexcept override
   {
      if (input_mode == Input_mode::imgui) {
         if (_devtype == DI8DEVTYPE_MOUSE && cbData == sizeof(DIMOUSESTATE2)) {
            DIMOUSESTATE2 state{};

            _actual->GetDeviceState(sizeof(state), &state);

            auto& io = ImGui::GetIO();

            std::transform(std::begin(state.rgbButtons),
                           std::begin(state.rgbButtons) +
                              safe_min(std::distance(std::begin(io.MouseDown),
                                                     std::end(io.MouseDown)),
                                       std::distance(std::begin(state.rgbButtons),
                                                     std::end(state.rgbButtons))),
                           std::begin(io.MouseDown),
                           [](const auto v) { return v != 0; });

            io.MouseWheel += static_cast<float>(state.lZ);
         }

         std::memset(lpvData, 0, cbData);

         return DI_OK;
      }

      if (const auto result = _actual->GetDeviceState(cbData, lpvData); FAILED(result)) {
         return result;
      }
      else {
         if (_devtype == DI8DEVTYPE_KEYBOARD &&
             cbData >= sizeof(std::array<std::byte, 256>)) {
            static_cast<std::byte*>(lpvData)[DIK_SYSRQ] = std::byte{0x0};
         }

         return result;
      }
   }

   HRESULT
   __stdcall GetDeviceData(DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod,
                           LPDWORD pdwInOut, DWORD dwFlags) noexcept override
   {
      return _actual->GetDeviceData(cbObjectData, rgdod, pdwInOut, dwFlags);
   }

   HRESULT __stdcall SetDataFormat(LPCDIDATAFORMAT lpdf) noexcept override
   {
      return _actual->SetDataFormat(lpdf);
   }

   HRESULT __stdcall SetEventNotification(HANDLE hEvent) noexcept override
   {
      return _actual->SetEventNotification(hEvent);
   }

   HRESULT __stdcall SetCooperativeLevel(HWND hwnd, DWORD dwFlags) noexcept override
   {
      if (_devtype == DI8DEVTYPE_MOUSE) {
         return _actual->SetCooperativeLevel(hwnd, dwFlags);
      }
      else {
         return _actual->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
      }
   }

   HRESULT
   __stdcall GetObjectInfo(LPDIDEVICEOBJECTINSTANCE pdidoi, DWORD dwObj,
                           DWORD dwHow) noexcept override
   {
      return _actual->GetObjectInfo(pdidoi, dwObj, dwHow);
   }

   HRESULT
   __stdcall GetDeviceInfo(LPDIDEVICEINSTANCE pdidi) noexcept override
   {
      return _actual->GetDeviceInfo(pdidi);
   }

   HRESULT __stdcall RunControlPanel(HWND hwnd, DWORD dwFlags) noexcept override
   {
      return _actual->RunControlPanel(hwnd, dwFlags);
   }

   HRESULT
   __stdcall Initialize(HINSTANCE hinst, DWORD dwVersion, const GUID& rguid) noexcept override
   {
      return _actual->Initialize(hinst, dwVersion, rguid);
   }

   HRESULT
   __stdcall CreateEffect(const GUID& rguid, LPCDIEFFECT lpeff,
                          LPDIRECTINPUTEFFECT* ppdeff, LPUNKNOWN punkOuter) noexcept override
   {
      return _actual->CreateEffect(rguid, lpeff, ppdeff, punkOuter);
   }

   HRESULT
   __stdcall EnumEffects(LPDIENUMEFFECTSCALLBACKA lpCallback, LPVOID pvRef,
                         DWORD dwEffType) noexcept override
   {
      return _actual->EnumEffects(lpCallback, pvRef, dwEffType);
   }

   HRESULT
   __stdcall GetEffectInfo(LPDIEFFECTINFOA pdei, const GUID& rguid) noexcept override
   {
      return _actual->GetEffectInfo(pdei, rguid);
   }

   HRESULT __stdcall GetForceFeedbackState(LPDWORD pdwOut) noexcept override
   {
      return _actual->GetForceFeedbackState(pdwOut);
   }

   HRESULT __stdcall SendForceFeedbackCommand(DWORD dwFlags) noexcept override
   {
      return _actual->SendForceFeedbackCommand(dwFlags);
   }

   HRESULT
   __stdcall EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
                                      LPVOID pvRef, DWORD fl) noexcept override
   {
      return _actual->EnumCreatedEffectObjects(lpCallback, pvRef, fl);
   }

   HRESULT __stdcall Escape(LPDIEFFESCAPE pesc) noexcept override
   {
      return _actual->Escape(pesc);
   }

   HRESULT __stdcall Poll() noexcept override
   {
      return _actual->Poll();
   }

   HRESULT
   __stdcall SendDeviceData(DWORD cbObjectData, LPCDIDEVICEOBJECTDATA rgdod,
                            LPDWORD pdwInOut, DWORD fl) noexcept override
   {
      return _actual->SendDeviceData(cbObjectData, rgdod, pdwInOut, fl);
   }

   HRESULT
   __stdcall EnumEffectsInFile(LPCSTR lpszFileName, LPDIENUMEFFECTSINFILECALLBACK pec,
                               LPVOID pvRef, DWORD dwFlags) noexcept override
   {
      return _actual->EnumEffectsInFile(lpszFileName, pec, pvRef, dwFlags);
   }

   HRESULT __stdcall WriteEffectToFile(LPCSTR lpszFileName, DWORD dwEntries,
                                       LPDIFILEEFFECT rgDiFileEft,
                                       DWORD dwFlags) noexcept override
   {
      return _actual->WriteEffectToFile(lpszFileName, dwEntries, rgDiFileEft, dwFlags);
   }

   HRESULT
   __stdcall BuildActionMap(LPDIACTIONFORMAT lpdiaf, LPCSTR lpszUserName,
                            DWORD dwFlags) noexcept override
   {
      return _actual->BuildActionMap(lpdiaf, lpszUserName, dwFlags);
   }

   HRESULT
   __stdcall SetActionMap(LPDIACTIONFORMATA lpdiActionFormat,
                          LPCSTR lptszUserName, DWORD dwFlags) noexcept override
   {
      return _actual->SetActionMap(lpdiActionFormat, lptszUserName, dwFlags);
   }

   HRESULT
   __stdcall GetImageInfo(LPDIDEVICEIMAGEINFOHEADER lpdiDevImageInfoHeader) noexcept override
   {
      return _actual->GetImageInfo(lpdiDevImageInfoHeader);
   }

   static auto create(Com_ptr<IDirectInputDevice8A> actual)
      -> Com_ptr<IDirectInputDevice8AHook>
   {
      return Com_ptr{new IDirectInputDevice8AHook{std::move(actual)}};
   }

   IDirectInputDevice8AHook(const IDirectInputDevice8AHook&) = delete;
   IDirectInputDevice8AHook& operator=(const IDirectInputDevice8AHook&) = delete;
   IDirectInputDevice8AHook(IDirectInputDevice8AHook&&) = delete;
   IDirectInputDevice8AHook& operator=(IDirectInputDevice8AHook&&) = delete;

private:
   IDirectInputDevice8AHook(Com_ptr<IDirectInputDevice8A> actual)
      : _actual{std::move(actual)}
   {
   }

   ~IDirectInputDevice8AHook() = default;

   std::atomic<UINT> _ref_count;
   const Com_ptr<IDirectInputDevice8A> _actual;
   const DWORD _devtype = [this] {
      DIDEVICEINSTANCEA dev{.dwSize = sizeof(DIDEVICEINSTANCEA)};

      _actual->GetDeviceInfo(&dev);

      return GET_DIDEVICE_TYPE(dev.dwDevType);
   }();
};

struct IDirectInput8AHook final : public IDirectInput8A {
   HRESULT __stdcall QueryInterface(const IID& riid, LPVOID* ppvObj) noexcept override
   {
      if (!ppvObj) return E_POINTER;

      if (riid == IID_IDirectInput8A) {
         AddRef();

         *ppvObj = this;
      }
      else if (riid == IID_IUnknown) {
         AddRef();

         *ppvObj = static_cast<IUnknown*>(this);
      }
      else {
         return E_NOINTERFACE;
      }

      return S_OK;
   }

   ULONG __stdcall AddRef() noexcept override
   {
      return _ref_count += 1;
   }

   ULONG __stdcall Release() noexcept override
   {
      const auto ref_count = _ref_count -= 1;

      if (ref_count == 0) delete this;

      return ref_count;
   }

   HRESULT
   __stdcall CreateDevice(const GUID& rguid, LPDIRECTINPUTDEVICE8A* lplpDirectInputDevice,
                          LPUNKNOWN pUnkOuter) noexcept override
   {
      if (!lplpDirectInputDevice) return DIERR_INVALIDPARAM;

      if (rguid == GUID_SysKeyboard || rguid == GUID_SysMouse && !pUnkOuter) {
         Com_ptr<IDirectInputDevice8A> actual;

         if (const auto result =
                _actual->CreateDevice(rguid, actual.clear_and_assign(), nullptr);
             FAILED(result)) {
            return result;
         }

         *lplpDirectInputDevice =
            IDirectInputDevice8AHook::create(std::move(actual)).release();

         return DI_OK;
      }

      return _actual->CreateDevice(rguid, lplpDirectInputDevice, pUnkOuter);
   }

   HRESULT
   __stdcall EnumDevices(DWORD dwDevType, LPDIENUMDEVICESCALLBACKA lpCallback,
                         LPVOID pvRef, DWORD dwFlags) noexcept override
   {
      return _actual->EnumDevices(dwDevType, lpCallback, pvRef, dwFlags);
   }

   HRESULT
   __stdcall GetDeviceStatus(const GUID& rguidInstance) noexcept override
   {
      return _actual->GetDeviceStatus(rguidInstance);
   }

   HRESULT
   __stdcall RunControlPanel(HWND hwnd, DWORD dwFlags) noexcept override
   {
      return _actual->RunControlPanel(hwnd, dwFlags);
   }

   HRESULT
   __stdcall Initialize(HINSTANCE hinst, DWORD dwVersion) noexcept override
   {
      return _actual->Initialize(hinst, dwVersion);
   }

   HRESULT
   __stdcall FindDevice(const GUID& rguidClass, LPCSTR pszName,
                        LPGUID pguidInstance) noexcept override
   {
      return _actual->FindDevice(rguidClass, pszName, pguidInstance);
   }

   HRESULT
   __stdcall EnumDevicesBySemantics(LPCSTR pszUserName, LPDIACTIONFORMAT lpdiActionFormat,
                                    LPDIENUMDEVICESBYSEMANTICSCBA lpCallback,
                                    LPVOID pvRef, DWORD dwFlags) noexcept override
   {
      return _actual->EnumDevicesBySemantics(pszUserName, lpdiActionFormat,
                                             lpCallback, pvRef, dwFlags);
   }

   HRESULT
   __stdcall ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
                              LPDICONFIGUREDEVICESPARAMS lpdiCDParams,
                              DWORD dwFlags, LPVOID pvRefData) noexcept override
   {
      return _actual->ConfigureDevices(lpdiCallback, lpdiCDParams, dwFlags, pvRefData);
   }

   static auto create(Com_ptr<IDirectInput8A> actual) -> Com_ptr<IDirectInput8AHook>
   {
      return Com_ptr{new IDirectInput8AHook{std::move(actual)}};
   }

   IDirectInput8AHook(const IDirectInput8AHook&) = delete;
   IDirectInput8AHook& operator=(const IDirectInput8AHook&) = delete;
   IDirectInput8AHook(IDirectInput8AHook&&) = delete;
   IDirectInput8AHook& operator=(IDirectInput8AHook&&) = delete;

private:
   IDirectInput8AHook(Com_ptr<IDirectInput8A> actual)
      : _actual{std::move(actual)}
   {
   }

   ~IDirectInput8AHook() = default;

   std::atomic<UINT> _ref_count;
   const Com_ptr<IDirectInput8A> _actual;
};

decltype(&DirectInput8Create) true_DirectInput8Create = &DirectInput8Create;

HRESULT WINAPI DirectInput8Create_hook(HINSTANCE hinst, DWORD dwVersion,
                                       const IID& riidltf, LPVOID* ppvOut,
                                       LPUNKNOWN punkOuter)
{
   if (!ppvOut) return DIERR_INVALIDPARAM;

   if (riidltf == IID_IDirectInput8A && !punkOuter) {
      Com_ptr<IDirectInput8A> actual;

      if (const auto result =
             true_DirectInput8Create(hinst, dwVersion, IID_IDirectInput8A,
                                     actual.void_clear_and_assign(), nullptr);
          FAILED(result)) {
         return result;
      }

      *ppvOut = IDirectInput8AHook::create(std::move(actual)).release();

      return S_OK;
   }

   return true_DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

}

void install_dinput_hooks() noexcept
{
   bool failure = true;

   if (DetourTransactionBegin() == NO_ERROR) {
      if (DetourAttach(&reinterpret_cast<PVOID&>(true_DirectInput8Create),
                       DirectInput8Create_hook) == NO_ERROR) {
         if (DetourTransactionCommit() == NO_ERROR) {
            failure = false;
         }
      }
   }

   if (failure) {
      log(Log_level::error,
          "Failed to install DirectInput 8 hooks. Developer Screen may not function and pressing 'Print Screen' may crash the game."sv);
   }
}

void install_window_hooks(const HWND window) noexcept
{
   Expects(window);

   if (game_window) {
      log_and_terminate("Attempt to install window hooks twice.");
   }

   game_window = window;
   game_window_proc = reinterpret_cast<WNDPROC>(
      SetWindowLongPtrA(window, GWLP_WNDPROC,
                        reinterpret_cast<LONG_PTR>(&patch_window_proc)));

   if (!game_window_proc) {
      log(Log_level::error,
          "Failed to get install patch window procedure. Developer Screen will not function."sv);

      return;
   }
}

void set_input_mode(const Input_mode mode) noexcept
{
   input_mode = mode;
}

void set_input_hotkey(const WPARAM hotkey) noexcept
{
   input_hotkey = hotkey;
}

void set_input_hotkey_func(Small_function<void() noexcept> function) noexcept
{
   input_hotkey_func = std::move(function);
}

void set_input_screenshot_func(Small_function<void() noexcept> function) noexcept
{
   screenshot_func = std::move(function);
}
}
