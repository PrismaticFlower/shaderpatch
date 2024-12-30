#pragma once

#include <Windows.h>

extern "C" HRESULT WINAPI DirectInput8Create_hook(HINSTANCE hinst, DWORD dwVersion,
                                                  REFIID riidltf, LPVOID* ppvOut,
                                                  LPUNKNOWN punkOuter);