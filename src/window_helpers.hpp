#pragma once

#include <glm/glm.hpp>
#include <gsl/gsl>

#include <Windows.h>

namespace sp {

inline void make_borderless_window(const HWND window)
{
   Expects(IsWindow(window));

   SetWindowLongPtr(window, GWL_STYLE, WS_POPUP | WS_VISIBLE);
   SetWindowPos(window, nullptr, 0, 0, 0, 0,
                SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER |
                   SWP_NOACTIVATE);
}

inline void resize_window(const HWND window, const glm::uvec2 size)
{
   Expects(IsWindow(window));

   SetWindowPos(window, nullptr, 0, 0, size.x, size.y,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

inline void centre_window(const HWND window)
{
   Expects(IsWindow(window));

   MONITORINFO mon_info{sizeof(MONITORINFO)};

   GetMonitorInfoA(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &mon_info);
   const auto mon_area = mon_info.rcMonitor;

   RECT win_rect;
   GetWindowRect(window, &win_rect);

   auto x =
      (mon_area.left + mon_area.right) / 2 - (win_rect.right - win_rect.left) / 2;
   auto y =
      (mon_area.top + mon_area.bottom) / 2 - (win_rect.bottom - win_rect.top) / 2;

   // if (x > mon_area.left) x = mon_area.left;
   // if (y > mon_area.top) y = mon_area.top;

   SetWindowPos(window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

inline void clip_cursor_to_window(const HWND window)
{
   Expects(IsWindow(window));

   RECT rect;
   GetClientRect(window, &rect);

   POINT upper_left;
   upper_left.x = rect.left;
   upper_left.y = rect.top;

   POINT bottom_right;
   bottom_right.x = rect.right;
   bottom_right.y = rect.bottom;

   MapWindowPoints(window, nullptr, &upper_left, 1);
   MapWindowPoints(window, nullptr, &bottom_right, 1);

   RECT translated_rect;

   translated_rect.left = upper_left.x;
   translated_rect.top = upper_left.y;
   translated_rect.right = bottom_right.x;
   translated_rect.bottom = bottom_right.y;

   ClipCursor(&translated_rect);
}
}
