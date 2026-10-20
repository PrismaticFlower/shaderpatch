#pragma once

#include <glm/glm.hpp>
#include <gsl/gsl>

#include <Windows.h>

namespace sp::win32 {

inline void make_borderless_window(const HWND window)
{
   Expects(IsWindow(window));

   SetWindowLongPtrA(window, GWL_EXSTYLE, WS_EX_APPWINDOW);
   SetWindowLongPtrA(window, GWL_STYLE,
                     GetWindowLongPtrA(window, GWL_STYLE) & ~(WS_CAPTION | WS_THICKFRAME) |
                        WS_POPUP);
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

}
