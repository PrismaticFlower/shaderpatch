

#include "input_hooker.hpp"
#include "com_ptr.hpp"
#include "hook_vtable.hpp"
#include "logger.hpp"
#include "window_helpers.hpp"

#include <dinput.h>

#include <array>

#include <imgui.h>

IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                                 WPARAM wParam, LPARAM lParam);

namespace sp {

namespace {

constexpr auto window_name = L"Shader Patch Input Window";
constexpr auto window_class_name = L"Shader Patch Input Window Class";

HWND g_window = nullptr;
HHOOK g_get_message_hook = nullptr;
HHOOK g_wnd_proc_hook = nullptr;
Input_mode g_input_mode = Input_mode::normal;

WPARAM g_hotkey = 0;
std::function<void()> g_hotkey_func;

LRESULT CALLBACK get_message_hook(int code, WPARAM w_param, LPARAM l_param)
{
   MSG& msg = *reinterpret_cast<MSG*>(l_param);

   // hotkey handling
   switch (msg.message) {
   case WM_KEYUP:
   case WM_SYSKEYUP:
      if (msg.wParam == g_hotkey && g_hotkey_func) {
         g_hotkey_func();
      }
   }

   if (code < 0 || w_param == PM_NOREMOVE || msg.hwnd != g_window ||
       g_input_mode == Input_mode::normal) {
      return CallNextHookEx(g_get_message_hook, code, w_param, l_param);
   }

   TranslateMessage(&msg);

   // input redirection handling
   switch (msg.message) {
   case WM_LBUTTONDOWN:
   case WM_LBUTTONDBLCLK:
   case WM_RBUTTONDOWN:
   case WM_RBUTTONDBLCLK:
   case WM_MBUTTONDOWN:
   case WM_MBUTTONDBLCLK:
   case WM_LBUTTONUP:
   case WM_RBUTTONUP:
   case WM_MBUTTONUP:
   case WM_MOUSEWHEEL:
   case WM_MOUSEHWHEEL:
   case WM_MOUSEMOVE:
   case WM_KEYDOWN:
   case WM_SYSKEYDOWN:
   case WM_KEYUP:
   case WM_SYSKEYUP:
   case WM_CHAR:
      ImGui_ImplWin32_WndProcHandler(g_window, msg.message, msg.wParam, msg.lParam);
      msg.message = WM_NULL;
   }

   return CallNextHookEx(g_get_message_hook, code, w_param, l_param);
}

LRESULT CALLBACK wnd_proc_hook(int code, WPARAM w_param, LPARAM l_param)
{
   auto& msg = *reinterpret_cast<CWPSTRUCT*>(l_param);

   const volatile auto val = msg.message;

   if (msg.hwnd != g_window) {
      return CallNextHookEx(g_wnd_proc_hook, code, w_param, l_param);
   }

   // focus handling
   if (msg.message == WM_ACTIVATEAPP || msg.message == WM_ACTIVATE) {
      if (msg.wParam) {
         clip_cursor_to_window(msg.hwnd);
      }
      else {
         ClipCursor(nullptr);
      }
   }

   return CallNextHookEx(g_wnd_proc_hook, code, w_param, l_param);
}

decltype(&DirectInput8Create) get_directinput8_create() noexcept
{
   static decltype(&DirectInput8Create) proc = nullptr;

   if (proc) return proc;

   const static auto handle = LoadLibraryW(L"dinput8.dll");

   if (!handle) std::terminate();

   proc = reinterpret_cast<decltype(&DirectInput8Create)>(
      GetProcAddress(handle, "DirectInput8Create"));

   if (!proc) std::terminate();

   return proc;
}

void install_get_message_hook(const DWORD thread_id) noexcept
{
   if (g_get_message_hook != nullptr) return;

   g_get_message_hook =
      SetWindowsHookExW(WH_GETMESSAGE, &get_message_hook, nullptr, thread_id);

   if (!g_get_message_hook) {
      log_and_terminate("Failed to install WH_GETMESSAGE hook.");
   }
}

void install_dinput_hooks() noexcept
{
   static bool installed = false;

   if (installed) return;

   installed = true;

   static_assert(DIRECTINPUT_VERSION == 0x0800,
                 "Unexpected DirectInput version. Device vtables hooked may be "
                 "different than the devices used by the game.");

   Com_ptr<IDirectInput8A> dinput;

   get_directinput8_create()(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION,
                             IID_IDirectInput8A, dinput.void_clear_and_assign(),
                             nullptr);

   using Aquire_fn = HRESULT __stdcall(IDirectInputDevice8A & self);

   using Get_device_state_fn =
      HRESULT __stdcall(IDirectInputDevice8A & self, DWORD data_size, LPVOID data);

   // Hook System Keyboard Device
   {
      Com_ptr<IDirectInputDevice8A> device;

      dinput->CreateDevice(GUID_SysKeyboard, device.clear_and_assign(), nullptr);

      static Aquire_fn* true_keyboard_aquire = nullptr;

      true_keyboard_aquire =
         hook_vtable<Aquire_fn>(*device, 7, [](IDirectInputDevice8A& self) {
            self.SetCooperativeLevel(g_window, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

            return true_keyboard_aquire(self);
         });

      static Get_device_state_fn* true_get_keyboard_device_state = nullptr;

      true_get_keyboard_device_state = hook_vtable<Get_device_state_fn>(
         *device, 9, [](IDirectInputDevice8A& self, DWORD data_size, LPVOID data) {
            if (g_input_mode == Input_mode::normal) {
               return true_get_keyboard_device_state(self, data_size, data);
            }

            std::memset(data, 0, data_size);

            return DI_OK;
         });
   }

   // Hook System Mouse Device
   // {
   //    Com_ptr<IDirectInputDevice8A> device;
   //
   //    dinput->CreateDevice(GUID_SysMouse, device.clear_and_assign(), nullptr);
   //
   //    static Set_coop_level_type* true_set_mouse_coop_level = nullptr;
   //
   //    true_set_mouse_coop_level = hook_vtable<Set_coop_level_type>(
   //       *device, 13, [](IDirectInputDevice8A& self, HWND window, DWORD) {
   //          return true_set_mouse_coop_level(self, window,
   //                                           DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
   //       });
   //
   //    static Get_device_state_type* true_get_mouse_device_state = nullptr;
   //
   //    true_get_mouse_device_state = hook_vtable<Get_device_state_type>(
   //       *device, 9, [](IDirectInputDevice8A& self, DWORD data_size, LPVOID data) {
   //          if (g_input_mode == Input_mode::normal) {
   //             return true_get_mouse_device_state(self, data_size, data);
   //          }
   //
   //          std::memset(data, 0, data_size);
   //
   //          return DI_OK;
   //       });
   // }
}
}

void initialize_input_hooks(const DWORD thread_id) noexcept
{
   static bool hooked = false;

   if (std::exchange(hooked, true)) {
      log_and_terminate("Attempt to install input hooks twice.");
   }

   install_get_message_hook(thread_id);
   install_dinput_hooks();
}

void set_input_window(const DWORD thread_id, const HWND window) noexcept
{
   g_window = window;

   if (g_wnd_proc_hook) {
      UnhookWindowsHookEx(g_wnd_proc_hook);

      g_wnd_proc_hook = nullptr;
   }

   g_wnd_proc_hook =
      SetWindowsHookExW(WH_CALLWNDPROC, &wnd_proc_hook, nullptr, thread_id);

   if (GetFocus() == window) {
      clip_cursor_to_window(window);
   }
}

void set_input_mode(const Input_mode mode) noexcept
{
   g_input_mode = mode;
}

void set_input_hotkey(const WPARAM hotkey) noexcept
{
   g_hotkey = hotkey;
}

void set_input_hotkey_func(std::function<void()> function) noexcept
{
   g_hotkey_func = std::move(function);
}
}
