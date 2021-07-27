

#include "input_hooker.hpp"
#include "com_ptr.hpp"
#include "hook_vtable.hpp"
#include "logger.hpp"
#include "utility.hpp"
#include "window_helpers.hpp"

#include <dinput.h>

#include <array>

#include "imgui/imgui.h"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                                      WPARAM wParam, LPARAM lParam);

using namespace std::literals;

namespace sp {

namespace {

HWND game_window = nullptr;
Input_mode input_mode = Input_mode::normal;
HHOOK get_message_hhook = nullptr;
HHOOK wnd_proc_hhook = nullptr;

WPARAM input_hotkey = 0;
Small_function<void() noexcept> input_hotkey_func;
Small_function<void() noexcept> screenshot_func;

LRESULT CALLBACK get_message_hook(int code, WPARAM w_param, LPARAM l_param)
{
   MSG& msg = *reinterpret_cast<MSG*>(l_param);

   if (code < 0 || w_param == PM_NOREMOVE || msg.hwnd != game_window) {
      return CallNextHookEx(get_message_hhook, code, w_param, l_param);
   }

   switch (msg.message) {
   case WM_KEYUP:
      if (const auto key = msg.wParam; key == input_hotkey && input_hotkey_func) {
         input_hotkey_func();
      }
      else if (key == VK_SNAPSHOT && screenshot_func) {
         screenshot_func();
      }

      [[fallthrough]];
   case WM_KEYDOWN:
   case WM_SYSKEYUP:
   case WM_SYSKEYDOWN:
   case WM_CHAR:
      if (input_mode == Input_mode::normal) break;

      ImGui_ImplWin32_WndProcHandler(game_window, msg.message, msg.wParam, msg.lParam);
   }

   return CallNextHookEx(get_message_hhook, code, w_param, l_param);
}

LRESULT CALLBACK wnd_proc_hook(int code, WPARAM w_param, LPARAM l_param)
{
   auto& msg = *reinterpret_cast<CWPRETSTRUCT*>(l_param);

   if (msg.hwnd != game_window) {
      return CallNextHookEx(wnd_proc_hhook, code, w_param, l_param);
   }

   if (msg.message == WM_ACTIVATEAPP) {
      if (msg.wParam) {
         ShowWindow(msg.hwnd, SW_NORMAL);
         win32::clip_cursor_to_window(msg.hwnd);
      }
      else {
         ClipCursor(nullptr);
      }
   }

   return CallNextHookEx(wnd_proc_hhook, code, w_param, l_param);
}

// DirectInput 8 Hooks

DWORD get_device_type(IDirectInputDevice8A& device)
{
   DIDEVICEINSTANCEA dev{.dwSize = sizeof(DIDEVICEINSTANCEA)};

   device.GetDeviceInfo(&dev);

   return GET_DIDEVICE_TYPE(dev.dwDevType);
}

using DirectInput8A_GetDeviceStateFunc = HRESULT __stdcall(IDirectInputDevice8A* const,
                                                           DWORD, LPVOID);
using DirectInput8A_SetCooperativeLevelFunc = HRESULT __stdcall(IDirectInputDevice8A* const,
                                                                HWND, DWORD);

DirectInput8A_GetDeviceStateFunc* Keyboard_DirectInput8A_GetDeviceStateTrue = nullptr;
DirectInput8A_SetCooperativeLevelFunc* Keyboard_DirectInput8A_SetCooperativeLevelTrue =
   nullptr;
DirectInput8A_GetDeviceStateFunc* Mouse_DirectInput8A_GetDeviceStateTrue = nullptr;
DirectInput8A_SetCooperativeLevelFunc* Mouse_DirectInput8A_SetCooperativeLevelTrue =
   nullptr;

HRESULT __stdcall DirectInput8A_GetDeviceStateHook(
   IDirectInputDevice8A* const self, DWORD cbData, LPVOID lpvData,
   DirectInput8A_GetDeviceStateFunc& GetDeviceStateTrue) noexcept
{
   const auto devtype = get_device_type(*self);

   if (input_mode == Input_mode::imgui) {
      if (devtype == DI8DEVTYPE_MOUSE && cbData == sizeof(DIMOUSESTATE2)) {
         DIMOUSESTATE2 state{};

         GetDeviceStateTrue(self, sizeof(state), &state);

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

   if (const auto result = GetDeviceStateTrue(self, cbData, lpvData); FAILED(result)) {
      return result;
   }
   else {
      if (devtype == DI8DEVTYPE_KEYBOARD &&
          cbData >= sizeof(std::array<std::byte, 256>)) {
         static_cast<std::byte*>(lpvData)[DIK_SYSRQ] = std::byte{0x0};
      }

      return result;
   }
}

HRESULT __stdcall DirectInput8A_SetCooperativeLevelHook(
   IDirectInputDevice8A* const self, HWND hwnd, DWORD dwFlags,
   DirectInput8A_SetCooperativeLevelFunc& SetCooperativeLevelTrue) noexcept
{
   const auto devtype = get_device_type(*self);

   if (devtype == DI8DEVTYPE_MOUSE) {
      return SetCooperativeLevelTrue(self, hwnd, dwFlags);
   }
   else {
      return SetCooperativeLevelTrue(self, hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
   }
}

void hook_dinput8_device(IDirectInput8A& dinput) noexcept
{
   // Hook System Keyboard Device
   Com_ptr<IDirectInputDevice8A> device;
   dinput.CreateDevice(GUID_SysKeyboard, device.clear_and_assign(), nullptr);

   DirectInput8A_GetDeviceStateFunc* const Keyboard_DirectInput8A_GetDeviceStateHook =
      [](IDirectInputDevice8A* const self, DWORD cbData, LPVOID lpvData) noexcept {
         return DirectInput8A_GetDeviceStateHook(self, cbData, lpvData,
                                                 *Keyboard_DirectInput8A_GetDeviceStateTrue);
      };

   DirectInput8A_SetCooperativeLevelFunc* const Keyboard_DirectInput8A_SetCooperativeLevelHook =
      [](IDirectInputDevice8A* const self, HWND hwnd, DWORD dwFlags) noexcept {
         return DirectInput8A_SetCooperativeLevelHook(self, hwnd, dwFlags,
                                                      *Keyboard_DirectInput8A_SetCooperativeLevelTrue);
      };

   Keyboard_DirectInput8A_GetDeviceStateTrue =
      hook_vtable(*device, 9, Keyboard_DirectInput8A_GetDeviceStateHook);
   Keyboard_DirectInput8A_SetCooperativeLevelTrue =
      hook_vtable(*device, 13, Keyboard_DirectInput8A_SetCooperativeLevelHook);

   // Hook System Mouse Device
   dinput.CreateDevice(GUID_SysMouse, device.clear_and_assign(), nullptr);

   DirectInput8A_GetDeviceStateFunc* const Mouse_DirectInput8A_GetDeviceStateHook =
      [](IDirectInputDevice8A* const self, DWORD cbData, LPVOID lpvData) noexcept {
         return DirectInput8A_GetDeviceStateHook(self, cbData, lpvData,
                                                 *Mouse_DirectInput8A_GetDeviceStateTrue);
      };

   DirectInput8A_SetCooperativeLevelFunc* const Mouse_DirectInput8A_SetCooperativeLevelHook =
      [](IDirectInputDevice8A* const self, HWND hwnd, DWORD dwFlags) noexcept {
         return DirectInput8A_SetCooperativeLevelHook(self, hwnd, dwFlags,
                                                      *Mouse_DirectInput8A_SetCooperativeLevelTrue);
      };

   if (auto* const existing_func =
          get_vfunc_pointer<DirectInput8A_GetDeviceStateFunc>(*device, 9);
       existing_func != Keyboard_DirectInput8A_GetDeviceStateHook) {
      Mouse_DirectInput8A_GetDeviceStateTrue =
         hook_vtable(*device, 9, Mouse_DirectInput8A_GetDeviceStateHook);
   }

   if (auto* const existing_func =
          get_vfunc_pointer<DirectInput8A_SetCooperativeLevelFunc>(*device, 13);
       existing_func != Keyboard_DirectInput8A_SetCooperativeLevelHook) {
      Mouse_DirectInput8A_SetCooperativeLevelTrue =
         hook_vtable(*device, 13, Mouse_DirectInput8A_SetCooperativeLevelHook);
   }
}

}

void install_dinput_hooks() noexcept
{
   static_assert(DIRECTINPUT_VERSION == 0x0800,
                 "Unexpected DirectInput version. Device vtables hooked may be "
                 "different than the devices used by the game.");

   static bool installed = false;

   if (std::exchange(installed, true)) {

      log_and_terminate("Attempt to install DirectInput 8 hooks twice.");
   }

   Com_ptr<IDirectInput8A> dinput;

   if (const auto result =
          DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION,
                             IID_IDirectInput8A, dinput.void_clear_and_assign(), nullptr);
       FAILED(result)) {
      log(Log_level::error,
          "Failed to install DirectInput 8 hooks. Developer Screen may not function."sv);

      return;
   }

   hook_dinput8_device(*dinput);
}

void install_window_hooks(const HWND window) noexcept
{
   Expects(window);

   if (std::exchange(game_window, window)) {
      log_and_terminate("Attempt to install window hooks twice.");
   }

   const auto threadid = GetCurrentThreadId();

   wnd_proc_hhook =
      SetWindowsHookExW(WH_CALLWNDPROCRET, &wnd_proc_hook, nullptr, threadid);

   if (!wnd_proc_hhook) {
      log(Log_level::error,
          "Failed to get install game window procedure hook. Developer Screen may not function."sv);

      return;
   }

   get_message_hhook =
      SetWindowsHookExW(WH_GETMESSAGE, &get_message_hook, nullptr, threadid);

   if (!get_message_hhook) {
      log(Log_level::error, "Failed to game window message hook. Developer "
                            "Screen will not function");

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
