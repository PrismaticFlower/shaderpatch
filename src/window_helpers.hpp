#pragma once

#include <glm/glm.hpp>
#include <gsl/gsl>

#include <Windows.h>

namespace sp::win32 {

inline void make_borderless_window(const HWND window)
{
   Expects(IsWindow(window));

   SetWindowLongPtrA(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
   SetWindowLongPtrA(window, GWL_EXSTYLE, WS_EX_APPWINDOW);
   ShowWindow(window, SW_NORMAL);
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

inline void position_window(const HWND window, const UINT width, const UINT height)
{
   Expects(IsWindow(window));

   MONITORINFO mon_info{sizeof(MONITORINFO)};

   GetMonitorInfoA(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &mon_info);
   const RECT mon_area = mon_info.rcMonitor;

   const UINT monitor_width = mon_area.right - mon_area.left;
   const UINT monitor_height = mon_area.bottom - mon_area.top;

   const UINT offset_x = (monitor_width - width) / 2;
   const UINT offset_y = (monitor_height - height) / 2;

   SetWindowPos(window, HWND_TOP, mon_area.left + offset_x,
                mon_area.top + offset_y, width, height, SWP_SHOWWINDOW);
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

inline void clip_cursor_to_window(const HWND window)
{
   Expects(IsWindow(window));

   RECT rect = {};
   if (!GetClientRect(window, &rect)) return;

   POINT points[2] = {
      {.x = rect.left, .y = rect.top},
      {.x = rect.right, .y = rect.bottom},
   };

   MapWindowPoints(window, nullptr, &points[0], 2);

   rect = {
      .left = points[0].x,
      .top = points[0].y,
      .right = points[1].x,
      .bottom = points[1].y,
   };

   ClipCursor(&rect);
}

}
