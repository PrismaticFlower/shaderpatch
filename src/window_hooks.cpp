
#include "window_hooks.hpp"
#include "logger.hpp"
#include "user_config.hpp"

#include <atomic>

namespace sp {

namespace {

std::atomic_flag set_dpi_awareness{};
DWORD game_thread_id{};

}

void init_window_hook_game_thread_id(DWORD id) noexcept
{
   game_thread_id = id;
}

}

using namespace sp;

extern "C" HWND WINAPI CreateWindowExA_hook(DWORD dwExStyle, LPCSTR lpClassName,
                                            LPCSTR lpWindowName, DWORD dwStyle,
                                            int X, int Y, int nWidth, int nHeight,
                                            HWND hWndParent, HMENU hMenu,
                                            HINSTANCE hInstance, LPVOID lpParam)
{
   if (GetCurrentThreadId() == game_thread_id && user_config.enabled &&
       user_config.display.dpi_aware && !set_dpi_awareness.test_and_set()) {
      log(Log_level::info, "Setting game thread as Per Monitor Aware DPI aware."sv);

      SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
   }

   return CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y,
                          nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}