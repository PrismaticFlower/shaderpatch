#pragma once

#include <d3d11_4.h>

namespace sp::core {

enum class Game_rendertarget_id : int {};

enum class Game_depthstencil { nearscene, farscene, reflectionscene, none };

enum class Projtex_mode { clamp, wrap };

enum class Projtex_type { tex2d, texcube };

enum class Clear_color { transparent_black, opaque_black };

enum class Map {
   write = D3D11_MAP_WRITE,
   write_discard = D3D11_MAP_WRITE_DISCARD,
   write_nooverwrite = D3D11_MAP_WRITE_NO_OVERWRITE
};

struct Mapped_texture {
   UINT row_pitch;
   UINT depth_pitch;
   std::byte* data;
};

}
