
#include "window_hooks.hpp"
#include "logger.hpp"
#include "user_config.hpp"

#include <atomic>

#include <detours/detours.h>

namespace sp {

namespace {

std::atomic_flag set_dpi_awareness{};
DWORD game_thread_id{};

decltype(&CreateWindowExA) true_CreateWindowExA = CreateWindowExA;

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

   return true_CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X,
                               Y, nWidth, nHeight, hWndParent, hMenu, hInstance,
                               lpParam);
}

}

void install_window_hooks(const DWORD main_thread_id) noexcept
{
   game_thread_id = main_thread_id;

   bool failure = true;

   if (DetourTransactionBegin() == NO_ERROR) {
      if (DetourAttach(&reinterpret_cast<PVOID&>(true_CreateWindowExA),
                       CreateWindowExA_hook) == NO_ERROR) {
         if (DetourTransactionCommit() == NO_ERROR) {
            failure = false;
         }
      }
   }

   if (failure) {
      log_and_terminate("Failed to install file hooks."sv);
   }
}

}
