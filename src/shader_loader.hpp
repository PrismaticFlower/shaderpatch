#pragma once

#include "shader_database.hpp"
#include "ucfb_reader.hpp"

#include <utility>

#include <d3d9.h>

namespace sp {

void load_shader_pack(ucfb::Reader_strict<"shpk"_mn> reader,
                      IDirect3DDevice9& device, Shader_database& database);

}
