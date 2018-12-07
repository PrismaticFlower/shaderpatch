
#include "direct3d/creator.hpp"

extern "C" IDirect3D9* __stdcall direct3d9_create(UINT) noexcept
{
   return sp::d3d9::Creator::create().release();
}
