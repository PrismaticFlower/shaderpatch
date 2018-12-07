#pragma once

#include <d3d9.h>

namespace sp::d3d9 {

void check_required_features(IDirect3D9& d3d) noexcept;
}
