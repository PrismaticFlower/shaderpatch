

#include "message_hooks.hpp"
#include "input_config.hpp"
#include "logger.hpp"
#include "window_helpers.hpp"

#include "imgui/imgui.h"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                                      WPARAM wParam, LPARAM lParam);

namespace sp {

namespace {

HWND game_window = nullptr;
WNDPROC game_wnd_proc = nullptr;

LRESULT CALLBACK wnd_proc_hook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   if (hWnd != game_window) {
      return CallWindowProcA(game_wnd_proc, hWnd, message, wParam, lParam);
   }

   if (input_config.mode == Input_mode::imgui) {
      ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam);
   }

   switch (message) {
   case WM_ACTIVATEAPP: {
      if (wParam == TRUE) {
         if (input_config.activate_app_func) input_config.activate_app_func();
      }
   } break;
   case WM_KEYUP: {
      if (const auto key = wParam;
          key == input_config.hotkey && input_config.hotkey_func) {
         input_config.hotkey_func();
      }
      else if (key == VK_SNAPSHOT && input_config.screenshot_func) {
         input_config.screenshot_func();
      }
   } break;
   case WM_WINDOWPOSCHANGING: {
      WINDOWPOS* window_pos = reinterpret_cast<WINDOWPOS*>(lParam);

      MONITORINFO mon_info{sizeof(MONITORINFO)};

      GetMonitorInfoA(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &mon_info);
      const RECT mon_area = mon_info.rcMonitor;

      const UINT monitor_width = mon_area.right - mon_area.left;
      const UINT monitor_height = mon_area.bottom - mon_area.top;

      const UINT offset_x = (monitor_width - window_pos->cx) / 2;
      const UINT offset_y = (monitor_height - window_pos->cy) / 2;

      window_pos->x = mon_area.left + offset_x;
      window_pos->y = mon_area.top + offset_y;

      return 0;
   } break;
   }

   return CallWindowProcA(game_wnd_proc, hWnd, message, wParam, lParam);
}

}

void install_message_hooks(const HWND window) noexcept
{
   if (!window) {
      log(Log_level::error, "No window passed to install_message_hooks. Some "
                            "features may not work correctly.");

      return;
   }

   if (std::exchange(game_window, window)) {
      log_and_terminate("Attempt to install window hooks twice.");
   }

   game_wnd_proc =
      (WNDPROC)SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)&wnd_proc_hook);
}
}