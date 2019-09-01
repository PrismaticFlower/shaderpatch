
#include "direct3d/creator.hpp"
#include "file_hooks.hpp"
#include "input_hooker.hpp"
#include "user_config.hpp"

#include <Windows.h>

namespace {
auto load_system_d3d9_dll() noexcept -> HMODULE
{
   std::wstring buffer;
   buffer.resize(512u);

   const auto size = GetSystemDirectoryW(buffer.data(), buffer.size());
   buffer.resize(size);

   buffer += LR"(\d3d9.dll)";

   const static auto handle = LoadLibraryW(buffer.c_str());

   if (handle == nullptr) std::terminate();

   return handle;
}

template<typename Func>
auto get_dll_export(const HMODULE dll_handle, const char* const export_name)
   -> std::add_pointer_t<Func>
{
   const auto address = reinterpret_cast<std::add_pointer_t<Func>>(
      GetProcAddress(dll_handle, export_name));

   if (address == nullptr) std::terminate();

   return address;
}

}

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID)
{
   if (reason == DLL_PROCESS_ATTACH) {
      sp::install_file_hooks();
      sp::install_dinput_hooks();
   }

   return true;
}

extern "C" IDirect3D9* __stdcall direct3d9_create(UINT sdk) noexcept
{
   if (!sp::user_config.enabled) {
      const static auto d3d9_handle = load_system_d3d9_dll();
      const static auto d3d9_create =
         get_dll_export<decltype(Direct3DCreate9)>(d3d9_handle,
                                                   "Direct3DCreate9");

      return d3d9_create(sdk);
   }

   return sp::d3d9::Creator::create().release();
}
