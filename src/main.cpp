
#include "com_ptr.hpp"
#include "direct3d/creator.hpp"
#include "hook_vtable.hpp"
#include "logger.hpp"

#include <exception>
#include <string>

#include <d3d9.h>

namespace {
using namespace sp;

HMODULE load_d3d9_dll() noexcept
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

template<typename Func, typename Func_ptr = std::add_pointer_t<Func>>
Func_ptr get_d3d9_export(const char* name)
{
   const static auto lib_handle = load_d3d9_dll();

   const auto proc = reinterpret_cast<Func_ptr>(GetProcAddress(lib_handle, name));

   if (proc == nullptr) std::terminate();

   return proc;
}

const auto true_direct3d_create =
   get_d3d9_export<IDirect3D9* __stdcall(UINT)>("Direct3DCreate9");
}

extern "C" IDirect3D9* __stdcall direct3d9_create(UINT) noexcept
{
   Com_ptr actual{true_direct3d_create(D3D_SDK_VERSION)};

   return direct3d::Creator::create(std::move(actual)).release();
}
