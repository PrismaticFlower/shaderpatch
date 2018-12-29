#pragma once

#include <glm/glm.hpp>
#include <gsl/gsl>

#include <Windows.h>

namespace sp::win32 {

inline void make_borderless_window(const HWND window)
{
   Expects(IsWindow(window));

   SetWindowLongPtr(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
   SetWindowPos(window, nullptr, 0, 0, 0, 0,
                SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER |
                   SWP_NOACTIVATE | SWP_NOSENDCHANGING);
}

inline void resize_window(const HWND window, const UINT width, const UINT height)
{
   Expects(IsWindow(window));

   RECT rect{};
   rect.right = width;
   rect.bottom = height;

   AdjustWindowRectEx(&rect, GetWindowLongPtr(window, GWL_STYLE), FALSE,
                      GetWindowLongPtr(window, GWL_EXSTYLE));

   SetWindowPos(window, HWND_TOP, 0, 0, rect.right - rect.left,
                rect.bottom - rect.top,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
}

inline void centre_window(const HWND window)
{
   Expects(IsWindow(window));

   MONITORINFO mon_info{sizeof(MONITORINFO)};

   GetMonitorInfoA(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &mon_info);
   const auto mon_area = mon_info.rcMonitor;

   RECT win_rect;
   GetWindowRect(window, &win_rect);

   const auto x =
      (mon_area.left + mon_area.right) / 2 - (win_rect.right - win_rect.left) / 2;
   const auto y =
      (mon_area.top + mon_area.bottom) / 2 - (win_rect.bottom - win_rect.top) / 2;

   // if (x > mon_area.left) x = mon_area.left;
   // if (y > mon_area.top) y = mon_area.top;

   SetWindowPos(window, HWND_TOP, x, y, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
}

inline void leftalign_window(const HWND window)
{
   Expects(IsWindow(window));

   MONITORINFO mon_info{sizeof(MONITORINFO)};

   GetMonitorInfoA(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &mon_info);
   const auto mon_area = mon_info.rcMonitor;

   SetWindowPos(window, HWND_TOP, mon_area.left, mon_area.top, 0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
}

}
