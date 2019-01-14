

#include "input_hooker.hpp"
#include "com_ptr.hpp"
#include "hook_vtable.hpp"
#include "logger.hpp"
#include "window_helpers.hpp"

#include <dinput.h>

#include <array>

#include <imgui.h>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                                      WPARAM wParam, LPARAM lParam);

namespace sp {

namespace {

HWND g_window = nullptr;
HHOOK g_get_message_hook = nullptr;
HHOOK g_wnd_proc_hook = nullptr;
Input_mode g_input_mode = Input_mode::normal;

WPARAM g_hotkey = 0;
Small_function<void() noexcept> g_hotkey_func;

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
   auto& msg = *reinterpret_cast<CWPRETSTRUCT*>(l_param);

   if (msg.hwnd != g_window) {
      return CallNextHookEx(g_wnd_proc_hook, code, w_param, l_param);
   }

   if (g_input_mode != Input_mode::normal && msg.message == WM_SETCURSOR) {
      ImGui_ImplWin32_WndProcHandler(msg.hwnd, msg.message, msg.wParam, msg.lParam);
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
   Expects(g_window);

   static bool installed = false;

   if (std::exchange(installed, true)) return;

   static_assert(DIRECTINPUT_VERSION == 0x0800,
                 "Unexpected DirectInput version. Device vtables hooked may be "
                 "different than the devices used by the game.");

   Com_ptr<IDirectInput8A> dinput;

   get_directinput8_create()(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION,
                             IID_IDirectInput8A, dinput.void_clear_and_assign(),
                             nullptr);

   using Set_coop_level_fn = HRESULT __stdcall(IDirectInputDevice8A&, HWND, DWORD);

   using Poll_fn = HRESULT __stdcall(IDirectInputDevice8A&);

   using Get_device_state_fn = HRESULT __stdcall(IDirectInputDevice8A&, DWORD, LPVOID);

   const std::uintptr_t* keyboard_vtable;

   // Hook System Keyboard Device
   {
      Com_ptr<IDirectInputDevice8A> device;

      dinput->CreateDevice(GUID_SysKeyboard, device.clear_and_assign(), nullptr);

      keyboard_vtable = get_vtable_pointer(*device);

      static Set_coop_level_fn* true_set_coop_level = nullptr;

      true_set_coop_level = hook_vtable<Set_coop_level_fn>(
         *device, 13, [](IDirectInputDevice8A& self, HWND window, DWORD) {
            return true_set_coop_level(self, window,
                                       DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
         });

      static Poll_fn* true_poll = nullptr;

      true_poll = hook_vtable<Poll_fn>(*device, 25, [](IDirectInputDevice8A& self) {
         static int coop_overrides_left = 2;

         if (coop_overrides_left != 0) {
            --coop_overrides_left;
            self.Unacquire();
            true_set_coop_level(self, g_window, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
         }

         return true_poll(self);
      });

      static Get_device_state_fn* true_get_device_state = nullptr;

      true_get_device_state = hook_vtable<Get_device_state_fn>(
         *device, 9, [](IDirectInputDevice8A& self, DWORD data_size, LPVOID data) {
            if (g_input_mode == Input_mode::normal) {
               return true_get_device_state(self, data_size, data);
            }

            std::memset(data, 0, data_size);

            return DI_OK;
         });
   }

   // Hook System Mouse Device
   {
      Com_ptr<IDirectInputDevice8A> device;

      dinput->CreateDevice(GUID_SysMouse, device.clear_and_assign(), nullptr);

      if (keyboard_vtable == get_vtable_pointer(*device)) return;

      static Set_coop_level_fn* true_set_coop_level = nullptr;

      true_set_coop_level = hook_vtable<Set_coop_level_fn>(
         *device, 13, [](IDirectInputDevice8A& self, HWND window, DWORD) {
            return true_set_coop_level(self, window,
                                       DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
         });

      static Poll_fn* true_poll = nullptr;

      true_poll = hook_vtable<Poll_fn>(*device, 25, [](IDirectInputDevice8A& self) {
         static bool coop_level_overridden = false;

         if (!std::exchange(coop_level_overridden, true)) {
            self.Unacquire();
            true_set_coop_level(self, g_window, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
         }

         return true_poll(self);
      });

      static Get_device_state_fn* true_get_device_state = nullptr;

      true_get_device_state = hook_vtable<Get_device_state_fn>(
         *device, 9, [](IDirectInputDevice8A& self, DWORD data_size, LPVOID data) {
            if (g_input_mode == Input_mode::normal) {
               return true_get_device_state(self, data_size, data);
            }

            std::memset(data, 0, data_size);

            return DI_OK;
         });
   }
}

void install_winndow_hook(const DWORD thread_id) noexcept
{
   Expects(g_window);
   Expects(!g_wnd_proc_hook);

   g_wnd_proc_hook =
      SetWindowsHookExW(WH_CALLWNDPROCRET, &wnd_proc_hook, nullptr, thread_id);
}
}

void initialize_input_hooks(const DWORD thread_id, const HWND window) noexcept
{
   Expects(window);

   static bool hooked = false;

   if (std::exchange(hooked, true)) {
      log_and_terminate("Attempt to install input hooks twice.");
   }

   g_window = window;

   install_get_message_hook(thread_id);
   install_dinput_hooks();
   install_winndow_hook(thread_id);
}

void set_input_mode(const Input_mode mode) noexcept
{
   g_input_mode = mode;
}

void set_input_hotkey(const WPARAM hotkey) noexcept
{
   g_hotkey = hotkey;
}

void set_input_hotkey_func(Small_function<void() noexcept> function) noexcept
{
   g_hotkey_func = std::move(function);
}
}
