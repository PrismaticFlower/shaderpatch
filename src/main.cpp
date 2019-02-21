
#include "direct3d/creator.hpp"
#include "file_hooks.hpp"

#include <Windows.h>

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID)
{
   if (reason == DLL_PROCESS_ATTACH) sp::install_file_hooks();

   return true;
}

extern "C" IDirect3D9* __stdcall direct3d9_create(UINT) noexcept
{
   return sp::d3d9::Creator::create().release();
}
