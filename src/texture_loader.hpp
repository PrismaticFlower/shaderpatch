#pragma once

#include "com_ptr.hpp"

#include <d3d9.h>

namespace sp {

auto load_dds_from_file(IDirect3DDevice9& device, std::wstring file) noexcept
   -> Com_ptr<IDirect3DTexture9>;
}
