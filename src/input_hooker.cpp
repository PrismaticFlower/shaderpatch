

#include "input_hooker.hpp"
#include "com_ptr.hpp"
#include "input_config.hpp"
#include "logger.hpp"
#include "utility.hpp"
#include "window_helpers.hpp"

#include <array>

#include "imgui/imgui.h"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                                      WPARAM wParam, LPARAM lParam);

namespace sp {

namespace {

HWND game_window = nullptr;
HHOOK get_message_hhook = nullptr;
HHOOK wnd_proc_hhook = nullptr;

LRESULT CALLBACK get_message_hook(int code, WPARAM w_param, LPARAM l_param)
{
   MSG& msg = *reinterpret_cast<MSG*>(l_param);

   if (code < 0 || w_param == PM_NOREMOVE || msg.hwnd != game_window) {
      return CallNextHookEx(get_message_hhook, code, w_param, l_param);
   }

   switch (msg.message) {
   case WM_KEYUP:
      if (const auto key = msg.wParam;
          key == input_config.hotkey && input_config.hotkey_func) {
         input_config.hotkey_func();
      }
      else if (key == VK_SNAPSHOT && input_config.screenshot_func) {
         input_config.screenshot_func();
      }

      [[fallthrough]];
   default:
      if (input_config.mode == Input_mode::normal) break;

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

   if (msg.message == WM_SETFOCUS) {
      win32::clip_cursor_to_window(msg.hwnd);
   }
   else if (msg.message == WM_KILLFOCUS) {
      ClipCursor(nullptr);
   }

   return CallNextHookEx(wnd_proc_hhook, code, w_param, l_param);
}

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
}