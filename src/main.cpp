
#include "com_ptr.hpp"
#include "direct3d/hook.hpp"
#include "hook_vtable.hpp"
#include "input_hooker.hpp"
#include "logger.hpp"

#include <exception>
#include <string>

#include <d3d9.h>
#include <dinput.h>

namespace {
using namespace sp;

HMODULE load_dinput_dll() noexcept
{
   std::wstring buffer;
   buffer.resize(512u);

   const auto size = GetSystemDirectoryW(buffer.data(), buffer.size());
   buffer.resize(size);

   buffer += LR"(\DINPUT8.dll)";

   const static auto handle = LoadLibraryW(buffer.c_str());

   if (handle == nullptr) std::terminate();

   return handle;
}

template<typename Func, typename Func_ptr = std::add_pointer_t<Func>>
Func_ptr get_dinput_export(const char* name)
{
   const static auto lib_handle = load_dinput_dll();

   const auto proc = reinterpret_cast<Func_ptr>(GetProcAddress(lib_handle, name));

   if (proc == nullptr) std::terminate();

   return proc;
}

void hook_direct3d() noexcept
{
   if (!direct3d::initialize_hook()) {
      log_and_terminate("Failed to hook Direct3D!");
   }
}

}

extern "C" HRESULT __stdcall directinput8_create(HINSTANCE instance, DWORD version,
                                                 REFIID iid, LPVOID* out,
                                                 LPUNKNOWN unknown_outer)
{
   hook_direct3d();

   using DirectInput8Create_Proc =
      HRESULT __stdcall(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

   const auto directinput_create =
      get_dinput_export<DirectInput8Create_Proc>("DirectInput8Create");

   const auto result = directinput_create(instance, version, iid, out, unknown_outer);

   initialize_input_hooks(GetCurrentThreadId(), *static_cast<IDirectInput8A*>(*out));

   return result;
}
