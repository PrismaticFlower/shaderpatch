#pragma once

#include <d3d9.h>

namespace sp::direct3d {

void check_required_features(IDirect3D9& d3d) noexcept;
}
