
#include "direct3d/creator.hpp"
#include "file_hooks.hpp"
#include "logger.hpp"
#include "unlock_memory.hpp"
#include "user_config.hpp"
#include "window_hooks.hpp"

#include <string.h>

#include <Windows.h>
#include <detours/detours.h>

extern "C" IDirect3D9* __stdcall shader_patch_direct3d9_create(UINT sdk) noexcept;

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

void install_game_d3d9_redirection() noexcept
{
   struct enumerate_context {
      bool inside_d3d9_imports = false;
      bool processed_d3d9_imports = false;
   };

   enumerate_context context{};

   DetourEnumerateImportsEx(
      GetModuleHandleW(nullptr), &context,
      [](PVOID void_context, [[maybe_unused]] HMODULE module, LPCSTR name) noexcept -> BOOL {
         auto& context = *static_cast<enumerate_context*>(void_context);

         if (context.processed_d3d9_imports) return false;

         if (name && _strcmpi("d3d9.dll", name) == 0) {
            context.inside_d3d9_imports = true;
         }

         return true;
      },
      [](PVOID void_context, [[maybe_unused]] ULONG ordinal, PCSTR name,
         PVOID* func_ptr_ptr) noexcept -> BOOL {
         if (!name) return false;

         auto& context = *static_cast<enumerate_context*>(void_context);

         if (context.inside_d3d9_imports) {
            context.processed_d3d9_imports = true;

            if (strcmp("Direct3DCreate9", name) == 0) {
               auto memory_lock = sp::unlock_memory(func_ptr_ptr, sizeof(void*));

               *func_ptr_ptr = shader_patch_direct3d9_create;

               return false;
            }

            return true;
         }

         return true;
      });
}

}

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID)
{
   if (reason == DLL_PROCESS_ATTACH) {
      sp::install_file_hooks();
      sp::install_window_hooks(GetCurrentThreadId());
      install_game_d3d9_redirection();
   }

   return true;
}

extern "C" IDirect3D9* __stdcall direct3d9_create(UINT sdk) noexcept
{
   const static auto d3d9_create =
      get_dll_export<decltype(Direct3DCreate9)>(load_system_d3d9_dll(),
                                                "Direct3DCreate9");

   return d3d9_create(sdk);
}

extern "C" IDirect3D9* __stdcall shader_patch_direct3d9_create(UINT sdk) noexcept
{
   if (sp::user_config.enabled) return sp::d3d9::Creator::create().release();

   return direct3d9_create(sdk);
}

// d3d9.dll functions

extern "C" HRESULT __stdcall direct3d9_create_ex_stub(UINT sdk,
                                                      IDirect3D9Ex** out_id3d9ex) noexcept
{
   const static auto direct3d9_create_ex =
      get_dll_export<decltype(direct3d9_create_ex_stub)>(load_system_d3d9_dll(),
                                                         "Direct3DCreate9Ex");

   return direct3d9_create_ex(sdk, out_id3d9ex);
}

struct IDirect3DShaderValidator9;

extern "C" IDirect3DShaderValidator9* __stdcall direct3d_shader_validator_create9_stub() noexcept
{
   const static auto direct3d_shader_validator_create9 =
      get_dll_export<decltype(direct3d_shader_validator_create9_stub)>(
         load_system_d3d9_dll(), "Direct3DShaderValidatorCreate9");

   return direct3d_shader_validator_create9();
}

extern "C" int __stdcall d3dperf_begin_event_stub(D3DCOLOR color, LPCWSTR name) noexcept
{
   const static auto d3dperf_begin_event =
      get_dll_export<decltype(d3dperf_begin_event_stub)>(load_system_d3d9_dll(),
                                                         "D3DPERF_BeginEvent");

   return d3dperf_begin_event(color, name);
}

extern "C" int __stdcall d3dperf_end_event_stub() noexcept
{
   const static auto d3dperf_end_event =
      get_dll_export<decltype(d3dperf_end_event_stub)>(load_system_d3d9_dll(),
                                                       "D3DPERF_EndEvent");

   return d3dperf_end_event();
}

extern "C" void __stdcall d3dperf_set_marker_stub(D3DCOLOR color, LPCWSTR name) noexcept
{
   const static auto d3dperf_set_marker =
      get_dll_export<decltype(d3dperf_set_marker_stub)>(load_system_d3d9_dll(),
                                                        "D3DPERF_SetMarker");

   return d3dperf_set_marker(color, name);
}

extern "C" void __stdcall d3dperf_set_region_stub(D3DCOLOR color, LPCWSTR name) noexcept
{
   const static auto d3dperf_set_region =
      get_dll_export<decltype(d3dperf_set_region_stub)>(load_system_d3d9_dll(),
                                                        "D3DPERF_SetRegion");

   return d3dperf_set_region(color, name);
}

extern "C" BOOL __stdcall d3dperf_query_repeat_frame_stub() noexcept
{
   const static auto d3dperf_query_repeat_frame =
      get_dll_export<decltype(d3dperf_query_repeat_frame_stub)>(load_system_d3d9_dll(), "D3DPERF_QueryRepeatFrame");

   return d3dperf_query_repeat_frame();
}

extern "C" void __stdcall d3dperf_set_options_stub(DWORD options) noexcept
{
   const static auto d3dperf_set_options =
      get_dll_export<decltype(d3dperf_set_options_stub)>(load_system_d3d9_dll(),
                                                         "D3DPERF_SetOptions");

   return d3dperf_set_options(options);
}

extern "C" DWORD __stdcall d3dperf_get_status_stub() noexcept
{
   const static auto d3dperf_get_status =
      get_dll_export<decltype(d3dperf_get_status_stub)>(load_system_d3d9_dll(),
                                                        "D3DPERF_GetStatus");

   return d3dperf_get_status();
}