
#include "creator.hpp"

#include <type_traits>

#include <gsl/gsl>
#include <mhook-lib/mhook.h>

#include <Windows.h>
#include <d3d9.h>

namespace sp::direct3d {

namespace {

using D3d9_create = std::add_pointer_t<decltype(Direct3DCreate9)>;
D3d9_create direct3d_create_real = nullptr;

IDirect3D9* __stdcall direct3d_create_hook(UINT) noexcept
{
   Com_ptr actual{direct3d_create_real(D3D_SDK_VERSION)};

   return Creator::create(std::move(actual)).release();
}

}

bool initialize_hook() noexcept
{
   Expects(!direct3d_create_real);

   direct3d_create_real = reinterpret_cast<D3d9_create>(
      GetProcAddress(GetModuleHandleW(L"d3d9.dll"), "Direct3DCreate9"));

   return Mhook_SetHook(reinterpret_cast<void**>(&direct3d_create_real),
                        direct3d_create_hook) != 0;
}

}
