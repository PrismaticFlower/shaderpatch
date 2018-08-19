

#include "input_hooker.hpp"
#include "com_ptr.hpp"
#include "hook_vtable.hpp"
#include "window_helpers.hpp"

#include <dinput.h>

#include <array>

#include <imgui.h>

IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                                 WPARAM wParam, LPARAM lParam);

namespace sp {

namespace {

HWND g_window = nullptr;
HHOOK g_hook = nullptr;
HHOOK g_wnd_proc_hook = nullptr;
Input_mode g_input_mode = Input_mode::normal;

WPARAM g_hotkey = 0;
std::function<void()> g_hotkey_func;

using Set_coop_level_type = HRESULT __stdcall(IDirectInputDevice8A& self,
                                              HWND window, DWORD level);

using Get_device_state_type = HRESULT __stdcall(IDirectInputDevice8A& self,
                                                DWORD data_size, LPVOID data);

Set_coop_level_type* g_actual_set_coop_level = nullptr;
Get_device_state_type* g_actual_get_device_state = nullptr;

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
      return CallNextHookEx(g_hook, code, w_param, l_param);
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

   return CallNextHookEx(g_hook, code, w_param, l_param);
}

LRESULT CALLBACK wnd_proc_hook(int code, WPARAM w_param, LPARAM l_param)
{
   auto& msg = *reinterpret_cast<CWPSTRUCT*>(l_param);

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

HRESULT __stdcall set_cooperative_Level_hook(IDirectInputDevice8A& self,
                                             HWND window, DWORD)
{
   return g_actual_set_coop_level(self, window, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
}

HRESULT __stdcall get_device_data_hook(IDirectInputDevice8A& self,
                                       DWORD data_size, LPVOID data)
{
   if (g_input_mode == Input_mode::normal) {
      return g_actual_get_device_state(self, data_size, data);
   }

   std::memset(data, 0, data_size);

   return DI_OK;
}

void install_dinput_hooks(IDirectInput8A& direct_input) noexcept
{
   static bool installed = false;

   if (installed) return;

   installed = true;

   Com_ptr<IDirectInputDevice8A> device;

   direct_input.CreateDevice(GUID_SysKeyboard, device.clear_and_assign(), nullptr);

   g_actual_set_coop_level =
      hook_vtable<Set_coop_level_type>(*device, 13, set_cooperative_Level_hook);

   g_actual_get_device_state =
      hook_vtable<Get_device_state_type>(*device, 9, get_device_data_hook);
}
}

void initialize_input_hooks(const DWORD thread_id, IDirectInput8A& direct_input) noexcept
{
   if (g_hook != nullptr) return;

   g_hook = SetWindowsHookExW(WH_GETMESSAGE, &get_message_hook, nullptr, thread_id);

   install_dinput_hooks(direct_input);
}

void close_input_hooks() noexcept
{
   if (g_hook) {
      UnhookWindowsHookEx(g_hook);

      g_hook = nullptr;
   }
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
