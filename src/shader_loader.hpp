#pragma once

#include "shader_database.hpp"
#include "ucfb_reader.hpp"

#include <utility>

#include <d3d9.h>

namespace sp {

auto load_shader(ucfb::Reader reader, IDirect3DDevice9& device)
   -> std::pair<std::string, Shader_group>;
}
