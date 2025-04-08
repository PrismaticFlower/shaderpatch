#pragma once

#include <Windows.h>

namespace sp {

void init_window_hook_game_thread_id(DWORD id) noexcept;

}

extern "C" HWND WINAPI CreateWindowExA_hook(DWORD dwExStyle, LPCSTR lpClassName,
                                            LPCSTR lpWindowName, DWORD dwStyle,
                                            int X, int Y, int nWidth, int nHeight,
                                            HWND hWndParent, HMENU hMenu,
                                            HINSTANCE hInstance, LPVOID lpParam);