#pragma once

#include "../core/shader_patch.hpp"
#include "../logger.hpp"
#include "com_ptr.hpp"

#include <d3d9.h>

namespace sp::d3d9 {

Com_ptr<IDirect3DQuery9> make_query(core::Shader_patch& shader_patch,
                                    const D3DQUERYTYPE type) noexcept;

}
